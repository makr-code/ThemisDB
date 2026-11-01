// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <system_error>
#include <thread>

namespace themis::bench {

inline bool removeBenchmarkDirectory(const std::filesystem::path& path,
                                     std::error_code& error) {
    constexpr size_t cleanup_attempts = 6;
    for (size_t attempt = 0; attempt < cleanup_attempts; ++attempt) {
        error.clear();
        std::filesystem::remove_all(path, error);
        if (!error) {
            return true;
        }
        if (attempt + 1 < cleanup_attempts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10 * (1 << attempt)));
        }
    }
    return false;
}

} // namespace themis::bench