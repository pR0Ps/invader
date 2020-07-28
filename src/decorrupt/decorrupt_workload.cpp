// SPDX-License-Identifier: GPL-3.0-only

#include <invader/decorrupt/decorrupt_workload.hpp>
#include <invader/printf.hpp>
#include <zstd.h>
#include "decorrupt_blob.hpp"

using namespace Invader::Decorrupt;

DecorruptWorkload::DecorruptWorkload() : ErrorHandler() {}

std::vector<std::byte> DecorruptWorkload::decorrupt_cache_file(const std::vector<std::byte> &, DecorruptWorkload::DecorruptPaths, DecorruptWorkload::InferHiddenData, DecorruptWorkload::DecorruptClasses) {
    eprintf_error("decorrupt_cache_file() is unimplemented");
    std::terminate();
}

std::vector<std::byte> DecorruptWorkload::decompress_decorrupted_file(const std::vector<std::byte> &blob) {
    // Go through the process of making sure the decorrupt blob is valid, first
    if(blob.size() < sizeof(DecorruptBlobHeader)) {
        eprintf_error("blob header size is too small to even fit a header (%zu < %zu)", blob.size(), sizeof(DecorruptBlobHeader));
        throw InvalidDecorruptionBlobException();
    }
    
    const auto &header = *reinterpret_cast<const DecorruptBlobHeader *>(blob.data());
    if(header.blob_magic != DecorruptBlobHeader::DecorruptBlobMagic::DECORRUPT_BLOB_MAGIC) {
        eprintf_error("blob header magic is not valid %zu", sizeof(header));
        throw InvalidDecorruptionBlobException();
    }
    
    if(header.version != DecorruptBlobHeader::DecorruptBlobVersion::DECORRUPT_DEFAULT_BLOB_VERSION) {
        eprintf_error("blob header version mismatch: %zu != %zu (expected)", static_cast<std::size_t>(header.version.read()), static_cast<std::size_t>(DecorruptBlobHeader::DecorruptBlobVersion::DECORRUPT_DEFAULT_BLOB_VERSION));
        eprintf("If this was made with a different version of Invader, then you will need to\neither use that version or regenerate the blob.\n");
        throw InvalidDecorruptionBlobException();
    }
    
    if(header.decompressed_size < sizeof(DecorruptBlobHeader)) {
        eprintf_error("blob header decorrupt size is too small to even fit the header (%zu < %zu)", static_cast<std::size_t>(header.decompressed_size.read()), sizeof(header));
        throw InvalidDecorruptionBlobException();
    }
    
    // Copy in the header
    std::vector<std::byte> decompressed_blob(header.decompressed_size.read());
    std::memcpy(decompressed_blob.data(), &header, sizeof(header));
    
    // Begin work
    const std::byte *input = blob.data() + sizeof(header);
    std::size_t input_size = blob.size() - sizeof(header);
    std::byte *output = decompressed_blob.data() + sizeof(header);
    std::size_t output_size = decompressed_blob.size() - sizeof(header);
    auto decompressed_size = ZSTD_decompress(output, output_size, input, input_size);
    
    if(ZSTD_isError(decompressed_size)) {
        eprintf_error("error on decompression of the blob - invalid zstd data maybe?");
        throw InvalidDecorruptionBlobException();
    }
    
    if(output_size != decompressed_size) {
        eprintf_error("blob decompressed data does not match the expected size (%zu != %zu)", static_cast<std::size_t>(decompressed_size), output_size);
        throw InvalidDecorruptionBlobException();
    }
    
    // Done!
    return decompressed_blob;
}
