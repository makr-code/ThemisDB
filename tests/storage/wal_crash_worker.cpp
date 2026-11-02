// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "storage/wal_storage.h"

#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 4) {
        return 2;
    }

    themis::WALStorage::Config config;
    config.dir = argv[1];
    config.rotation_threshold_bytes = std::stoull(argv[2]);
    config.fsync_on_write = true;

    auto opened = themis::WALStorage::open(config);
    if (!opened) {
        std::cerr << opened.error().context() << '\n';
        return 1;
    }

    const auto checkpoint_after = std::stoull(argv[3]);
    std::cout << "READY\n" << std::flush;

    for (uint64_t sequence = 1; sequence <= 8; ++sequence) {
        const auto key = "key_" + std::to_string(sequence);
        const auto value = "value_" + std::to_string(sequence);
        auto appended = (*opened)->appendPut(key, value);
        if (!appended) {
            std::cerr << appended.error().context() << '\n';
            return 1;
        }

        std::cout << "ACK " << sequence << '\n' << std::flush;
        char command = '\0';
        if (!std::cin.get(command) || command != 'C') {
            return 0;
        }

        if (sequence == checkpoint_after) {
            auto checkpoint = (*opened)->checkpoint(false);
            if (!checkpoint) {
                std::cerr << checkpoint.error().context() << '\n';
                return 1;
            }

            std::cout << "CHECKPOINT " << sequence << '\n' << std::flush;
            if (!std::cin.get(command) || command != 'C') {
                return 0;
            }
        }
    }

    return 0;
}
