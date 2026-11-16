// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "replication/replication_manager.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        return 2;
    }

    themisdb::replication::ReplicationConfig config;
    config.wal_directory = argv[1];
    config.wal_sync_on_commit = true;
    config.wal_segment_size_bytes = 1;

    try {
        themisdb::replication::WALManager wal(config);
        std::cout << "READY\n" << std::flush;
        std::cout << "TERM " << wal.incrementTerm() << '\n' << std::flush;

        char command = '\0';
        if (!std::cin.get(command) || command != 'C') {
            return 0;
        }

        for (uint64_t index = 1; index <= 2; ++index) {
            themisdb::replication::WALEntry entry{};
            entry.operation = "INSERT";
            entry.collection = "crash_test";
            entry.document_id = "entry-" + std::to_string(index);
            entry.data = "durable-payload-" + std::to_string(index);

            const auto sequence = wal.appendDurably(entry);
            std::cout << "ACK " << sequence << '\n' << std::flush;

            command = '\0';
            if (!std::cin.get(command) || command != 'C') {
                return 0;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
