#include "storage/wal_storage.h"

#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool parseCount(std::string_view text, uint64_t& count) {
    const auto result = std::from_chars(text.data(), text.data() + text.size(), count);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size() &&
           count > 0;
}

int runWriter(const std::string& directory, uint64_t count) {
    themis::WALStorage::Config config;
    config.dir = directory;
    config.rotation_threshold_bytes = 96;
    config.fsync_on_write = true;

    auto opened = themis::WALStorage::open(config);
    if (!opened) {
        std::cerr << "WAL open failed: " << opened.error().context() << '\n';
        return 2;
    }

    auto wal = std::move(opened.value());
    for (uint64_t index = 1; index <= count; ++index) {
        const auto key = "ack_" + std::to_string(index);
        const auto value = "value_" + std::to_string(index);
        std::cout << "APPEND_STARTED " << index << std::endl;
        auto appended = wal->appendPut(key, value);
        if (!appended) {
            std::cerr << "WAL append failed: " << appended.error().context() << '\n';
            return 3;
        }

        std::cout << "ACK " << index << ' ' << appended.value() << std::endl;
        std::string permission;
        if (!std::getline(std::cin, permission) || permission != "continue") {
            return 4;
        }
    }
    return 0;
}

int runVerifier(const std::string& directory, uint64_t expected_count) {
    themis::WALStorage::Config config;
    config.dir = directory;
    config.rotation_threshold_bytes = 96;
    config.fsync_on_write = true;

    std::vector<themis::WALStorage::Entry> recovered;
    auto opened = themis::WALStorage::open(
        config, [&recovered](const themis::WALStorage::Entry& entry) {
            recovered.push_back(entry);
            return true;
        });
    if (!opened) {
        std::cerr << "WAL recovery failed: " << opened.error().context() << '\n';
        return 5;
    }

    if (recovered.size() != expected_count) {
        std::cerr << "Expected " << expected_count << " recovered entries, got "
                  << recovered.size() << '\n';
        return 6;
    }

    for (uint64_t index = 1; index <= expected_count; ++index) {
        const auto& entry = recovered[static_cast<std::size_t>(index - 1)];
        if (entry.sequence != index ||
            entry.type != themis::WALStorage::EntryType::PUT ||
            entry.key != "ack_" + std::to_string(index) ||
            entry.value != "value_" + std::to_string(index)) {
            std::cerr << "Recovered entry mismatch at index " << index << '\n';
            return 7;
        }
    }

    std::cout << "VERIFIED " << recovered.size() << '\n';
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: wal_process_crash_worker <write|verify> <directory> <count>\n";
        return 1;
    }

    uint64_t count = 0;
    if (!parseCount(argv[3], count)) {
        std::cerr << "Count must be a positive integer\n";
        return 1;
    }

    const std::string mode(argv[1]);
    if (mode == "write") {
        return runWriter(argv[2], count);
    }
    if (mode == "verify") {
        return runVerifier(argv[2], count);
    }

    std::cerr << "Unknown mode: " << mode << '\n';
    return 1;
}
