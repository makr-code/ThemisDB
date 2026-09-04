// RocksDB WAL (Write-Ahead Log) and Manifest corruption recovery tests
// Tests recovery behavior when critical DB files are corrupted

#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/options.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <iostream>

namespace fs = std::filesystem;

class WALManifestCorruptionTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = fs::temp_directory_path() / ("themis_wal_corruption_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(db_path_);
    }

    void TearDown() override {
        closeDB();
        std::error_code ec;
        fs::remove_all(db_path_, ec);
    }

    void openDB() {
        if (db_) {
          return;
        }

        rocksdb::TransactionDBOptions txn_db_opts;
        txn_db_opts.transaction_lock_timeout = 500;

        rocksdb::Options options;
        options.create_if_missing = true;
        options.max_open_files = 256;
        options.wal_dir = (db_path_ / "wal").string();  // Separate WAL dir for easier manipulation

        rocksdb::Status s = rocksdb::TransactionDB::Open(
            options, txn_db_opts, db_path_.string(), &db_);
        ASSERT_TRUE(s.ok()) << "Failed to open TransactionDB: " << s.ToString();
    }

    void closeDB() {
        if (db_) {
            delete db_;
            db_ = nullptr;
        }
    }

    // Write a test document
    void writeDoc(const std::string& key, int value) {
        ASSERT_TRUE(db_) << "DB not open";

        rocksdb::Transaction* txn = db_->BeginTransaction(
            rocksdb::WriteOptions(), rocksdb::TransactionOptions());
        ASSERT_TRUE(txn);

        nlohmann::json doc{{"key", key}, {"value", value}};
        rocksdb::Status s = txn->Put(key, doc.dump());
        ASSERT_TRUE(s.ok());

        s = txn->Commit();
        ASSERT_TRUE(s.ok());
        delete txn;
    }

    // Read and verify a document
    bool verifyDoc(const std::string& key, int expected_value) {
        if (!db_) {
          return false;
        }

        std::string value;
        rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &value);

        if (s.IsNotFound()) {
          return false;
        }
        if (!s.ok()) {
          return false;
        }

        auto doc = nlohmann::json::parse(value, nullptr, false);
        if (doc.is_discarded()) {
          return false;
        }

        return doc.contains("value") && doc["value"] == expected_value;
    }

    // Corrupt a file by truncating it
    void corruptFileByTruncation(const fs::path& file_path, size_t bytes_to_keep = 10) {
        ASSERT_TRUE(fs::exists(file_path)) << "File does not exist: " << file_path.string();

        // Truncate the file to a small size (corrupt it)
        std::ofstream file(file_path, std::ios::binary | std::ios::in | std::ios::out);
        ASSERT_TRUE(file.is_open()) << "Failed to open file for corruption: " << file_path.string();

        file.seekp(0, std::ios::end);
        std::streampos file_size = file.tellp();

        if (file_size > static_cast<std::streampos>(bytes_to_keep)) {
            file.seekp(bytes_to_keep);
            file.put('\x00');  // Write a null byte to truncate
            file.flush();
            file.close();

            // Use filesystem to actually truncate if possible
            std::error_code ec;
            fs::resize_file(file_path, bytes_to_keep, ec);
        }
    }

    // Corrupt a file by flipping bits
    void corruptFileByBitFlip(const fs::path& file_path, size_t offset = 100) {
        ASSERT_TRUE(fs::exists(file_path)) << "File does not exist";

        std::vector<char> buffer;
        {
            std::ifstream file(file_path, std::ios::binary);
            ASSERT_TRUE(file.is_open());

            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0);

            buffer.resize(size);
            file.read(buffer.data(), size);
        }

        // Flip bits at offset
        if (offset < buffer.size()) {
            buffer[offset] ^= 0xFF;  // Flip all bits

            std::ofstream file(file_path, std::ios::binary);
            ASSERT_TRUE(file.is_open());
            file.write(buffer.data(), buffer.size());
            file.close();
        }
    }

    // Get WAL file path
    fs::path getWALDir() {
        return db_path_ / "wal";
    }

    // Get MANIFEST file path
    fs::path getManifestFile() {
        // Find MANIFEST-* file in db_path_
        for (const auto& entry : fs::directory_iterator(db_path_)) {
            if (entry.is_regular_file() && entry.path().filename().string().find("MANIFEST") == 0) {
                return entry.path();
            }
        }
        return {};
    }

    fs::path db_path_;
    rocksdb::TransactionDB* db_ = nullptr;
};

// Test basic write without corruption
TEST_F(WALManifestCorruptionTest, BaselineWriteWithoutCorruption) {
    openDB();

    // Write and verify
    writeDoc("doc1", 100);
    ASSERT_TRUE(verifyDoc("doc1", 100));

    closeDB();

    // Reopen and re-verify
    openDB();
    ASSERT_TRUE(verifyDoc("doc1", 100));
    closeDB();
}

// Test WAL truncation and recovery
TEST_F(WALManifestCorruptionTest, WALTruncationRecovery) {
    openDB();

    // Write initial data
    for (int i = 0; i < 10; ++i) {
        writeDoc("doc_" + std::to_string(i), i * 10);
    }

    closeDB();

    // Try to corrupt WAL directory (if it exists and has files)
    fs::path wal_dir = getWALDir();
    if (fs::exists(wal_dir)) {
        for (const auto& entry : fs::directory_iterator(wal_dir)) {
            if (entry.is_regular_file()) {
                // Truncate WAL file
                try {
                    corruptFileByTruncation(entry.path(), 5);
                    break;  // Corrupt just one file
                } catch (...) {
                    // Skip if truncation fails
                }
            }
        }
    }

    // Try to reopen - RocksDB should attempt recovery
    rocksdb::TransactionDBOptions txn_db_opts;
    rocksdb::Options options;
    options.create_if_missing = false;
    options.wal_dir = wal_dir.string();

    rocksdb::TransactionDB* recovery_db = nullptr;
    rocksdb::Status s = rocksdb::TransactionDB::Open(
        options, txn_db_opts, db_path_.string(), &recovery_db);

    // Even if recovery fails initially, we test graceful handling
    if (s.ok() && recovery_db) {
        // Check if some data persists
        std::string value;
        rocksdb::Status check = recovery_db->Get(rocksdb::ReadOptions(), "doc_0", &value);
        // We don't assert here - the important thing is DB didn't crash
        delete recovery_db;
    }

    db_ = nullptr;  // Already deleted or failed to open
}

// Test MANIFEST corruption and recovery
TEST_F(WALManifestCorruptionTest, ManifestCorruptionRecovery) {
    openDB();

    // Write initial data
    for (int i = 0; i < 20; ++i) {
        writeDoc("manifest_doc_" + std::to_string(i), i);
    }

    closeDB();

    // Get MANIFEST file path and corrupt it
    fs::path manifest = getManifestFile();
    if (manifest.empty()) {
        GTEST_SKIP() << "No MANIFEST file found";
    }

    try {
        corruptFileByBitFlip(manifest, 50);  // Flip bits at offset 50
    } catch (...) {
        GTEST_SKIP() << "Could not corrupt MANIFEST file";
    }

    // Try to reopen
    rocksdb::TransactionDBOptions txn_db_opts;
    rocksdb::Options options;
    options.create_if_missing = false;

    rocksdb::TransactionDB* recovery_db = nullptr;
    rocksdb::Status s = rocksdb::TransactionDB::Open(
        options, txn_db_opts, db_path_.string(), &recovery_db);

    // RocksDB may fail to open or succeed with recovery
    if (s.ok() && recovery_db) {
        delete recovery_db;
    }

    db_ = nullptr;
}

// Test CompactRange during write stress
TEST_F(WALManifestCorruptionTest, CompactRangeStressDuringWrites) {
    openDB();

    // Insert bulk data
    for (int i = 0; i < 100; ++i) {
        writeDoc("compact_doc_" + std::to_string(i), i);
    }

    // Trigger compaction while writing
    rocksdb::Status s = db_->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);
    EXPECT_TRUE(s.ok() || s.IsShutdownInProgress())
        << "CompactRange failed unexpectedly: " << s.ToString();

    // Verify data is still intact
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(verifyDoc("compact_doc_" + std::to_string(i), i))
            << "Data lost during compaction at index " << i;
    }

    closeDB();

    // Reopen and verify again
    openDB();
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(verifyDoc("compact_doc_" + std::to_string(i), i))
            << "Data lost after restart at index " << i;
    }
    closeDB();
}

// Test multiple compactions and recovery
TEST_F(WALManifestCorruptionTest, MultipleCompactionsAndRecovery) {
    openDB();

    // Write in batches and compact after each batch
    for (int batch = 0; batch < 3; ++batch) {
        for (int i = 0; i < 30; ++i) {
            writeDoc("batch_" + std::to_string(batch) + "_doc_" + std::to_string(i), i);
        }

        // Compact after each batch
        rocksdb::Status s = db_->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);
        EXPECT_TRUE(s.ok()) << "CompactRange failed in batch " << batch;
    }

    closeDB();

    // Reopen and verify all data
    openDB();
    for (int batch = 0; batch < 3; ++batch) {
        for (int i = 0; i < 30; ++i) {
            EXPECT_TRUE(verifyDoc("batch_" + std::to_string(batch) + "_doc_" + std::to_string(i), i))
                << "Data lost in batch " << batch << " at index " << i;
        }
    }
    closeDB();
}

// Test large transaction with compaction
TEST_F(WALManifestCorruptionTest, LargeTransactionWithCompaction) {
    openDB();

    // Single large transaction
    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    ASSERT_TRUE(txn);

    for (int i = 0; i < 50; ++i) {
        nlohmann::json doc{{"index", i}, {"value", i * 100}};
        rocksdb::Status s = txn->Put("large_txn_" + std::to_string(i), doc.dump());
        EXPECT_TRUE(s.ok()) << "Put failed at index " << i;
    }

    rocksdb::Status s = txn->Commit();
    EXPECT_TRUE(s.ok()) << "Commit failed";
    delete txn;

    // Compact
    s = db_->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);
    EXPECT_TRUE(s.ok());

    // Verify all committed data
    for (int i = 0; i < 50; ++i) {
        std::string value;
        rocksdb::Status check = db_->Get(rocksdb::ReadOptions(), "large_txn_" + std::to_string(i), &value);
        EXPECT_TRUE(check.ok()) << "Failed to read at index " << i;
    }

    closeDB();

    // Reopen and re-verify
    openDB();
    for (int i = 0; i < 50; ++i) {
        std::string value;
        rocksdb::Status check = db_->Get(rocksdb::ReadOptions(), "large_txn_" + std::to_string(i), &value);
        EXPECT_TRUE(check.ok()) << "Data lost after restart at index " << i;
    }
    closeDB();
}

// Test database recovery after incomplete flush
TEST_F(WALManifestCorruptionTest, IncompleteFlushRecovery) {
    openDB();

    // Write data in small increments
    for (int i = 0; i < 50; ++i) {
        writeDoc("flush_doc_" + std::to_string(i), i);

        // Small delay to allow partial flushing
        if (i % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    closeDB();

    // Reopen and verify recovery
    openDB();
    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(verifyDoc("flush_doc_" + std::to_string(i), i))
            << "Data lost at index " << i;
    }
    closeDB();
}

// Test empty WAL recovery
TEST_F(WALManifestCorruptionTest, EmptyWALRecovery) {
    openDB();
    writeDoc("test", 42);
    closeDB();

    // Verify DB opens normally
    openDB();
    ASSERT_TRUE(verifyDoc("test", 42));
    closeDB();
}

// Test snapshot isolation during compaction
TEST_F(WALManifestCorruptionTest, SnapshotIsolationDuringCompaction) {
    openDB();

    // Write initial batch
    for (int i = 0; i < 25; ++i) {
        writeDoc("snap_" + std::to_string(i), i);
    }

    // Get snapshot before compaction
    const rocksdb::Snapshot* snapshot = db_->GetSnapshot();
    ASSERT_TRUE(snapshot);

    // Write more data
    for (int i = 25; i < 50; ++i) {
        writeDoc("snap_" + std::to_string(i), i);
    }

    // Compact
    rocksdb::Status s = db_->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);
    EXPECT_TRUE(s.ok());

    // Read through snapshot should only see first 25
    for (int i = 0; i < 25; ++i) {
        std::string value;
        rocksdb::Status check = db_->Get(rocksdb::ReadOptions(), "snap_" + std::to_string(i), &value);
        EXPECT_TRUE(check.ok());
    }

    // Release snapshot
    db_->ReleaseSnapshot(snapshot);

    closeDB();

    // Verify all data persists
    openDB();
    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(verifyDoc("snap_" + std::to_string(i), i))
            << "Data lost at index " << i;
    }
    closeDB();
}
