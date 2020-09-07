// SPDX-License-Identifier: GPL-3.0-only

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <filesystem>

#include <invader/build/build_workload.hpp>
#include <invader/map/map.hpp>
#include <invader/version.hpp>
#include <invader/printf.hpp>
#include <invader/command_line_option.hpp>
#include <invader/file/file.hpp>
#include <invader/tag/index/index.hpp>

enum ReturnValue : int {
    RETURN_OK = 0,
    RETURN_FAILED_NOTHING_TO_DO = 1,
    RETURN_FAILED_UNKNOWN_ARGUMENT = 2,
    RETURN_FAILED_UNHANDLED_ARGUMENT = 3,
    RETURN_FAILED_FILE_SAVE_ERROR = 4,
    RETURN_FAILED_EXCEPTION_ERROR = 5,
    RETURN_FAILED_INVALID_ARGUMENT = 6
};

static std::uint32_t read_str32(const char *err, const char *s) {
    std::size_t given_crc32_length = std::strlen(s);
    if(given_crc32_length > 8 || given_crc32_length < 1) {
        eprintf_error("%s %s (must be 1-8 digits)", err, s);
        std::exit(RETURN_FAILED_INVALID_ARGUMENT);
    }
    for(std::size_t i = 0; i < given_crc32_length; i++) {
        char c = std::tolower(s[i]);
        if(!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f')) {
            eprintf_error("%s %s (must be hexadecimal)", err, s);
            std::exit(RETURN_FAILED_INVALID_ARGUMENT);
        }
    }
    return static_cast<std::uint32_t>(std::strtoul(s, nullptr, 16));
}

int main(int argc, const char **argv) {
    using namespace Invader;
    using namespace Invader::HEK;

    // Parameters
    struct BuildOptions {
        std::string maps = "maps";
        std::vector<std::string> tags;
        std::string output;
        std::string last_argument;
        std::string index;
        std::optional<HEK::CacheFileEngine> engine;
        bool handled = true;
        bool quiet = false;
        BuildWorkload::RawDataHandling raw_data_handling = BuildWorkload::RawDataHandling::RAW_DATA_HANDLING_DEFAULT;
        std::optional<std::uint32_t> forged_crc;
        bool use_filesystem_path = false;
        const char *rename_scenario = nullptr;
        std::optional<bool> compress;
        bool optimize_space = false;
        bool hide_pedantic_warnings = false;
    } build_options;

    std::vector<CommandLineOption> options;
    options.emplace_back("no-external-tags", 'n', 0, "Do not use external tags. This can speed up build time at a cost of a much larger file size.");
    options.emplace_back("always-index-tags", 'a', 0, "Always index tags when possible. This can speed up build time, but stock tags can't be modified.");
    options.emplace_back("discard", 'd', 0, "Discard all raw data. This will result in a map that is invalid for use with game clients (except MCC) and tag extractors.");
    options.emplace_back("quiet", 'q', 0, "Only output error messages.");
    options.emplace_back("info", 'i', 0, "Show credits, source info, and other info.");
    options.emplace_back("game-engine", 'g', 1, "Specify the game engine. This option is required. Valid engines are: custom, demo, native, retail", "<id>");
    options.emplace_back("with-index", 'w', 1, "Use an index file for the tags, ensuring the map's tags are ordered in the same way.", "<file>");
    options.emplace_back("maps", 'm', 1, "Use the specified maps directory.", "<dir>");
    options.emplace_back("tags", 't', 1, "Use the specified tags directory. Use multiple times to add more directories, ordered by precedence.", "<dir>");
    options.emplace_back("output", 'o', 1, "Output to a specific file.", "<file>");
    options.emplace_back("forge-crc", 'C', 1, "Forge the CRC32 value of the map after building it.", "<crc>");
    options.emplace_back("fs-path", 'P', 0, "Use a filesystem path for the tag.");
    options.emplace_back("rename-scenario", 'N', 1, "Rename the scenario.", "<name>");
    options.emplace_back("compress", 'c', 0, "Compress the cache file.");
    options.emplace_back("uncompressed", 'u', 0, "Do not compress the cache file. This is default for demo, retail, and custom engines.");
    options.emplace_back("optimize", 'O', 0, "Optimize tag space. This will drastically increase the amount of time required to build the cache file.");
    options.emplace_back("hide-pedantic-warnings", 'H', 0, "Don't show minor warnings.");

    static constexpr char DESCRIPTION[] = "Build a cache file for a version of Halo: Combat Evolved.";
    static constexpr char USAGE[] = "[options] -g <target> <scenario>";

    auto remaining_arguments = CommandLineOption::parse_arguments<BuildOptions &>(argc, argv, options, USAGE, DESCRIPTION, 1, 1, build_options, [](char opt, const auto &arguments, auto &build_options) {
        switch(opt) {
            case 'n':
                build_options.raw_data_handling = BuildWorkload::RawDataHandling::RAW_DATA_HANDLING_RETAIN_ALL;
                break;
            case 'a':
                build_options.raw_data_handling = BuildWorkload::RawDataHandling::RAW_DATA_HANDLING_ALWAYS_INDEX;
                break;
            case 'd':
                build_options.raw_data_handling = BuildWorkload::RawDataHandling::RAW_DATA_HANDLING_REMOVE_ALL;
                break;
            case 'q':
                build_options.quiet = true;
                break;
            case 'w':
                build_options.index = std::string(arguments[0]);
                break;
            case 't':
                build_options.tags.emplace_back(arguments[0]);
                break;
            case 'o':
                build_options.output = std::string(arguments[0]);
                break;
            case 'm':
                build_options.maps = std::string(arguments[0]);
                break;
            case 'g':
                if(std::strcmp(arguments[0], "custom") == 0) {
                    build_options.engine = HEK::CacheFileEngine::CACHE_FILE_CUSTOM_EDITION;
                }
                else if(std::strcmp(arguments[0], "retail") == 0) {
                    build_options.engine = HEK::CacheFileEngine::CACHE_FILE_RETAIL;
                }
                else if(std::strcmp(arguments[0], "demo") == 0) {
                    build_options.engine = HEK::CacheFileEngine::CACHE_FILE_DEMO;
                }
                else if(std::strcmp(arguments[0], "native") == 0) {
                    build_options.engine = HEK::CacheFileEngine::CACHE_FILE_NATIVE;
                }
                else {
                    eprintf_error("Unknown engine type %s.", arguments[0]);
                    std::exit(RETURN_FAILED_INVALID_ARGUMENT);
                }
                break;
            case 'C':
                build_options.forged_crc = read_str32("Invalid CRC32", arguments[0]);
                break;
            case 'c':
                build_options.compress = true;
                break;
            case 'u':
                build_options.compress = false;
                break;
            case 'P':
                build_options.use_filesystem_path = true;
                break;
            case 'i':
                show_version_info();
                std::exit(RETURN_OK);
                break;
            case 'N':
                build_options.rename_scenario = arguments[0];
                break;
            case 'O':
                build_options.optimize_space = true;
                break;
            case 'H':
                build_options.hide_pedantic_warnings = true;
                break;
        }
    });
    
    std::string scenario;

    // By default, just use tags
    if(build_options.tags.size() == 0) {
        build_options.tags.emplace_back("tags");
    }

    if(build_options.use_filesystem_path) {
        auto scenario_maybe = Invader::File::file_path_to_tag_path_with_extension(remaining_arguments[0], build_options.tags, ".scenario");
        if(scenario_maybe.has_value()) {
            scenario = scenario_maybe.value();
        }
        else {
            eprintf_error("Failed to find a valid tag %s in the tags directory", remaining_arguments[0]);
            return RETURN_FAILED_UNHANDLED_ARGUMENT;
        }
    }
    else {
        scenario = File::halo_path_to_preferred_path(remaining_arguments[0]);
    }

    try {
        // Get the index
        std::optional<std::vector<Invader::File::TagFilePath>> with_index;
        if(build_options.index.size()) {
            with_index = std::vector<Invader::File::TagFilePath>();
            
            std::fstream index_file(build_options.index, std::ios_base::in);
            std::string tag;
            while(std::getline(index_file, tag)) {
                // Check if empty
                if(tag.size() == 0) {
                    break;
                }

                // Get the extension
                const char *extension = nullptr;
                for(char &c : tag) {
                    if(c == '.') {
                        extension = &c + 1;
                    }
                }

                if(!extension) {
                    eprintf_error("Invalid index given. \"%s\" is missing an extension.", tag.c_str());
                    return EXIT_FAILURE;
                }

                auto substr = tag.substr(0, extension - tag.c_str() - 1);
                const char *substr_c = substr.c_str();
                std::vector<char> substr_v(substr_c, substr_c + substr.size() + 1);
                File::preferred_path_to_halo_path_chars(substr_v.data());

                // Lowercase everything
                for(char &c : substr) {
                    c = std::tolower(c);
                }

                with_index->emplace_back(substr_v.data(), extension_to_tag_class(extension));
            }
        }

        // Figure out our engine target
        if(!build_options.engine.has_value()) {
            eprintf_error("No engine target specified. Use -h for more information.");
            return 1;
        }
    
        // Default compress value
        if(!build_options.compress.has_value()) {
            switch(*build_options.engine) {
                case HEK::CacheFileEngine::CACHE_FILE_CUSTOM_EDITION:
                case HEK::CacheFileEngine::CACHE_FILE_RETAIL:
                case HEK::CacheFileEngine::CACHE_FILE_DEMO:
                    build_options.compress = false;
                    break;
                case HEK::CacheFileEngine::CACHE_FILE_NATIVE:
                case HEK::CacheFileEngine::CACHE_FILE_XBOX:
                    build_options.compress = true;
                    break;
                default:
                    break;
            }
        }
        
        // Instantiate build parameters
        BuildWorkload::BuildParameters build_parameters(*build_options.engine);
        build_parameters.raw_data_handling = build_options.raw_data_handling;
        
        // Attempt to open the resource map
        auto open_resource_map = [&build_options, &build_parameters](const char *map, const char *map_alt = nullptr) -> std::vector<Resource> {
            if(build_parameters.raw_data_handling == BuildWorkload::RawDataHandling::RAW_DATA_HANDLING_RETAIN_ALL || build_parameters.raw_data_handling == BuildWorkload::RawDataHandling::RAW_DATA_HANDLING_REMOVE_ALL) {
                return std::vector<Resource>();
            }
            else {
                if(map_alt) {
                    oprintf("Reading %s or %s...", map, map_alt);
                }
                else {
                    oprintf("Reading %s...", map);
                }
                oflush();

                // Make two paths
                auto map_path = std::filesystem::path(build_options.maps) / map;
                auto map_path_str = map_path.string();
                auto map_path_alt = std::filesystem::path(build_options.maps) / (map_alt ? map_alt : map);
                auto map_path_alt_str = map_path_alt.string();

                // Try to open either
                auto map_data = Invader::File::open_file(map_path_str.c_str());
                if(!map_data.has_value() && map_alt) {
                    map_data = Invader::File::open_file(map_path_alt_str.c_str());
                }

                if(!map_data.has_value()) {
                    oprintf(" failed\n");
                    if(map_alt) {
                        eprintf_error("Failed to open %s or %s", map_path_str.c_str(), map_path_alt_str.c_str());
                    }
                    else {
                        eprintf_error("Failed to open %s", map_path_str.c_str());
                    }
                    throw FailedToOpenFileException();
                }
                oprintf(" done\n");
                return load_resource_map(map_data->data(), map_data->size());
            }
        };
        
        // Figure out our scenario name
        auto scenario_thing = File::preferred_path_to_halo_path(build_options.rename_scenario != nullptr ? build_options.rename_scenario : scenario);
        const char *scenario_name = scenario_thing.c_str();
        for(auto *i = scenario_name; *i; i++) {
            if(*i == '\\') {
                scenario_name = i + 1;
            }
        }
        
        // Did we specify an index?
        if(with_index.has_value()) {
            build_parameters.index = *with_index;
        }
        
        // If not, make one!
        else {
            switch(build_parameters.engine_target) {
                case HEK::CacheFileEngine::CACHE_FILE_CUSTOM_EDITION:
                    build_parameters.index = custom_edition_indices(scenario_name);
                    break;
                case HEK::CacheFileEngine::CACHE_FILE_RETAIL:
                    build_parameters.index = retail_indices(scenario_name);
                    break;
                case HEK::CacheFileEngine::CACHE_FILE_DEMO:
                    build_parameters.index = demo_indices(scenario_name);
                    break;
                default:
                    break;
            }
            
            // If the index is empty, remove it
            if(build_parameters.index.has_value() && build_parameters.index->size() == 0) {
                build_parameters.index = std::nullopt;
            }
            
            // Note if we have built-in indices being used
            if(build_parameters.index.has_value()) {
                oprintf_success_lesser_warn("Using built-in indices for %s...", scenario_name);
            }
        }
        
        // Are we forging the CRC?
        if(build_options.forged_crc.has_value()) {
            build_parameters.forge_crc = build_options.forged_crc;
        }

        // Next, if no CRC is passed and we need a CRC, press on
        else if(build_parameters.engine_target == CacheFileEngine::CACHE_FILE_CUSTOM_EDITION) {
            if(std::strcmp(scenario_name, "beavercreek") == 0) {
                build_parameters.forge_crc = 0x07B3876A;
            }
            else if(std::strcmp(scenario_name, "bloodgulch") == 0) {
                build_parameters.forge_crc = 0x7B309554;
            }
            else if(std::strcmp(scenario_name, "boardingaction") == 0) {
                build_parameters.forge_crc = 0xF4DEEF94;
            }
            else if(std::strcmp(scenario_name, "carousel") == 0) {
                build_parameters.forge_crc = 0x9C301A08;
            }
            else if(std::strcmp(scenario_name, "chillout") == 0) {
                build_parameters.forge_crc = 0x93C53C27;
            }
            else if(std::strcmp(scenario_name, "damnation") == 0) {
                build_parameters.forge_crc = 0x0FBA059D;
            }
            else if(std::strcmp(scenario_name, "dangercanyon") == 0) {
                build_parameters.forge_crc = 0xC410CD74;
            }
            else if(std::strcmp(scenario_name, "deathisland") == 0) {
                build_parameters.forge_crc = 0x1DF8C97F;
            }
            else if(std::strcmp(scenario_name, "gephyrophobia") == 0) {
                build_parameters.forge_crc = 0xD2872165;
            }
            else if(std::strcmp(scenario_name, "hangemhigh") == 0) {
                build_parameters.forge_crc = 0xA7C8B9C6;
            }
            else if(std::strcmp(scenario_name, "icefields") == 0) {
                build_parameters.forge_crc = 0x5EC1DEB7;
            }
            else if(std::strcmp(scenario_name, "infinity") == 0) {
                build_parameters.forge_crc = 0x0E7F7FE7;
            }
            else if(std::strcmp(scenario_name, "longest") == 0) {
                build_parameters.forge_crc = 0xC8F48FF6;
            }
            else if(std::strcmp(scenario_name, "prisoner") == 0) {
                build_parameters.forge_crc = 0x43B81A8B;
            }
            else if(std::strcmp(scenario_name, "putput") == 0) {
                build_parameters.forge_crc = 0xAF2F0B84;
            }
            else if(std::strcmp(scenario_name, "ratrace") == 0) {
                build_parameters.forge_crc = 0xF7F8E14C;
            }
            else if(std::strcmp(scenario_name, "sidewinder") == 0) {
                build_parameters.forge_crc = 0xBD95CF55;
            }
            else if(std::strcmp(scenario_name, "timberland") == 0) {
                build_parameters.forge_crc = 0x54446470;
            }
            else if(std::strcmp(scenario_name, "wizard") == 0) {
                build_parameters.forge_crc = 0xCF3359B1;
            }
            if(build_parameters.forge_crc.has_value()) {
                oprintf_success_lesser_warn("Using built-in CRC32 for %s...", scenario_name);
            }
        }

        // Load resources maybe
        switch(build_parameters.engine_target) {
            case CacheFileEngine::CACHE_FILE_DEMO:
            case CacheFileEngine::CACHE_FILE_RETAIL:
                build_parameters.bitmaps = open_resource_map("bitmaps.map");
                build_parameters.sounds = open_resource_map("sounds.map");
                break;
            case CacheFileEngine::CACHE_FILE_CUSTOM_EDITION:
                build_parameters.loc = open_resource_map("custom_loc.map", "loc.map");
                build_parameters.bitmaps = open_resource_map("custom_bitmaps.map", "bitmaps.map");
                build_parameters.sounds = open_resource_map("custom_sounds.map", "sounds.map");
                break;
            default:
                break;
        }

        // Build!
        auto map = Invader::BuildWorkload::compile_map(
            scenario.c_str(),
            build_parameters
        );

        // Set the map name
        const char *map_name;
        if(build_options.rename_scenario) {
            map_name = build_options.rename_scenario;
        }
        else {
            map_name = File::base_name_chars(scenario.c_str());
        }
        
        static const char MAP_EXTENSION[] = ".map"; 
        auto map_name_with_extension = std::string(map_name) + MAP_EXTENSION;

        // Format path to maps/map_name.map if output not specified
        std::string final_file;
        if(!build_options.output.size()) {
            final_file = (std::filesystem::path(build_options.maps) / map_name_with_extension).string();
        }
        else {
            final_file = build_options.output;
            auto final_file_path = std::filesystem::path(final_file);
            auto final_file_name_no_extension = final_file_path.filename().replace_extension();
            auto final_file_name_no_extension_string = final_file_name_no_extension.string();
            
            // If it's not a .map, warn
            if(final_file_path.extension() != MAP_EXTENSION) {
                eprintf_warn("The base file extension is not \"%s\" which is required by the target engine", MAP_EXTENSION);
            }
            
            // If we are not building for MCC and the scenario name is mismatched, warn
            if(final_file_name_no_extension_string != map_name) {
                eprintf_warn("The base name (%s) does not match the scenario (%s)", final_file_name_no_extension_string.c_str(), map_name);
                eprintf_warn("The map will fail to load correctly in the target engine with this file name.");
                
                bool incorrect_case = false;
                for(char &c : final_file_name_no_extension_string) {
                    if(std::tolower(c) != c) {
                        incorrect_case = true;
                        break;
                    }
                }
                if(!incorrect_case) {
                    eprintf_warn("Did you intend to use --rename-scenario \"%s\"", final_file_name_no_extension_string.c_str());
                }
            }
        }

        std::FILE *file = std::fopen(final_file.c_str(), "wb");

        // Check if file is open
        if(!file) {
            eprintf_error("Failed to open %s for writing.", final_file.c_str());
            return RETURN_FAILED_FILE_SAVE_ERROR;
        }

        // Write to file
        if(std::fwrite(map.data(), map.size(), 1, file) == 0) {
            eprintf_error("Failed to save.");
            return RETURN_FAILED_FILE_SAVE_ERROR;
        }

        std::fclose(file);

        return RETURN_OK;
    }
    catch(std::exception &exception) {
        eprintf_error("Failed to compile the map.");
        eprintf_error("%s", exception.what());
        return RETURN_FAILED_EXCEPTION_ERROR;
    }
}
