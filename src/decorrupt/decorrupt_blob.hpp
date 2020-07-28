// SPDX-License-Identifier: GPL-3.0-only

#ifndef INVADER__DECORRUPT__DECORRUPT_BLOB_HPP
#define INVADER__DECORRUPT__DECORRUPT_BLOB_HPP

#include <invader/hek/endian.hpp>
#include <invader/hek/pad.hpp>
#include <invader/hek/map.hpp>

namespace Invader::Decorrupt {
    struct DecorruptBlobHeader {
        enum DecorruptBlobMagic : std::uint32_t {
            DECORRUPT_BLOB_MAGIC = 0x4D455732
        };
        
        /** Blob magic */
        HEK::LittleEndian<DecorruptBlobMagic> blob_magic = DecorruptBlobMagic::DECORRUPT_BLOB_MAGIC;
        
        /** Version of the decorrupt blob format used */
        HEK::LittleEndian<std::uint32_t> version = 1;
        
        /** Cache file engine used */
        HEK::LittleEndian<HEK::CacheFileEngine> cache_file_engine;
        
        PAD(0x4);
        
        /** Number of indices in the blob */
        HEK::LittleEndian<std::uint64_t> index_count;
        
        /** Size of the blob when decompressed */
        HEK::LittleEndian<std::uint64_t> decompressed_size;
        
        /** Name of the map respective to this */
        HEK::TagString name;
        
        /** Build string of the tool used to generate the decorrupt blob */
        HEK::TagString blob_build;
        
        /** sha256sum of the map and blob XOR'd, not including the header */
        std::byte sha256_mapblob[0x20];
        
        PAD(0x780);
    };
    static_assert(sizeof(DecorruptBlobHeader) == 0x800);
    
    struct DecorruptBlobIndex {
        enum DataType : std::uint32_t {
            DATA_TYPE_TAG_DATA = 0,
            DATA_TYPE_BSP_DATA,
            DATA_TYPE_TAG_PATH
        };
        
        /** Data type */
        HEK::LittleEndian<DataType> data_type;
        
        /** Index of data (if tag data, it's just 0; if bsp data, it's the BSP index; if tag path, it's the tag index) */
        HEK::LittleEndian<std::uint32_t> data_index = 0;
        
        /** Offset of the data */
        HEK::LittleEndian<std::uint64_t> data_offset;
        
        /** Size of the data */
        HEK::LittleEndian<std::uint64_t> data_size;
    };
    
    static_assert(sizeof(DecorruptBlobIndex) == 0x18);
}

#endif
