/**
 * @file raft_log.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_log.h"
#include <stdexcept>
#include "utils/zstd_codec.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string_view>
#include <cstdlib>
#include <limits>
#include <system_error>
#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <cerrno>
#  include <fcntl.h>
#  include <sys/file.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace themisdb {
namespace sharding {

// ============================================================================
// TIMEOUT CONFIGURATION FOR CONSENSUS OPERATIONS
// ============================================================================
// Environment variable: THEMIS_SHARDING_CONSENSUS_TIMEOUT_MS
// Default: 10000 ms (10 seconds) - consensus ops must complete or timeout quickly
// This prevents indefinite blocking during network partitions or node failures.
// ============================================================================

namespace {
    std::string raftJournalChecksum(std::string_view payload) {
        std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
        SHA256(reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), hash.data());
        std::ostringstream result;
        result << std::hex << std::setfill('0');
        for (const auto byte : hash) {
            result << std::setw(2) << static_cast<unsigned int>(byte);
        }
        return result.str();
    }

    std::string formatRaftJournalRecord(const std::string& payload) {
        return payload + "|" + raftJournalChecksum(payload) + "\n";
    }

    std::string encodeJournalField(std::string_view value) {
        std::ostringstream encoded;
        encoded << std::hex << std::setfill('0');
        for (const unsigned char byte : value) {
            encoded << std::setw(2) << static_cast<unsigned int>(byte);
        }
        return encoded.str();
    }

    std::string decodeJournalField(std::string_view value) {
        if (value.size() % 2 != 0) {
            throw std::runtime_error("Raft journal contains malformed hex data");
        }
        std::string decoded;
        decoded.reserve(value.size() / 2);
        for (std::size_t offset = 0; offset < value.size(); offset += 2) {
            unsigned int byte = 0;
            std::istringstream parser(std::string(value.substr(offset, 2)));
            parser >> std::hex >> byte;
            if (!parser || !parser.eof() || byte > 0xff) {
                throw std::runtime_error("Raft journal contains malformed hex data");
            }
            decoded.push_back(static_cast<char>(byte));
        }
        return decoded;
    }

    std::vector<std::string_view> splitJournalFields(std::string_view record) {
        std::vector<std::string_view> fields;
        std::size_t begin = 0;
        while (true) {
            const auto delimiter = record.find('|', begin);
            fields.emplace_back(record.substr(
                begin, delimiter == std::string_view::npos ? delimiter : delimiter - begin));
            if (delimiter == std::string_view::npos) {
                break;
            }
            begin = delimiter + 1;
        }
        return fields;
    }

    uint64_t parseJournalInteger(std::string_view field) {
        uint64_t value = 0;
        const auto [end, error] = std::from_chars(field.data(), field.data() + field.size(), value);
        if (error != std::errc{} || end != field.data() + field.size()) {
            throw std::runtime_error("Raft journal contains an invalid integer");
        }
        return value;
    }

    bool syncDirectory(const std::filesystem::path& directory_path,
                       std::string& error) {
#ifdef _WIN32
        // FlushFileBuffers is applied to journal/snapshot files on Windows;
        // directory handles do not expose a portable equivalent of POSIX fsync.
        (void)directory_path;
        (void)error;
        return true;
#else
        const auto path = directory_path.empty() ? std::filesystem::path(".") : directory_path;
        int flags = O_RDONLY;
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
#ifdef O_DIRECTORY
        flags |= O_DIRECTORY;
#endif
        const int directory = ::open(path.c_str(), flags);
        if (directory < 0) {
            error = "cannot open directory for durable sync: " +
                    std::error_code(errno, std::generic_category()).message();
            return false;
        }
        const bool synced = ::fsync(directory) == 0;
        const int sync_error = errno;
        const int close_result = ::close(directory);
        if (!synced) {
            error = "cannot sync directory entry: " +
                    std::error_code(sync_error, std::generic_category()).message();
            return false;
        }
        if (close_result != 0) {
            error = "cannot close directory after sync: " +
                    std::error_code(errno, std::generic_category()).message();
            return false;
        }
        return true;
#endif
    }

    bool createDirectoriesDurably(const std::filesystem::path& directory_path,
                                  std::string& error) {
        if (directory_path.empty()) {
            return true;
        }
        std::vector<std::filesystem::path> missing_directories;
        auto current = directory_path;
        while (!current.empty()) {
            std::error_code exists_error;
            if (std::filesystem::exists(current, exists_error)) {
                if (exists_error || !std::filesystem::is_directory(current, exists_error) ||
                    exists_error) {
                    error = "Raft persistence parent is not an accessible directory";
                    return false;
                }
                break;
            }
            if (exists_error) {
                error = "cannot inspect Raft persistence directory: " +
                        exists_error.message();
                return false;
            }
            missing_directories.push_back(current);
            const auto parent = current.parent_path();
            if (parent == current) {
                break;
            }
            current = parent;
        }

        std::error_code create_error;
        std::filesystem::create_directories(directory_path, create_error);
        if (create_error) {
            error = "cannot create Raft persistence directory: " + create_error.message();
            return false;
        }
        for (auto it = missing_directories.rbegin(); it != missing_directories.rend(); ++it) {
            if (!syncDirectory(it->parent_path(), error)) {
                return false;
            }
        }
        return true;
    }

    bool writeDurableTemporaryFile(const std::filesystem::path& path,
                                   std::string_view contents,
                                   std::string& error) {
#ifdef _WIN32
        HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            error = "cannot create temporary Raft journal";
            return false;
        }
        std::size_t offset = 0;
        while (offset < contents.size()) {
            const auto chunk = static_cast<DWORD>(
                std::min<std::size_t>(contents.size() - offset, MAXDWORD));
            DWORD written = 0;
            if (!WriteFile(file, contents.data() + offset, chunk, &written, nullptr) ||
                written == 0) {
                error = "cannot write temporary Raft journal";
                CloseHandle(file);
                return false;
            }
            offset += written;
        }
        if (!FlushFileBuffers(file)) {
            error = "cannot flush temporary Raft journal";
            CloseHandle(file);
            return false;
        }
        if (!CloseHandle(file)) {
            error = "cannot close temporary Raft journal";
            return false;
        }
#else
        int flags = O_WRONLY | O_CREAT | O_EXCL;
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
#ifdef O_NOFOLLOW
        flags |= O_NOFOLLOW;
#endif
        const int file = ::open(path.c_str(), flags, 0600);
        if (file < 0) {
            error = "cannot create temporary Raft journal: " +
                    std::error_code(errno, std::generic_category()).message();
            return false;
        }
        std::size_t offset = 0;
        while (offset < contents.size()) {
            const auto write_size = std::min<std::size_t>(
                contents.size() - offset,
                static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
            const auto written = ::write(file, contents.data() + offset, write_size);
            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                error = "cannot write temporary Raft journal: " +
                        std::error_code(errno, std::generic_category()).message();
                ::close(file);
                return false;
            }
            if (written == 0) {
                error = "cannot write temporary Raft journal: write returned zero bytes";
                ::close(file);
                return false;
            }
            offset += static_cast<std::size_t>(written);
        }
        if (::fsync(file) != 0) {
            error = "cannot sync temporary Raft journal: " +
                    std::error_code(errno, std::generic_category()).message();
            ::close(file);
            return false;
        }
        if (::close(file) != 0) {
            error = "cannot close temporary Raft journal: " +
                    std::error_code(errno, std::generic_category()).message();
            return false;
        }
#endif
        return true;
    }

    bool appendAndSyncFile(const std::filesystem::path& path,
                           std::string_view record,
                           std::string& error) {
        if (!createDirectoriesDurably(path.parent_path(), error)) {
            return false;
        }
#ifdef _WIN32
        HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            error = "cannot open Raft journal: " +
                    std::error_code(static_cast<int>(GetLastError()),
                                    std::system_category()).message();
            return false;
        }
        LARGE_INTEGER end{};
        if (!SetFilePointerEx(file, end, nullptr, FILE_END)) {
            error = "cannot seek Raft journal: " +
                    std::error_code(static_cast<int>(GetLastError()),
                                    std::system_category()).message();
            CloseHandle(file);
            return false;
        }
        std::size_t offset = 0;
        while (offset < record.size()) {
            const auto chunk = static_cast<DWORD>(
                std::min<std::size_t>(record.size() - offset, MAXDWORD));
            DWORD written = 0;
            if (!WriteFile(file, record.data() + offset, chunk, &written, nullptr) ||
                written == 0) {
                error = "failed writing Raft journal: " +
                        std::error_code(static_cast<int>(GetLastError()),
                                        std::system_category()).message();
                CloseHandle(file);
                return false;
            }
            offset += written;
        }
        if (!FlushFileBuffers(file)) {
            error = "failed flushing Raft journal: " +
                    std::error_code(static_cast<int>(GetLastError()),
                                    std::system_category()).message();
            CloseHandle(file);
            return false;
        }
        if (!CloseHandle(file)) {
            error = "failed closing Raft journal: " +
                    std::error_code(static_cast<int>(GetLastError()),
                                    std::system_category()).message();
            return false;
        }
#else
        int flags = O_WRONLY | O_APPEND | O_CREAT;
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
#ifdef O_NOFOLLOW
        flags |= O_NOFOLLOW;
#endif
        const int file = ::open(path.c_str(), flags, 0600);
        if (file < 0) {
            error = "cannot open Raft journal: " +
                    std::error_code(errno, std::generic_category()).message();
            return false;
        }
        struct stat status {};
        if (::fstat(file, &status) != 0 || !S_ISREG(status.st_mode)) {
            error = "Raft journal path is not a regular file";
            ::close(file);
            return false;
        }
        std::size_t offset = 0;
        while (offset < record.size()) {
            const auto written = ::write(file, record.data() + offset, record.size() - offset);
            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                error = "failed writing Raft journal: " +
                        std::error_code(errno, std::generic_category()).message();
                ::close(file);
                return false;
            }
            if (written == 0) {
                error = "failed writing Raft journal: write returned zero bytes";
                ::close(file);
                return false;
            }
            offset += static_cast<std::size_t>(written);
        }
        if (::fsync(file) != 0) {
            error = "failed syncing Raft journal: " +
                    std::error_code(errno, std::generic_category()).message();
            ::close(file);
            return false;
        }
        if (::close(file) != 0) {
            error = "failed closing Raft journal: " +
                    std::error_code(errno, std::generic_category()).message();
            return false;
        }
        if (!syncDirectory(path.parent_path(), error)) {
            return false;
        }
#endif
        return true;
    }

    bool installDurableSnapshot(const std::filesystem::path& temporary_path,
                                const std::filesystem::path& snapshot_path,
                                std::string& error) {
        if (!createDirectoriesDurably(snapshot_path.parent_path(), error)) {
            return false;
        }
#ifdef _WIN32
        HANDLE file = CreateFileW(temporary_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            error = "cannot open Raft snapshot for durable flush";
            return false;
        }
        const bool flushed = FlushFileBuffers(file) != 0;
        const int flush_error = static_cast<int>(GetLastError());
        CloseHandle(file);
        if (!flushed) {
            error = "cannot flush Raft snapshot: " +
                    std::error_code(flush_error, std::system_category()).message();
            return false;
        }
        if (!MoveFileExW(temporary_path.c_str(), snapshot_path.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            error = "cannot atomically install Raft snapshot: " +
                    std::error_code(static_cast<int>(GetLastError()),
                                    std::system_category()).message();
            return false;
        }
#else
        int flags = O_RDONLY;
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
#ifdef O_NOFOLLOW
        flags |= O_NOFOLLOW;
#endif
        const int file = ::open(temporary_path.c_str(), flags);
        if (file < 0) {
            error = "cannot open Raft snapshot for durable sync: " +
                    std::error_code(errno, std::generic_category()).message();
            return false;
        }
        struct stat status {};
        if (::fstat(file, &status) != 0 || !S_ISREG(status.st_mode)) {
            error = "Raft snapshot path is not a regular file";
            ::close(file);
            return false;
        }
        if (::fsync(file) != 0) {
            error = "cannot sync Raft snapshot: " +
                    std::error_code(errno, std::generic_category()).message();
            ::close(file);
            return false;
        }
        if (::close(file) != 0) {
            error = "cannot close Raft snapshot after sync: " +
                    std::error_code(errno, std::generic_category()).message();
            return false;
        }
        std::error_code rename_error;
        std::filesystem::rename(temporary_path, snapshot_path, rename_error);
        if (rename_error) {
            error = "cannot atomically install Raft snapshot: " + rename_error.message();
            return false;
        }
        if (!syncDirectory(snapshot_path.parent_path(), error)) {
            return false;
        }
#endif
        return true;
    }

    /**
     * @brief Get Consensus Timeout.
     * @return Return value.
     * @details Calls: std::getenv(), std::stol(), std::chrono::milliseconds(), spdlog::warn().
     */
    std::chrono::milliseconds getConsensusTimeout() {
        const char* env_timeout = std::getenv("THEMIS_SHARDING_CONSENSUS_TIMEOUT_MS");
        if (env_timeout) {
            try {
                long timeout_ms = std::stol(env_timeout);
                if (timeout_ms > 0 && timeout_ms <= 120000) {  // 2 minute max
                    return std::chrono::milliseconds(timeout_ms);
                }
            } catch (...) {
                spdlog::warn("Invalid THEMIS_SHARDING_CONSENSUS_TIMEOUT_MS: {}; using default", env_timeout);
            }
        }
        return std::chrono::milliseconds(10000);  // Default: 10 seconds
    }
}

RaftLog::RaftLog(const std::string& persistence_path)
    : commit_index_(0), persistence_path_(persistence_path) {
    if (!persistence_path_.empty()) {
        try {
            acquirePersistenceLock();
            replayJournal();
        } catch (...) {
            releasePersistenceLock();
            throw;
        }
    }
}

RaftLog::~RaftLog() {
    releasePersistenceLock();
}

void RaftLog::acquirePersistenceLock() {
    const std::filesystem::path journal_path(persistence_path_);
    std::string directory_error;
    if (!createDirectoriesDurably(journal_path.parent_path(), directory_error)) {
        throw std::runtime_error(directory_error);
    }
    const auto lock_path = std::filesystem::path(persistence_path_ + ".lock");
#ifdef _WIN32
    HANDLE lock_file = CreateFileW(lock_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                                   nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (lock_file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Raft journal is already owned or its lock cannot be opened");
    }
    persistence_lock_handle_ = lock_file;
#else
    int flags = O_CREAT | O_RDWR;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
    persistence_lock_fd_ = ::open(lock_path.c_str(), flags, 0600);
    if (persistence_lock_fd_ < 0) {
        throw std::runtime_error("cannot open Raft journal lock: " +
                                 std::error_code(errno, std::generic_category()).message());
    }
    if (::flock(persistence_lock_fd_, LOCK_EX | LOCK_NB) != 0) {
        const int lock_error = errno;
        releasePersistenceLock();
        throw std::runtime_error("Raft journal is already owned: " +
                                 std::error_code(lock_error, std::generic_category()).message());
    }
#endif
}

void RaftLog::releasePersistenceLock() noexcept {
#ifdef _WIN32
    if (persistence_lock_handle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(persistence_lock_handle_));
        persistence_lock_handle_ = nullptr;
    }
#else
    if (persistence_lock_fd_ >= 0) {
        ::close(persistence_lock_fd_);
        persistence_lock_fd_ = -1;
    }
#endif
}

void RaftLog::appendDurableRecord(const std::string& payload) {
    if (persistence_path_.empty()) {
        return;
    }
    if (!persistence_healthy_) {
        throw std::runtime_error("Raft persistence is unhealthy");
    }
    const std::string record = formatRaftJournalRecord(payload);
    std::string error;
    if (!appendAndSyncFile(persistence_path_, record, error)) {
        persistence_healthy_ = false;
        throw std::runtime_error(error);
    }
}

void RaftLog::replayJournal() {
    std::ifstream input(persistence_path_, std::ios::binary);
    if (!input) {
        if (std::filesystem::exists(persistence_path_)) {
            persistence_healthy_ = false;
            throw std::runtime_error("cannot read existing Raft journal");
        }
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (input.eof()) {
            persistence_healthy_ = false;
            throw std::runtime_error("Raft journal ends with an incomplete record");
        }
        const auto checksum_separator = line.rfind('|');
        if (checksum_separator == std::string::npos ||
            line.substr(checksum_separator + 1) !=
                raftJournalChecksum(std::string_view(line).substr(0, checksum_separator))) {
            persistence_healthy_ = false;
            throw std::runtime_error("Raft journal checksum validation failed");
        }
        const auto fields = splitJournalFields(
            std::string_view(line).substr(0, checksum_separator));
        if (fields.empty()) {
            persistence_healthy_ = false;
            throw std::runtime_error("Raft journal contains an empty record");
        }

        if (fields[0] == "B" && fields.size() == 3) {
            const uint64_t index = parseJournalInteger(fields[1]);
            const uint64_t term = parseJournalInteger(fields[2]);
            if (!log_.empty() || snapshot_index_ != 0 || commit_index_ != 0 ||
                persisted_hard_state_present_) {
                persistence_healthy_ = false;
                throw std::runtime_error("Raft journal base record is out of order");
            }
            snapshot_index_ = index;
            snapshot_term_ = term;
            commit_index_ = index;
        } else if (fields[0] == "E" && fields.size() == 5) {
            LogEntry entry;
            entry.term = parseJournalInteger(fields[1]);
            entry.index = parseJournalInteger(fields[2]);
            entry.timestamp_ns = parseJournalInteger(fields[3]);
            entry.command = decodeJournalField(fields[4]);
            const uint64_t last_index = log_.empty() ? snapshot_index_ : log_.rbegin()->first;
            if (entry.index != last_index + 1) {
                persistence_healthy_ = false;
                throw std::runtime_error("Raft journal log indexes are not contiguous");
            }
            log_.emplace(entry.index, std::move(entry));
        } else if (fields[0] == "T" && fields.size() == 2) {
            const uint64_t index = parseJournalInteger(fields[1]);
            const uint64_t last_index = log_.empty() ? snapshot_index_ : log_.rbegin()->first;
            if (index <= commit_index_ || index > last_index + 1) {
                persistence_healthy_ = false;
                throw std::runtime_error("Raft journal truncation boundary is invalid");
            }
            log_.erase(log_.lower_bound(index), log_.end());
        } else if (fields[0] == "C" && fields.size() == 2) {
            const uint64_t index = parseJournalInteger(fields[1]);
            const uint64_t last_index = log_.empty() ? snapshot_index_ : log_.rbegin()->first;
            if (index < commit_index_ || index > last_index) {
                persistence_healthy_ = false;
                throw std::runtime_error("Raft journal commit index is invalid");
            }
            commit_index_ = index;
        } else if ((fields[0] == "S" || fields[0] == "K") && fields.size() == 3) {
            const uint64_t index = parseJournalInteger(fields[1]);
            const uint64_t term = parseJournalInteger(fields[2]);
            const auto snapshot_entry = log_.find(index);
            const bool has_matching_entry =
                index == snapshot_index_ ? term == snapshot_term_
                : snapshot_entry != log_.end() && snapshot_entry->second.term == term;
            if (index < snapshot_index_ || index > commit_index_ || !has_matching_entry) {
                persistence_healthy_ = false;
                throw std::runtime_error("Raft journal snapshot boundary is invalid");
            }
            snapshot_index_ = index;
            snapshot_term_ = term;
            if (fields[0] == "K") {
                log_.erase(log_.begin(), log_.upper_bound(index));
            }
        } else if (fields[0] == "H" && fields.size() == 3) {
            const uint64_t term = parseJournalInteger(fields[1]);
            const std::string vote = decodeJournalField(fields[2]);
            if (term < persisted_term_) {
                persistence_healthy_ = false;
                throw std::runtime_error("Raft journal hard-state term regressed");
            }
            if (term == persisted_term_ && !persisted_vote_.empty() &&
                vote != persisted_vote_) {
                persistence_healthy_ = false;
                throw std::runtime_error("Raft journal contains conflicting votes in one term");
            }
            persisted_term_ = term;
            persisted_vote_ = vote;
            persisted_hard_state_present_ = true;
        } else {
            persistence_healthy_ = false;
            throw std::runtime_error("Raft journal contains an unknown record");
        }
    }
    if (input.bad()) {
        persistence_healthy_ = false;
        throw std::runtime_error("failed while reading Raft journal");
    }
}

bool RaftLog::isPersistenceHealthy() const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return persistence_healthy_;
}

std::pair<uint64_t, std::string> RaftLog::getHardState() const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return {persisted_term_, persisted_vote_};
}

void RaftLog::persistHardState(uint64_t term, const std::string& voted_for) {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    if (term < persisted_term_) {
        throw std::runtime_error("Raft hard-state term cannot regress");
    }
    if (term == persisted_term_ && !persisted_vote_.empty() &&
        voted_for != persisted_vote_) {
        throw std::runtime_error("Raft node cannot change its vote within one term");
    }
    if (!persistence_path_.empty()) {
        appendDurableRecord("H|" + std::to_string(term) + "|" + encodeJournalField(voted_for));
        persisted_hard_state_present_ = true;
    }
    persisted_term_ = term;
    persisted_vote_ = voted_for;
}

void RaftLog::compactJournal() {
    if (persistence_path_.empty()) {
        return;
    }
    if (!persistence_healthy_) {
        throw std::runtime_error("Raft persistence is unhealthy");
    }

    std::string contents;
    if (snapshot_index_ > 0) {
        contents += formatRaftJournalRecord(
            "B|" + std::to_string(snapshot_index_) + "|" + std::to_string(snapshot_term_));
    }
    if (persisted_hard_state_present_) {
        contents += formatRaftJournalRecord(
            "H|" + std::to_string(persisted_term_) + "|" +
            encodeJournalField(persisted_vote_));
    }
    for (const auto& [index, entry] : log_) {
        contents += formatRaftJournalRecord(
            "E|" + std::to_string(entry.term) + "|" + std::to_string(index) + "|" +
            std::to_string(entry.timestamp_ns) + "|" + encodeJournalField(entry.command));
    }
    if (commit_index_ > 0) {
        contents += formatRaftJournalRecord("C|" + std::to_string(commit_index_));
    }

    const auto temporary_path = std::filesystem::path(
        persistence_path_ + ".compact-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::string write_error;
    if (!writeDurableTemporaryFile(temporary_path, contents, write_error)) {
        std::error_code remove_error;
        std::filesystem::remove(temporary_path, remove_error);
        persistence_healthy_ = false;
        throw std::runtime_error(write_error);
    }

    std::string install_error;
    if (!installDurableSnapshot(temporary_path, persistence_path_, install_error)) {
        std::error_code remove_error;
        std::filesystem::remove(temporary_path, remove_error);
        persistence_healthy_ = false;
        throw std::runtime_error("cannot install compacted Raft journal: " + install_error);
    }
}

/**
 * @brief Append.
 * @param[in] entry Input parameter.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: lock(), getConsensusTimeout(), try_lock_for(), std::to_string(), count(), spdlog::error().
 */
uint64_t RaftLog::append(const LogEntry& entry) {
    // TIMEOUT ENFORCEMENT: Use try_lock_for to prevent indefinite blocking
    // during consensus operations. If timeout expires, throw exception to
    // allow caller to handle consensus abort.
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        std::string error_msg = "RaftLog::append timeout after " + 
                               std::to_string(timeout.count()) + "ms";
        spdlog::error("{}", error_msg);
        throw std::runtime_error(error_msg);
    }
    
    const uint64_t last_index = log_.empty() ? snapshot_index_ : log_.rbegin()->first;
    if (!persistence_path_.empty() &&
        (entry.index <= snapshot_index_ || entry.index != last_index + 1)) {
        throw std::runtime_error("persistent Raft log append must extend the current tail");
    }
    if (!persistence_path_.empty()) {
        appendDurableRecord("E|" + std::to_string(entry.term) + "|" +
                            std::to_string(entry.index) + "|" +
                            std::to_string(entry.timestamp_ns) + "|" +
                            encodeJournalField(entry.command));
    }
    log_[entry.index] = entry;
    return entry.index;
}

std::optional<LogEntry> RaftLog::getEntry(uint64_t index) const {
    /**
     * @brief TIMEOUT ENFORCEMENT: Use try_lock_for for consistent timeout behavior
     * @param[in] mutex_ Input parameter.
     * @param[in] defer_lock Input parameter.
     * @return Return value.
     */
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::getEntry timeout after {}ms retrieving index {}",
                     timeout.count(), index);
        return std::nullopt;
    }
    
    auto it = log_.find(index);
    if (it != log_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<LogEntry> RaftLog::getEntries(uint64_t start_index, uint64_t end_index) const {
    /**
     * @brief TIMEOUT ENFORCEMENT: Batch read operation with timeout
     * @param[in] mutex_ Input parameter.
     * @param[in] defer_lock Input parameter.
     * @return Return value.
     */
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::getEntries timeout after {}ms retrieving range [{}, {}]",
                     timeout.count(), start_index, end_index);
        return {};  // Return empty vector on timeout
    }
    
    std::vector<LogEntry> entries;
    
    // FIXED: Add bounds validation to prevent unbounded iteration and data races
    if (start_index > end_index) {
        return entries;  // Invalid range
    }
    
    // Cap iteration to prevent memory exhaustion
    uint64_t max_entries = 10000;
    if (end_index - start_index + 1 > max_entries) {
        end_index = start_index + max_entries - 1;
        spdlog::warn("RaftLog::getEntries range capped at {} entries", max_entries);
    }
    
    for (uint64_t i = start_index; i <= end_index; ++i) {
        auto it = log_.find(i);
        if (it != log_.end()) {
            entries.push_back(it->second);
        } else {
            break;  // Stop at first missing entry
        }
    }
    
    return entries;
}

bool RaftLog::hasEntry(uint64_t index, uint64_t term) const {
    /**
     * @brief TIMEOUT ENFORCEMENT: Consensus query with timeout
     * @param[in] mutex_ Input parameter.
     * @param[in] defer_lock Input parameter.
     * @return Return value.
     */
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::hasEntry timeout after {}ms checking index={} term={}",
                     timeout.count(), index, term);
        return false;
    }
    
    // Special case: index 0 always matches (no previous entry)
    if (index == 0) {
        return true;
    }

    // If the index is exactly the snapshot boundary, match against snapshot term
    if (index == snapshot_index_) {
        return term == snapshot_term_;
    }
    
    auto it = log_.find(index);
    if (it == log_.end()) {
        return false;
    }
    
    return it->second.term == term;
}

/**
 * @brief Truncate From.
 * @param[in] index Input parameter.
 * @details Calls: lock(), getConsensusTimeout(), try_lock_for(), spdlog::error(), count(), lower_bound(), erase(), end().
 */
void RaftLog::truncateFrom(uint64_t index) {
    // TIMEOUT ENFORCEMENT: Consensus write with timeout
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::truncateFrom timeout after {}ms truncating from index={}",
                     timeout.count(), index);
        if (!persistence_path_.empty()) {
            throw std::runtime_error("persistent Raft log truncation timed out");
        }
        return;  // Log truncation timeout - caller should handle
    }
    
    // Erase all entries from index onward
    if (!persistence_path_.empty()) {
        if (index <= commit_index_) {
            throw std::runtime_error("persistent Raft log cannot truncate committed entries");
        }
        const uint64_t last_index = log_.empty() ? snapshot_index_ : log_.rbegin()->first;
        if (index > last_index + 1) {
            throw std::runtime_error("persistent Raft truncation boundary exceeds the log tail");
        }
        appendDurableRecord("T|" + std::to_string(index));
    }
    auto it = log_.lower_bound(index);
    log_.erase(it, log_.end());
    if (persistence_path_.empty() && commit_index_ >= index) {
        commit_index_ = index > 0 ? index - 1 : 0;
    }
}

/**
 * @brief Set Commit Index.
 * @param[in] index Input parameter.
 * @details Calls: lock(), getConsensusTimeout(), try_lock_for(), spdlog::error(), count(), spdlog::warn(), empty(), rbegin().
 */
void RaftLog::setCommitIndex(uint64_t index) {
    // TIMEOUT ENFORCEMENT: Consensus commit update with timeout
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::setCommitIndex timeout after {}ms setting index={}",
                     timeout.count(), index);
        if (!persistence_path_.empty()) {
            throw std::runtime_error("persistent Raft commit update timed out");
        }
        return;
    }
    
    // RLOG-2: Reject attempts to regress or jump past the last appended entry.
    if (index < commit_index_) {
        spdlog::warn("RaftLog::setCommitIndex: rejecting regression from {} to {}",
                     commit_index_, index);
        if (!persistence_path_.empty()) {
            throw std::runtime_error("persistent Raft commit index cannot regress");
        }
        return;
    }
    const uint64_t last = log_.empty() ? snapshot_index_ : log_.rbegin()->first;
    if (index > last) {
        spdlog::error("RaftLog::setCommitIndex: index {} exceeds last log entry {} – ignored",
                      index, last);
        if (!persistence_path_.empty()) {
            throw std::runtime_error("persistent Raft commit index exceeds the log tail");
        }
        return;
    }
    if (!persistence_path_.empty()) {
        appendDurableRecord("C|" + std::to_string(index));
    }
    commit_index_ = index;
}

uint64_t RaftLog::getCommitIndex() const {
    /**
     * @brief TIMEOUT ENFORCEMENT: Fast read with timeout
     * @param[in] mutex_ Input parameter.
     * @param[in] defer_lock Input parameter.
     * @return Return value.
     */
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::getCommitIndex timeout after {}ms", timeout.count());
        return 0;  // Return 0 on timeout (nothing committed)
    }
    return commit_index_;
}

uint64_t RaftLog::getLastLogIndex() const {
    /**
     * @brief TIMEOUT ENFORCEMENT: Fast read with timeout
     * @param[in] mutex_ Input parameter.
     * @param[in] defer_lock Input parameter.
     * @return Return value.
     */
    std::unique_lock<std::timed_mutex> lock(mutex_, std::defer_lock);
    auto timeout = getConsensusTimeout();
    
    if (!lock.try_lock_for(timeout)) {
        spdlog::error("RaftLog::getLastLogIndex timeout after {}ms", timeout.count());
        return snapshot_index_;  // Return snapshot index on timeout
    }
    if (log_.empty()) {
        // RLOG-1: After snapshot compaction the in-memory log is empty but
        // snapshot_index_ marks the last included entry. Return it so that
        // callers (e.g. propose()) compute the correct next log slot.
        return snapshot_index_;
    }
    return log_.rbegin()->first;
}

uint64_t RaftLog::getLastLogTerm() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);
    if (log_.empty()) {
        return snapshot_term_;
    }
    return log_.rbegin()->second.term;
}

size_t RaftLog::size() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return log_.size();
}

size_t RaftLog::estimatedSizeBytes() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);
    // Fixed overhead per entry: two uint64_t (term + index) + one uint64_t (timestamp)
    constexpr size_t kEntryOverhead = sizeof(uint64_t) * 3;
    size_t total = 0;
    for (const auto& [idx, entry] : log_) {
        total += kEntryOverhead + entry.command.size() ;
    }
    return total;
}

/**
 * @brief Compact Up To.
 * @param[in] snapshot_index Input parameter.
 * @param[in] snapshot_term Input parameter.
 * @details Calls: lock(), spdlog::error(), upper_bound(), erase(), begin().
 */
void RaftLog::compactUpTo(uint64_t snapshot_index, uint64_t snapshot_term) {
    std::lock_guard<std::timed_mutex> lock(mutex_);

    // Safety: never compact past committed entries.  The caller must ensure
    // that snapshot_index <= commit_index before invoking this method.
    if (snapshot_index > commit_index_) {
        spdlog::error("RaftLog::compactUpTo: snapshot_index ({}) > commit_index ({}) – "
                      "refusing to compact uncommitted entries",
                      snapshot_index, commit_index_);
        if (!persistence_path_.empty()) {
            throw std::runtime_error("persistent Raft snapshot exceeds the commit index");
        }
        return;
    }
    if (!persistence_path_.empty() && snapshot_index < snapshot_index_) {
        throw std::runtime_error("persistent Raft snapshot index cannot regress");
    }
    if (!persistence_path_.empty()) {
        const auto snapshot_entry = log_.find(snapshot_index);
        const bool has_matching_entry =
            snapshot_index == snapshot_index_ ? snapshot_term == snapshot_term_
            : snapshot_entry != log_.end() && snapshot_entry->second.term == snapshot_term;
        if (!has_matching_entry) {
            throw std::runtime_error("persistent Raft snapshot term does not match the log");
        }
    }

    if (!persistence_path_.empty()) {
        appendDurableRecord("K|" + std::to_string(snapshot_index) + "|" +
                            std::to_string(snapshot_term));
    }

    // Erase all entries at or below snapshot_index
    auto it = log_.upper_bound(snapshot_index);
    log_.erase(log_.begin(), it);

    // Record snapshot anchor so hasEntry() and prevLogIndex checks still work
    snapshot_index_ = snapshot_index;
    snapshot_term_  = snapshot_term;
    // commit_index_ is deliberately NOT advanced here: the caller already
    // verified snapshot_index <= commit_index, so commit_index_ is already
    // correct and we must not move it forward.
    compactJournal();
}

/**
 * @brief Set Snapshot Meta.
 * @param[in] index Input parameter.
 * @param[in] term Input parameter.
 * @details Calls: lock().
 */
void RaftLog::setSnapshotMeta(uint64_t index, uint64_t term) {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    if (!persistence_path_.empty()) {
        if (index < snapshot_index_ || index > commit_index_ ||
            (index == snapshot_index_ && term != snapshot_term_)) {
            throw std::runtime_error("persistent Raft snapshot boundary is invalid");
        }
        if (index > snapshot_index_) {
            const auto snapshot_entry = log_.find(index);
            if (snapshot_entry == log_.end() || snapshot_entry->second.term != term) {
                throw std::runtime_error("persistent Raft snapshot term does not match the log");
            }
        }
        appendDurableRecord("S|" + std::to_string(index) + "|" +
                            std::to_string(term));
    }
    snapshot_index_ = index;
    snapshot_term_  = term;
}

uint64_t RaftLog::getSnapshotIndex() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return snapshot_index_;
}

uint64_t RaftLog::getSnapshotTerm() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::timed_mutex> lock(mutex_);
    return snapshot_term_;
}

/**
 * @brief Clear.
 * @details Calls: lock().
 */
void RaftLog::clear() {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    if (!persistence_path_.empty()) {
        throw std::runtime_error("RaftLog::clear is unavailable for persistent journals");
    }
    log_.clear();
    commit_index_ = 0;
    snapshot_index_ = 0;
    snapshot_term_ = 0;
}

// ============================================================================
// RaftSnapshotManager
// ============================================================================

namespace {

/**
 * @brief Sha256 Hex.
 * @param[in] data Input parameter.
 * @param[in] size Input parameter.
 * @return Return value.
 * @details Calls: SHA256(), std::setw(), std::setfill(), str().
 */
std::string sha256Hex(const uint8_t* data, size_t size) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (size == 0) {
        // SHA-256 of empty input is well-defined; use a static 0-length buffer.
        static const uint8_t kEmpty[1] = {0};
        SHA256(kEmpty, 0, hash);
    } else {
        SHA256(data, size, hash);
    }
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

}  // namespace

RaftSnapshotManager::RaftSnapshotManager(const Config& config)
    : config_(config) {
    if (config_.chunk_size_bytes == 0) {
        spdlog::warn("RaftSnapshotManager: chunk_size_bytes=0 is invalid; "
                     "clamping to default 4 MB");
        config_.chunk_size_bytes = kDefaultChunkSizeBytes;
    }
    std::string directory_error;
    if (!createDirectoriesDurably(config_.snapshot_directory, directory_error)) {
        throw std::runtime_error(directory_error);
    }
}

/* static */ std::string RaftSnapshotManager::computeChecksum(const uint8_t* data, size_t size) {
    return sha256Hex(data, size);
}

std::string RaftSnapshotManager::snapshotPath(uint64_t snapshot_index) const {
    return config_.snapshot_directory + "/raft_snapshot_" +
           std::to_string(snapshot_index) + ".bin";
}

bool RaftSnapshotManager::shouldCompact(const RaftLog& log) const {
    return log.estimatedSizeBytes() >= config_.compaction_threshold_bytes;
}

/**
 * @brief Create And Install.
 * @param[in,out] log Input/output parameter.
 * @param[in] snapshot_index Input parameter.
 * @param[in] snapshot_term Input parameter.
 * @param[in] state_data Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), spdlog::error(), getCommitIndex(), getEntry(), getSnapshotIndex(), getSnapshotTerm(), empty(), spdlog::warn().
 */
bool RaftSnapshotManager::createAndInstall(RaftLog& log,
                                            uint64_t snapshot_index,
                                            uint64_t snapshot_term,
                                            const std::vector<uint8_t>& state_data) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // 1. Validate preconditions
        if (snapshot_index == 0) {
            spdlog::error("RaftSnapshotManager: snapshot_index must be > 0");
            return false;
        }
        // Compaction must only cover committed entries; refuse if the caller
        // attempts to snapshot beyond what has been committed.
        if (snapshot_index > log.getCommitIndex()) {
            spdlog::error("RaftSnapshotManager: snapshot_index ({}) > commit_index ({}) – "
                          "refusing to snapshot uncommitted entries",
                          snapshot_index, log.getCommitIndex());
            return false;
        }
        // The snapshot_term must match the term of the entry at snapshot_index
        // (or the existing snapshot anchor if the entry was already compacted).
        if (auto entry = log.getEntry(snapshot_index)) {
            if (entry->term != snapshot_term) {
                spdlog::error("RaftSnapshotManager: snapshot_term ({}) does not match "
                              "log[{}].term ({}) – Raft safety violation",
                              snapshot_term, snapshot_index, entry->term);
                return false;
            }
        } else if (log.getSnapshotIndex() == snapshot_index &&
                   log.getSnapshotTerm() != snapshot_term) {
            spdlog::error("RaftSnapshotManager: snapshot_term ({}) does not match "
                          "existing snapshot anchor term ({}) for index {}",
                          snapshot_term, log.getSnapshotTerm(), snapshot_index);
            return false;
        }
        if (state_data.empty()) {
            spdlog::warn("RaftSnapshotManager: creating snapshot with empty state data");
        }

        // 2. Compute checksum of raw (pre-compression) state data
        const std::string checksum = computeChecksum(state_data.data(),state_data.size());

        // 3. Compress with ZSTD level 3
        auto compressed = themis::utils::zstd_compress(state_data, config_.compression_level);
        if (compressed.empty() && !state_data.empty()) {
            spdlog::error("RaftSnapshotManager: ZSTD compression failed for snapshot {}",
                          snapshot_index);
            return false;
        }

        // 4. Build binary snapshot file layout:
        //    [snapshot_index: 8B][snapshot_term: 8B][uncompressed_size: 8B]
        //    [checksum_len: 4B][checksum: N bytes][compressed_data: rest]
        const uint32_t checksum_len = static_cast<uint32_t>(checksum.size());
        const uint64_t uncompressed_size = state_data.size();
        const uint64_t ts = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        const std::string path = snapshotPath(snapshot_index);
        const std::string temp_path = path + ".tmp";

        std::ofstream file(temp_path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            spdlog::error("RaftSnapshotManager: cannot open {} for writing", temp_path);
            return false;
        }

        auto write64 = [&](uint64_t v) -> bool {
            file.write(reinterpret_cast<const char*>(&v), sizeof(v));
            return file.good();
        };
        auto write32 = [&](uint32_t v) -> bool {
            file.write(reinterpret_cast<const char*>(&v), sizeof(v));
            return file.good();
        };

        if (!write64(snapshot_index) ||
            !write64(snapshot_term) ||
            !write64(uncompressed_size) ||
            !write64(ts) ||
            !write32(checksum_len)) {
            spdlog::error("RaftSnapshotManager: header write failed for {}", temp_path);
            file.close();
            std::error_code ec = {};
            std::filesystem::remove(temp_path, ec);
            return false;
        }

        file.write(checksum.data(), static_cast<std::streamsize>(checksum_len));
        if (!file.good()) {
            spdlog::error("RaftSnapshotManager: checksum write failed for {}", temp_path);
            file.close();
            std::error_code ec = {};
            std::filesystem::remove(temp_path, ec);
            return false;
        }

        if (!compressed.empty()) {
            file.write(reinterpret_cast<const char*>(compressed.data()),
                       static_cast<std::streamsize>(compressed.size()));
            if (!file.good()) {
                spdlog::error("RaftSnapshotManager: payload write failed for {}", temp_path);
                file.close();
                std::error_code ec = {};
                std::filesystem::remove(temp_path, ec);
                return false;
            }
        }

        file.flush();
        if (!file.good()) {
            spdlog::error("RaftSnapshotManager: flush failed for {}", temp_path);
            file.close();
            std::error_code ec = {};
            std::filesystem::remove(temp_path, ec);
            return false;
        }
        file.close();
        if (file.fail()) {
            spdlog::error("RaftSnapshotManager: close failed for {}", temp_path);
            std::error_code ec = {};
            std::filesystem::remove(temp_path, ec);
            return false;
        }

        std::string install_error;
        if (!installDurableSnapshot(temp_path, path, install_error)) {
            spdlog::error("RaftSnapshotManager: durable install failed {} -> {}: {}",
                          temp_path, path, install_error);
            std::error_code ec = {};
            std::filesystem::remove(temp_path, ec);
            return false;
        }

        const double ratio = uncompressed_size > 0
            ? static_cast<double>(uncompressed_size) / std::max<size_t>(1,compressed.size())
            : 1.0;

        spdlog::info("RaftSnapshotManager: created snapshot index={} term={} "
                     "uncompressed={}B compressed={}B ratio={:.2f}x path={}",
                     snapshot_index, snapshot_term,
                     uncompressed_size,compressed.size(), ratio, path);

        // 5. Compact the in-memory Raft log
        log.compactUpTo(snapshot_index, snapshot_term);

        // 6. Remove old snapshots beyond retention limit
        cleanupOldSnapshots();

        return true;

    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: exception creating snapshot: {}", e.what());
        return false;
    }
}

std::optional<RaftSnapshot> RaftSnapshotManager::loadSnapshot(uint64_t snapshot_index) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::string path = snapshotPath(snapshot_index);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    try {
        // Minimum valid file: 4×8B (index/term/uncompressed_size/timestamp) + 4B (checksum_len)
        constexpr size_t kMinHeaderBytes = 4 * sizeof(uint64_t) + sizeof(uint32_t);
        // Maximum checksum length we will accept (SHA-256 hex is exactly 64 chars)
        constexpr uint32_t kExpectedChecksumLen = 64;

        const size_t file_size = std::filesystem::file_size(path);
        if (file_size < kMinHeaderBytes) {
            spdlog::error("RaftSnapshotManager: snapshot file too small ({} bytes): {}",
                          file_size, path);
            return std::nullopt;
        }

        /**
         * @brief File.
         * @param[in] path Input parameter.
         * @param[in] binary Input parameter.
         * @return Return value.
         */
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            spdlog::error("RaftSnapshotManager: cannot open {} for reading", path);
            return std::nullopt;
        }

        // Helper lambdas that check stream state after every read
        auto read64 = [&](const char* field) -> std::optional<uint64_t> {
            uint64_t v = 0;
            file.read(reinterpret_cast<char*>(&v), sizeof(v));
            if (!file.good() || file.gcount() != static_cast<std::streamsize>(sizeof(v))) {
                spdlog::error("RaftSnapshotManager: failed to read {} from {}", field, path);
                return std::nullopt;
            }
            return v;
        };
        auto read32 = [&](const char* field) -> std::optional<uint32_t> {
            uint32_t v = 0;
            file.read(reinterpret_cast<char*>(&v), sizeof(v));
            if (!file.good() || file.gcount() != static_cast<std::streamsize>(sizeof(v))) {
                spdlog::error("RaftSnapshotManager: failed to read {} from {}", field, path);
                return std::nullopt;
            }
            return v;
        };

        RaftSnapshot snap;

        auto idx = read64("snapshot_index");  if (!idx) return std::nullopt;
        snap.snapshot_index = *idx;

        auto term = read64("snapshot_term");  if (!term) return std::nullopt;
        snap.snapshot_term = *term;

        auto usize = read64("uncompressed_size"); if (!usize) return std::nullopt;
        snap.uncompressed_size = *usize;

        auto ts = read64("timestamp");  if (!ts) return std::nullopt;
        snap.timestamp = *ts;

        auto clen = read32("checksum_len"); if (!clen) return std::nullopt;
        const uint32_t checksum_len = *clen;

        // Enforce expected checksum length to prevent large allocations from
        // a corrupt or malicious file.
        if (checksum_len != kExpectedChecksumLen) {
            spdlog::error("RaftSnapshotManager: unexpected checksum_len {} (expected {}) in {}",
                          checksum_len, kExpectedChecksumLen, path);
            return std::nullopt;
        }
        // Verify the file contains enough bytes for the checksum + any data
        const size_t bytes_consumed_so_far = kMinHeaderBytes;
        if (file_size < bytes_consumed_so_far + checksum_len) {
            spdlog::error("RaftSnapshotManager: file too small for checksum in {}", path);
            return std::nullopt;
        }

        snap.checksum.resize(checksum_len);
        file.read(snap.checksum.data(), static_cast<std::streamsize>(checksum_len));
        if (!file.good() || file.gcount() != static_cast<std::streamsize>(checksum_len)) {
            spdlog::error("RaftSnapshotManager: failed to read checksum from {}", path);
            return std::nullopt;
        }

        // Read remaining bytes as compressed data
        const std::streampos data_start = file.tellg();
        file.seekg(0, std::ios::end);
        const std::streampos data_end = file.tellg();
        const size_t data_size = static_cast<size_t>(data_end - data_start);
        file.seekg(data_start);

        snap.data.resize(data_size);
        if (data_size > 0) {
            file.read(reinterpret_cast<char*>(snap.data.data()),
                      static_cast<std::streamsize>(data_size));
            if (file.gcount() != static_cast<std::streamsize>(data_size)) {
                spdlog::error("RaftSnapshotManager: truncated data read from {}", path);
                return std::nullopt;
            }
        }
        file.close();

        // Decompress to verify checksum
        if (!snap.data.empty()) {
            auto decompressed = themis::utils::zstd_decompress(snap.data);
            if (decompressed.empty() && snap.uncompressed_size > 0) {
                spdlog::error("RaftSnapshotManager: decompression failed for snapshot {}",
                              snapshot_index);
                return std::nullopt;
            }
            // Verify decompressed size matches the stored metadata
            if (decompressed.size() != snap.uncompressed_size) {
                spdlog::error("RaftSnapshotManager: decompressed size mismatch for snapshot {}: "
                              "expected {} got {}",
                              snapshot_index, snap.uncompressed_size,decompressed.size());
                return std::nullopt;
            }
            const std::string actual_checksum =
                computeChecksum(decompressed.data(),decompressed.size());
            if (actual_checksum != snap.checksum) {
                spdlog::error("RaftSnapshotManager: checksum mismatch for snapshot {} "
                              "(expected={} actual={})",
                              snapshot_index, snap.checksum, actual_checksum);
                return std::nullopt;
            }
        }

        return snap;

    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: exception loading snapshot {}: {}",
                      snapshot_index, e.what());
        return std::nullopt;
    }
}

std::optional<RaftSnapshot> RaftSnapshotManager::loadLatestSnapshot() const {
    auto snapshots = listSnapshots();
    if (snapshots.empty()) {
        return std::nullopt;
    }
    return loadSnapshot(snapshots.front());
}

std::vector<uint64_t> RaftSnapshotManager::listSnapshots() const {
    std::vector<uint64_t> ids;
    try {
        if (!std::filesystem::exists(config_.snapshot_directory)) {
            return ids;
        }
        for (const auto& entry :
             std::filesystem::directory_iterator(config_.snapshot_directory)) {
            if (!entry.is_regular_file()) {
              continue;
            }
            const std::string name = entry.path().filename().string();
            // Format: raft_snapshot_<index>.bin
            if (name.rfind("raft_snapshot_", 0) == 0 && name.ends_with(".bin")) {
                const std::string id_str = name.substr(14, name.size() - 18);
                try {
                    ids.push_back(std::stoull(id_str));
                } catch (const std::invalid_argument&) {
                    continue;
                } catch (const std::out_of_range&) {
                    continue;
                }
            }
        }
        std::sort(ids.begin(), ids.end(), std::greater<uint64_t>());
    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: listSnapshots error: {}", e.what());
    }
    return ids;
}

size_t RaftSnapshotManager::getChunkCount(uint64_t snapshot_index) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::string path = snapshotPath(snapshot_index);
    if (!std::filesystem::exists(path)) {
        return 0;
    }
    const size_t file_size = std::filesystem::file_size(path);
    if (file_size == 0) {
        return 0;
    }
    return (file_size + config_.chunk_size_bytes - 1) / config_.chunk_size_bytes;
}

std::optional<RaftSnapshotChunk> RaftSnapshotManager::getChunk(uint64_t snapshot_index,
                                                                 uint64_t chunk_index) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::string path = snapshotPath(snapshot_index);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    try {
        const size_t file_size = std::filesystem::file_size(path);
        if (file_size == 0) {
            return std::nullopt;
        }

        const size_t total_chunks =
            (file_size + config_.chunk_size_bytes - 1) / config_.chunk_size_bytes;
        if (chunk_index >= total_chunks) {
            return std::nullopt;
        }

        const size_t offset = chunk_index * config_.chunk_size_bytes;
        const size_t length = std::min(config_.chunk_size_bytes, file_size - offset);

        /**
         * @brief File.
         * @param[in] path Input parameter.
         * @param[in] binary Input parameter.
         * @return Return value.
         */
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return std::nullopt;
        }
        file.seekg(static_cast<std::streamoff>(offset));
        if (!file.good()) {
            return std::nullopt;
        }

        RaftSnapshotChunk chunk;
        chunk.snapshot_index = snapshot_index;
        chunk.chunk_index    = chunk_index;
        chunk.total_chunks   = total_chunks;
        chunk.last_chunk     = (chunk_index + 1 == total_chunks);
        chunk.data.resize(length);
        file.read(reinterpret_cast<char*>(chunk.data.data()),
                  static_cast<std::streamsize>(length));
        if (!file.good() || file.gcount() != static_cast<std::streamsize>(length)) {
            return std::nullopt;
        }
        file.close();

        chunk.checksum = computeChecksum(chunk.data.data(),chunk.data.size());
        return chunk;

    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: getChunk error snapshot={} chunk={}: {}",
                      snapshot_index, chunk_index, e.what());
        return std::nullopt;
    }
}

/**
 * @brief Cleanup Old Snapshots.
 * @details Calls: std::filesystem::exists(), std::filesystem::directory_iterator(), is_regular_file(), path(), filename(), string(), rfind(), ends_with().
 */
void RaftSnapshotManager::cleanupOldSnapshots() {
    // NOTE: called with mutex_ held
    try {
        // Build list without taking the lock again (already held by caller)
        std::vector<uint64_t> ids = {};

        if (std::filesystem::exists(config_.snapshot_directory)) {
            for (const auto& entry :
                 std::filesystem::directory_iterator(config_.snapshot_directory)) {
                if (!entry.is_regular_file()) {
                  continue;
                }
                const std::string name = entry.path().filename().string();
                if (name.rfind("raft_snapshot_", 0) == 0 && name.ends_with(".bin")) {
                    const std::string id_str = name.substr(14, name.size() - 18);
                    try {
                        ids.push_back(std::stoull(id_str));
                    } catch (const std::invalid_argument&) {
                        continue;
                    } catch (const std::out_of_range&) {
                        continue;
                    }
                }
            }
        }
        std::sort(ids.begin(), ids.end(), std::greater<uint64_t>());
        for (size_t i = config_.max_snapshots; i < ids.size(); ++i) {
            const std::string p = snapshotPath(ids[i]);
            std::filesystem::remove(p);
            spdlog::info("RaftSnapshotManager: removed old snapshot {}", ids[i]);
        }
    } catch (const std::exception& e) {
        spdlog::error("RaftSnapshotManager: cleanupOldSnapshots error: {}", e.what());
    }
}

}  // namespace sharding
}  // namespace themisdb
