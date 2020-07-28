// SPDX-License-Identifier: GPL-3.0-only

#include <invader/decorrupt/decorrupt_workload.hpp>
#include <invader/printf.hpp>

using namespace Invader::Decorrupt;

DecorruptWorkload::DecorruptWorkload() : ErrorHandler() {}

std::vector<std::byte> DecorruptWorkload::decorrupt_cache_file(const std::vector<std::byte> &, DecorruptWorkload::DecorruptPaths, DecorruptWorkload::InferHiddenData, DecorruptWorkload::DecorruptClasses) {
    eprintf_error("decorrupt_cache_file() is unimplemented");
    std::terminate();
}

std::vector<std::byte> DecorruptWorkload::decompress_decorrupted_file(const std::vector<std::byte> &) {
    eprintf_error("decompress_decorrupted_file() is unimplemented");
    std::terminate();
}
