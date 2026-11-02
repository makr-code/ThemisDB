// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "storage/rocksdb_wrapper.h"
#include "storage/streaming_ingest_manager.h"

#include <rocksdb/env.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory()
        : path_(std::filesystem::temp_directory_path() /
                ("themis_ingest_wal_failure_" +
                 std::to_string(std::chrono::steady_clock::now()
                                    .time_since_epoch()
                                    .count()))) {}

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

class WalAppendFailure {
public:
    void arm(std::size_t append_number) {
        std::lock_guard<std::mutex> lock(mutex_);
        armed_ = true;
        released_ = false;
        triggered_ = false;
        append_count_ = 0;
        fail_on_append_ = append_number;
    }

    bool shouldFailAppend() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!armed_) {
            return false;
        }

        ++append_count_;
        if (append_count_ != fail_on_append_) {
            return false;
        }

        triggered_ = true;
        condition_.notify_all();
        condition_.wait(lock, [this] { return released_; });
        armed_ = false;
        return true;
    }

    bool waitUntilTriggered(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return condition_.wait_for(lock, timeout,
                                   [this] { return triggered_; });
    }

    void release() {
        std::lock_guard<std::mutex> lock(mutex_);
        released_ = true;
        condition_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::size_t append_count_ = 0;
    std::size_t fail_on_append_ = 0;
    bool armed_ = false;
    bool triggered_ = false;
    bool released_ = false;
};

class FaultInjectingWalFile final : public rocksdb::WritableFileWrapper {
public:
    FaultInjectingWalFile(std::unique_ptr<rocksdb::WritableFile> target,
                          WalAppendFailure& failure)
        : rocksdb::WritableFileWrapper(target.get())
        , target_(std::move(target))
        , failure_(failure) {}

    rocksdb::Status Append(const rocksdb::Slice& data) override {
        if (failure_.shouldFailAppend()) {
            return rocksdb::Status::IOError("injected WAL append failure");
        }
        return rocksdb::WritableFileWrapper::Append(data);
    }

private:
    std::unique_ptr<rocksdb::WritableFile> target_;
    WalAppendFailure& failure_;
};

class FaultInjectingWalEnv final : public rocksdb::EnvWrapper {
public:
    explicit FaultInjectingWalEnv(WalAppendFailure& failure)
        : rocksdb::EnvWrapper(rocksdb::Env::Default())
        , failure_(failure) {}

    rocksdb::Status NewWritableFile(
        const std::string& filename,
        std::unique_ptr<rocksdb::WritableFile>* result,
        const rocksdb::EnvOptions& options) override {
        auto status = rocksdb::EnvWrapper::NewWritableFile(
            filename, result, options);
        constexpr std::string_view wal_suffix = ".log";
        if (!status.ok() || filename.size() < wal_suffix.size() ||
            filename.compare(filename.size() - wal_suffix.size(),
                             wal_suffix.size(), wal_suffix) != 0) {
            return status;
        }

        *result = std::make_unique<FaultInjectingWalFile>(
            std::move(*result), failure_);
        return status;
    }

private:
    WalAppendFailure& failure_;
};

TEST(StreamingIngestRocksDBFailure,
     LaterWalAppendFailureRetainsRetrySuffixBeforeConcurrentEvents) {
    TemporaryDirectory temporary_directory;
    WalAppendFailure failure;
    FaultInjectingWalEnv env(failure);

    themis::RocksDBWrapper::Config db_config;
    db_config.db_path = temporary_directory.path().string();
    db_config.env = &env;
    db_config.memtable_size_mb = 16;
    db_config.block_cache_size_mb = 16;

    auto db = std::make_shared<themis::RocksDBWrapper>(db_config);
    ASSERT_TRUE(db->open());

    themis::StreamingIngestManager::Config ingest_config;
    ingest_config.max_batch_size = 1;
    auto manager = themis::StreamingIngestManager::create(
        db, std::move(ingest_config));

    auto accepted = manager->ingestBatch({
        {"committed-prefix", "prefix"},
        {"retry-key", "older-retry"},
        {"retry-tail", "tail"},
    });
    ASSERT_TRUE(accepted);
    ASSERT_EQ(*accepted, 3u);

    failure.arm(2);
    bool flush_succeeded = true;
    std::string flush_error;
    std::thread flush_thread([&] {
        auto result = manager->flush();
        flush_succeeded = result.has_value();
        if (!result) {
            flush_error = result.error().context();
        }
    });

    const bool failed_append_reached =
        failure.waitUntilTriggered(std::chrono::seconds(30));
    bool newer_event_accepted = false;
    if (failed_append_reached) {
        newer_event_accepted =
            manager->ingest("retry-key", "newer-event").has_value();
    }
    failure.release();
    flush_thread.join();

    ASSERT_TRUE(failed_append_reached)
        << "the injected RocksDB WAL append was not reached";
    ASSERT_TRUE(newer_event_accepted);
    ASSERT_FALSE(flush_succeeded);
    EXPECT_NE(flush_error.find("failed_offset=1"), std::string::npos);
    EXPECT_NE(flush_error.find("total=3"), std::string::npos);
    EXPECT_EQ(manager->stats().events_flushed, 1u);

    std::string value;
    ASSERT_TRUE(db->get("committed-prefix", value));
    EXPECT_EQ(value, "prefix");
    EXPECT_FALSE(db->get("retry-key", value));
    EXPECT_FALSE(db->get("retry-tail", value));

    ASSERT_TRUE(manager->flush());
    EXPECT_EQ(manager->stats().events_flushed, 4u);

    db->close();
    ASSERT_TRUE(db->open());
    ASSERT_TRUE(db->get("committed-prefix", value));
    EXPECT_EQ(value, "prefix");
    ASSERT_TRUE(db->get("retry-key", value));
    EXPECT_EQ(value, "newer-event");
    ASSERT_TRUE(db->get("retry-tail", value));
    EXPECT_EQ(value, "tail");
}

}  // namespace
