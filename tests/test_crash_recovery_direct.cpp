// Direct RocksDB crash/recovery durability tests
// These tests verify TransactionDB write durability without HTTP layer

#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/checkpoint.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <random>
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

class TransactionDBDurabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temp directory for each test
        db_path_ = fs::temp_directory_path() / ("themis_durability_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(db_path_);
    }

    void TearDown() override {
        closeDB();
        std::error_code ec;
        fs::remove_all(db_path_, ec);
    }

    void openDB() {
        if (db_) return;

        rocksdb::TransactionDBOptions txn_db_opts;
        txn_db_opts.transaction_lock_timeout = 500;  // 500ms lock timeout

        rocksdb::Options options;
        options.create_if_missing = true;
        options.create_missing_column_families = true;
        options.max_open_files = 256;

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

    // Insert JSON document into "documents" column family
    void writeDocument(const std::string& key, const nlohmann::json& doc) {
        ASSERT_TRUE(db_) << "DB not open";

        rocksdb::Transaction* txn = db_->BeginTransaction(
            rocksdb::WriteOptions(), rocksdb::TransactionOptions());
        ASSERT_TRUE(txn) << "Failed to begin transaction";

        std::string value = doc.dump();
        rocksdb::Status s = txn->Put(key, value);
        ASSERT_TRUE(s.ok()) << "Put failed: " << s.ToString();

        s = txn->Commit();
        ASSERT_TRUE(s.ok()) << "Commit failed: " << s.ToString();

        delete txn;
    }

    // Retrieve JSON document
    nlohmann::json readDocument(const std::string& key) {
        EXPECT_TRUE(db_) << "DB not open";
        if (!db_) return nlohmann::json();

        std::string value;
        rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &value);

        if (s.IsNotFound()) {
            return nlohmann::json();  // Return null json
        }
        EXPECT_TRUE(s.ok()) << "Get failed: " << s.ToString();

        auto result = nlohmann::json::parse(value, nullptr, false);
        EXPECT_FALSE(result.is_discarded()) << "Invalid JSON: " << value;
        return result;
    }

    // Verify a list of documents exist with their expected values
    void verifyDocuments(const std::vector<std::pair<std::string, int>>& expected_docs) {
        openDB();

        for (const auto& [key, expected_val] : expected_docs) {
            auto doc = readDocument(key);
            EXPECT_FALSE(doc.is_null()) << "Missing document: " << key;
            if (!doc.is_null() && doc.contains("value")) {
                EXPECT_EQ(doc["value"], expected_val) << "Wrong value for " << key;
            }
        }
    }

    // Write count documents and return keys
    std::vector<std::pair<std::string, int>> insertDocuments(int count, int start_idx = 0) {
        std::vector<std::pair<std::string, int>> docs;
        for (int i = 0; i < count; ++i) {
            int idx = start_idx + i;
            std::string key = "doc_" + std::to_string(idx);
            int value = idx * 100;

            nlohmann::json doc{
                {"id", key},
                {"value", value},
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
            };

            writeDocument(key, doc);
            docs.emplace_back(key, value);
        }
        return docs;
    }

    fs::path db_path_;
    rocksdb::TransactionDB* db_ = nullptr;
};

// Basic durability: write -> close -> reopen -> verify
TEST_F(TransactionDBDurabilityTest, BasicWriteDurability) {
    openDB();

    // Write initial document
    auto docs = insertDocuments(1, 0);
    EXPECT_EQ(docs.size(), 1);

    closeDB();

    // Reopen and verify
    verifyDocuments(docs);
}

// Multiple writes before crash
TEST_F(TransactionDBDurabilityTest, MultipleDocs) {
    openDB();
    auto docs = insertDocuments(50, 0);
    closeDB();

    verifyDocuments(docs);
}

// Repeated crash cycles
TEST_F(TransactionDBDurabilityTest, RepeatedCrashCycles) {
    std::vector<std::pair<std::string, int>> all_docs;

    for (int cycle = 0; cycle < 3; ++cycle) {
        openDB();
        auto cycle_docs = insertDocuments(10, cycle * 10);
        all_docs.insert(all_docs.end(), cycle_docs.begin(), cycle_docs.end());
        closeDB();
    }

    // Final verify
    verifyDocuments(all_docs);
}

// Bulk write in single transaction
TEST_F(TransactionDBDurabilityTest, BulkTransactionDurability) {
    openDB();

    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    ASSERT_TRUE(txn);

    std::vector<std::pair<std::string, int>> docs;
    for (int i = 0; i < 100; ++i) {
        std::string key = "bulk_" + std::to_string(i);
        int value = i * 10;
        nlohmann::json doc{{"id", key}, {"value", value}};
        std::string value_str = doc.dump();

        rocksdb::Status s = txn->Put(key, value_str);
        ASSERT_TRUE(s.ok()) << "Put in transaction failed";

        docs.emplace_back(key, value);
    }

    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok()) << "Commit failed: " << s.ToString();
    delete txn;

    closeDB();

    // Verify all were written
    verifyDocuments(docs);
}

// Write, rollback (should not persist), then verify empty
TEST_F(TransactionDBDurabilityTest, RollbackDoesNotPersist) {
    openDB();

    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    ASSERT_TRUE(txn);

    nlohmann::json doc{{"id", "rollback_test"}, {"value", 999}};
    rocksdb::Status s = txn->Put("rollback_test", doc.dump());
    ASSERT_TRUE(s.ok());

    // Explicitly rollback
    s = txn->Rollback();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify document NOT present after rollback
    openDB();
    auto result = readDocument("rollback_test");
    EXPECT_TRUE(result.is_null()) << "Rollback failed - document persisted";
    closeDB();
}

// Parallel transactions
TEST_F(TransactionDBDurabilityTest, ParallelTransactions) {
    openDB();

    std::vector<std::string> keys;
    std::mutex mtx;

    auto writer = [&](int thread_id) {
        for (int i = 0; i < 10; ++i) {
            std::string key = "t" + std::to_string(thread_id) + "_" + std::to_string(i);
            nlohmann::json doc{{"thread", thread_id}, {"index", i}};
            writeDocument(key, doc);

            {
                std::lock_guard<std::mutex> lk(mtx);
                keys.push_back(key);
            }
        }
    };

    std::thread t1(writer, 1);
    std::thread t2(writer, 2);
    t1.join();
    t2.join();

    closeDB();

    // Verify all writes persisted
    openDB();
    for (const auto& key : keys) {
        auto doc = readDocument(key);
        EXPECT_FALSE(doc.is_null()) << "Missing from parallel write: " << key;
    }
    closeDB();
}

// Checkpoint (backup) functionality
TEST_F(TransactionDBDurabilityTest, CheckpointDurability) {
    openDB();

    auto docs = insertDocuments(25, 0);

    // Create checkpoint
    fs::path checkpoint_path = fs::temp_directory_path() / "themis_checkpoint";
    std::error_code ec;
    fs::remove_all(checkpoint_path, ec);
    // Note: Do NOT create the directory - RocksDB expects it to not exist

    rocksdb::Checkpoint* checkpoint = nullptr;
    rocksdb::Status s = rocksdb::Checkpoint::Create(db_, &checkpoint);
    ASSERT_TRUE(s.ok()) << "Checkpoint::Create failed: " << s.ToString();

    s = checkpoint->CreateCheckpoint(checkpoint_path.string());
    ASSERT_TRUE(s.ok()) << "CreateCheckpoint failed: " << s.ToString();
    delete checkpoint;

    closeDB();

    // Verify original DB
    verifyDocuments(docs);

    // Now verify checkpoint is also valid
    rocksdb::Options opts;
    opts.create_if_missing = false;
    rocksdb::TransactionDB* checkpoint_db = nullptr;
    rocksdb::TransactionDBOptions txn_opts;

    s = rocksdb::TransactionDB::Open(
        opts, txn_opts, checkpoint_path.string(), &checkpoint_db);
    ASSERT_TRUE(s.ok()) << "Failed to open checkpoint DB: " << s.ToString();

    for (const auto& [key, expected_val] : docs) {
        std::string value;
        s = checkpoint_db->Get(rocksdb::ReadOptions(), key, &value);
        EXPECT_TRUE(s.ok()) << "Get from checkpoint failed: " << s.ToString();

        auto doc = nlohmann::json::parse(value);
        EXPECT_EQ(doc["value"], expected_val);
    }

    delete checkpoint_db;
    fs::remove_all(checkpoint_path, ec);
}

// Concurrent reads while writing
TEST_F(TransactionDBDurabilityTest, ConcurrentReadWrite) {
    openDB();

    std::vector<std::string> written_keys;
    std::mutex mtx;

    auto writer = [&]() {
        for (int i = 0; i < 50; ++i) {
            std::string key = "concurrent_" + std::to_string(i);
            nlohmann::json doc{{"value", i * 10}};
            writeDocument(key, doc);

            {
                std::lock_guard<std::mutex> lk(mtx);
                written_keys.push_back(key);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };

    std::thread write_thread(writer);

    // Reader thread
    std::vector<nlohmann::json> read_docs;
    auto reader = [&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        for (int i = 0; i < 30; ++i) {
            {
                std::lock_guard<std::mutex> lk(mtx);
                for (const auto& key : written_keys) {
                    auto doc = readDocument(key);
                    if (!doc.is_null()) {
                        read_docs.push_back(doc);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };

    std::thread read_thread(reader);

    write_thread.join();
    read_thread.join();

    closeDB();

    // Verify all written keys persisted
    openDB();
    std::vector<std::pair<std::string, int>> expected;
    for (int i = 0; i < 50; ++i) {
        expected.emplace_back("concurrent_" + std::to_string(i), i * 10);
    }
    verifyDocuments(expected);
    closeDB();
}

// Large document durability
TEST_F(TransactionDBDurabilityTest, LargeDocumentDurability) {
    openDB();

    // Create a large document (1MB string)
    std::string large_string(1024 * 1024, 'x');
    nlohmann::json doc{
        {"id", "large_doc"},
        {"data", large_string},
        {"size", large_string.size()}
    };

    writeDocument("large_doc", doc);
    closeDB();

    // Reopen and verify
    openDB();
    auto retrieved = readDocument("large_doc");
    EXPECT_FALSE(retrieved.is_null());
    if (!retrieved.is_null()) {
        EXPECT_EQ(retrieved["size"], 1024 * 1024);
        EXPECT_EQ(retrieved["data"].get<std::string>().size(), 1024 * 1024);
    }
    closeDB();
}

// Transaction timeout behavior
TEST_F(TransactionDBDurabilityTest, TransactionTimeout) {
    openDB();

    // Normal write
    nlohmann::json doc1{{"id", "doc1"}, {"value", 1}};
    writeDocument("doc1", doc1);

    // Begin long-running transaction
    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    ASSERT_TRUE(txn);

    nlohmann::json doc2{{"id", "doc2"}, {"value", 2}};
    rocksdb::Status s = txn->Put("doc2", doc2.dump());
    ASSERT_TRUE(s.ok());

    // Hold the transaction for a bit, then commit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    s = txn->Commit();
    EXPECT_TRUE(s.ok()) << "Commit should succeed: " << s.ToString();

    delete txn;
    closeDB();

    // Verify both docs persisted
    openDB();
    EXPECT_FALSE(readDocument("doc1").is_null());
    EXPECT_FALSE(readDocument("doc2").is_null());
    closeDB();
}
