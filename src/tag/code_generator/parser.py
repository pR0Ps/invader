# SPDX-License-Identifier: GPL-3.0-only

import os
from compile import make_cache_format_data
from generate_hek_tag_data import make_cpp_save_hek_data
from read_cache_file_data import make_parse_cache_file_data
from read_hek_data import make_parse_hek_tag_data
from read_hek_file import make_parse_hek_tag_file
from cache_deformat_data import make_cache_deformat
from refactor_reference import make_refactor_reference
from parser_struct import make_parser_struct
from check_broken_enums import make_check_broken_enums
from check_invalid_references import make_check_invalid_references
from check_invalid_ranges import make_check_invalid_ranges
from check_invalid_indices import make_check_invalid_indices

def make_parser(all_enums, all_bitfields, all_structs_arranged, all_structs, extract_hidden, hpp, parser_sources_dir):
    hpp.write("// SPDX-License-Identifier: GPL-3.0-only\n\n// This file was auto-generated.\n// If you want to edit this, edit the .json definitions and rerun the generator script, instead.\n\n")
    header_name = "INVADER__TAG__PARSER__PARSER_HPP"
    hpp.write("#ifndef {}\n".format(header_name))
    hpp.write("#define {}\n\n".format(header_name))
    hpp.write("#include <string>\n")
    hpp.write("#include <optional>\n")
    hpp.write("#include \"../../map/map.hpp\"\n")
    hpp.write("#include \"parser_struct.hpp\"\n\n")
    hpp.write("namespace Invader {\n")
    hpp.write("    class BuildWorkload;\n")
    hpp.write("}\n")
    hpp.write("namespace Invader::Parser {\n")

    for s in all_structs_arranged:
        struct_name = s["name"]
        post_cache_deformat = "post_cache_deformat" in s and s["post_cache_deformat"]
        post_cache_parse = "post_cache_parse" in s and s["post_cache_parse"]
        pre_compile = "pre_compile" in s and s["pre_compile"]
        post_compile = "post_compile" in s and s["post_compile"]
        postprocess_hek_data = "postprocess_hek_data" in s and s["postprocess_hek_data"]
        read_only = "read_only" in s and s["read_only"]
        private_functions = post_cache_deformat
        hpp.write("\n")
        hpp.write("    #ifdef USE_{}\n".format(s["name"]))
        hpp.write("    struct {} : public ParserStruct {{\n".format(struct_name))
        hpp.write("        using struct_big = HEK::{}<HEK::BigEndian>;\n".format(struct_name))
        hpp.write("        using struct_little = HEK::{}<HEK::LittleEndian>;\n".format(struct_name))
        all_used_structs = []
        def add_structs_from_struct(struct):
            if "inherits" in struct:
                for t in all_structs:
                    if t["name"] == struct["inherits"]:
                        add_structs_from_struct(t)
                        break
            for t in struct["fields"]:
                if t["type"] == "pad":
                    continue
                type_to_write = t["type"]
                non_type = False
                if type_to_write.startswith("int") or type_to_write.startswith("uint"):
                    type_to_write = "std::{}_t".format(type_to_write)
                    non_type = True
                elif type_to_write == "float":
                    type_to_write = "float"
                    non_type = True
                elif type_to_write == "TagDependency":
                    type_to_write = "Dependency"
                    non_type = True
                elif type_to_write == "TagReflexive":
                    if t["struct"] == "PredictedResource":
                        continue
                    type_to_write = "std::vector<{}>".format(t["struct"])
                    non_type = True
                elif type_to_write == "TagDataOffset":
                    type_to_write = "std::vector<std::byte>"
                    non_type = True
                else:
                    type_to_write = "HEK::{}".format(type_to_write)
                if "flagged" in t and t["flagged"]:
                    type_to_write = "HEK::FlaggedInt<{}>".format(type_to_write)
                if "compound" in t and t["compound"] and not non_type:
                    type_to_write = "{}<HEK::NativeEndian>".format(type_to_write)
                if "bounds" in t and t["bounds"]:
                    type_to_write = "HEK::Bounds<{}>".format(type_to_write)
                hpp.write("        {} {}{};\n".format(type_to_write, t["member_name"], "" if "count" not in t or t["count"] == 1 else "[{}]".format(t["count"])))
                all_used_structs.append(t)
                continue
        add_structs_from_struct(s)

        with open(os.path.join(parser_sources_dir, struct_name + ".cpp"), "w") as cpp:
            cpp.write("// SPDX-License-Identifier: GPL-3.0-only\n\n// This file was auto-generated.\n// If you want to edit this, edit the .json definitions and rerun the generator script, instead.\n\n")
            cpp.write("#define INVADER_DO_NOT_USE_EVERYTHING\n")
            cpp.write("#define USE_{}\n".format(struct_name))
            cpp.write("#include <invader/tag/hek/header.hpp>\n")
            cpp.write("extern \"C\" std::uint32_t crc32(std::uint32_t crc, const void *buf, std::size_t size) noexcept;\n")
            cpp.write("#include <invader/tag/parser/parser.hpp>\n")
            cpp.write("#include <invader/printf.hpp>\n")
            cpp.write("#include <invader/build/build_workload.hpp>\n")
            cpp.write("#include <invader/map/map.hpp>\n")
            cpp.write("#include <invader/map/tag.hpp>\n")
            cpp.write("#include <invader/file/file.hpp>\n")
            cpp.write("#include <invader/tag/hek/header.hpp>\n")
            cpp.write("namespace Invader::Parser {\n")
            make_cache_deformat(post_cache_deformat, all_used_structs, struct_name, hpp, cpp)
            make_cache_format_data(struct_name, s, pre_compile, post_compile, all_used_structs, hpp, cpp, all_enums, all_structs_arranged)
            make_parse_cache_file_data(post_cache_parse, all_used_structs, struct_name, hpp, cpp)
            make_cpp_save_hek_data(extract_hidden, all_used_structs, struct_name, hpp, cpp)
            make_parse_hek_tag_file(struct_name, hpp, cpp)
            make_parse_hek_tag_data(postprocess_hek_data, struct_name, all_used_structs, hpp, cpp)
            make_refactor_reference(all_used_structs, struct_name, hpp, cpp)
            make_parser_struct(cpp, all_enums, all_bitfields, all_used_structs, hpp, struct_name, extract_hidden, read_only, None if not "title" in s else s["title"])
            make_check_broken_enums(all_enums, all_used_structs, struct_name, hpp, cpp)
            make_check_invalid_references(all_used_structs, struct_name, hpp, cpp)
            make_check_invalid_indices(all_used_structs, struct_name, hpp, cpp, all_structs_arranged)
            make_check_invalid_ranges(all_used_structs, struct_name, hpp, cpp)
            cpp.write("}\n")

        hpp.write("        ~{}() override = default;\n".format(struct_name))

        if postprocess_hek_data:
            hpp.write("        void postprocess_hek_data();\n")

        if post_cache_deformat:
            hpp.write("        void post_cache_deformat();\n")

        if post_cache_parse:
            hpp.write("        void post_cache_parse(const Invader::Tag &, std::optional<HEK::Pointer>);\n")

        if pre_compile:
            hpp.write("        void pre_compile(BuildWorkload &workload, std::size_t tag_index, std::size_t struct_index, std::size_t offset);\n")

        if post_compile:
            hpp.write("        void post_compile(BuildWorkload &workload, std::size_t tag_index, std::size_t struct_index, std::size_t offset);\n")

        hpp.write("    };\n")
        hpp.write("\n\n    #endif\n\n")
    hpp.write("}\n")
    hpp.write("#endif\n")
    hpp.close()
