// SPDX-License-Identifier: GPL-3.0-only

#ifndef INVADER__DECORRUPT__DECORRUPT_WORKLOAD_HPP
#define INVADER__DECORRUPT__DECORRUPT_WORKLOAD_HPP

#include "../error_handler/error_handler.hpp"

namespace Invader::Decorrupt {
    class DecorruptWorkload : public ErrorHandler {
    public:
        enum DecorruptPaths {
            /** Do not decorrupt paths */
            DECORRUPT_PATHS_NONE = 0,
            
            /** Create simple paths that can at least be extracted */
            DECORRUPT_PATHS_BASIC,
            
            /** Perform full decorruption on paths, producing organized and useful tag paths */
            DECORRUPT_PATHS_FULL
        };
        
        enum InferHiddenData {
            /** Do not infer hidden data */
            INFER_HIDDEN_DATA_NONE = 0,
            
            /** Infer hidden data and perform a reverse recalculation of non-hidden data based on it */
            INFER_HIDDEN_DATA_FULL
        };
        
        enum DecorruptClasses {
            /** Do not decorrupt classes */
            DECORRUPT_CLASSES_NONE = 0,
            
            /** Decorrupt classes by inferring them based on dependencies */
            DECORRUPT_CLASSES_FULL
        };
        
        /**
         * Decorrupt a cache file, producing a compressed blob
         * @param map          map to decorrupt
         * @param path_setting path setting
         * @param hidden_data  hidden data setting
         * @param classes      classes setting
         * @return             compressed blob of the decorrupted map
         */
        static std::vector<std::byte> decorrupt_cache_file(const std::vector<std::byte> &map, DecorruptPaths path_setting, InferHiddenData hidden_data, DecorruptClasses classes);
        
        /**
         * Decompress a compressed blob
         * @param blob compressed blob
         * @return     decompressed version of the blob
         */
        static std::vector<std::byte> decompress_decorrupted_file(const std::vector<std::byte> &blob);
        
        ~DecorruptWorkload() override = default;
    private:
        DecorruptWorkload();
    };
}

#endif
