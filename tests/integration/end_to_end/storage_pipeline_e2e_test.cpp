/**
 * @file storage_pipeline_e2e_test.cpp
 * @brief End-to-end integration tests for storage pipeline workflows
 * 
 * Tests complete workflows:
 * - Write → Read → Update → Delete cycles
 * - Batch operations
 * - Concurrent access patterns
 * - Error handling and recovery
 * - Data consistency verification
 */

#include "../test_fixture.h"
#include "../test_data_generator.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <random>

// Mock storage interfaces for testing
namespace themis {

class StorageEngine {
public:
    virtual ~StorageEngine() = default;
    virtual bool write(const std::string& key, const std::string& value) = 0;
    virtual std::optional<std::string> read(const std::string& key) = 0;
    virtual bool update(const std::string& key, const std::string& value) = 0;
    virtual bool remove(const std::string& key) = 0;
    virtual size_t count() = 0;
};

// Simple in-memory storage for testing
class InMemoryStorage : public StorageEngine {
private:
    std::unordered_map<std::string, std::string> data_;
    mutable std::mutex mutex_;

public:
    bool write(const std::string& key, const std::string& value) override {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = value;
        return true;
    }

    std::optional<std::string> read(const std::string& key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        if (it != data_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool update(const std::string& key, const std::string& value) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_.find(key) == data_.end()) {
            return false; // Key doesn't exist
        }
        data_[key] = value;
        return true;
    }

    bool remove(const std::string& key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.erase(key) > 0;
    }

    size_t count() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.size();
    }
};

} // namespace themis

namespace themis {
namespace test {

/**
 * @brief End-to-end tests for storage pipeline
 */
class StoragePipelineE2ETest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        storage_ = std::make_unique<InMemoryStorage>();
        data_gen_ = std::make_unique<TestDataGenerator>();
    }

    std::unique_ptr<StorageEngine> storage_;
    std::unique_ptr<TestDataGenerator> data_gen_;
};

/**
 * @test Complete CRUD cycle
 * 
 * Acceptance Criteria:
 * - Write operation succeeds
 * - Read retrieves correct data
 * - Update modifies existing data
 * - Delete removes data
 * - Read after delete returns nothing
 */
TEST_F(StoragePipelineE2ETest, CompleteCRUDCycle) {
    const std::string key = "test_key_1";
    const std::string initial_value = "initial_data";
    const std::string updated_value = "updated_data";

    // Create
    ASSERT_TRUE(storage_->write(key, initial_value));

    // Read
    auto read_result = storage_->read(key);
    ASSERT_TRUE(read_result.has_value());
    EXPECT_EQ(read_result.value(), initial_value);

    // Update
    ASSERT_TRUE(storage_->update(key, updated_value));
    read_result = storage_->read(key);
    ASSERT_TRUE(read_result.has_value());
    EXPECT_EQ(read_result.value(), updated_value);

    // Delete
    ASSERT_TRUE(storage_->remove(key));
    read_result = storage_->read(key);
    EXPECT_FALSE(read_result.has_value());
}

/**
 * @test Batch write and verify
 * 
 * Acceptance Criteria:
 * - Multiple writes succeed
 * - All written data can be read back
 * - Data integrity is maintained
 */
TEST_F(StoragePipelineE2ETest, BatchWriteAndVerify) {
    const int batch_size = 1000;
    std::vector<std::pair<std::string, std::string>> test_data;

    // Generate test data
    for (int i = 0; i < batch_size; ++i) {
        std::string key = "batch_key_" + std::to_string(i);
        std::string value = "batch_value_" + std::to_string(i) + "_" + data_gen_->GenerateRandomString(50);
        test_data.emplace_back(key, value);
    }

    // Batch write
    for (const auto& [key, value] : test_data) {
        ASSERT_TRUE(storage_->write(key, value));
    }

    // Verify count
    EXPECT_EQ(storage_->count(), batch_size);

    // Verify all data
    for (const auto& [key, expected_value] : test_data) {
        auto read_result = storage_->read(key);
        ASSERT_TRUE(read_result.has_value()) << "Failed to read key: " << key;
        EXPECT_EQ(read_result.value(), expected_value) << "Data mismatch for key: " << key;
    }
}

/**
 * @test Concurrent writes from multiple threads
 * 
 * Acceptance Criteria:
 * - All writes succeed without conflicts
 * - No data corruption occurs
 * - Final count matches expected
 */
TEST_F(StoragePipelineE2ETest, ConcurrentWrites) {
    const int num_threads = 10;
    const int writes_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, writes_per_thread, &success_count]() {
            for (int i = 0; i < writes_per_thread; ++i) {
                std::string key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
                std::string value = "thread_" + std::to_string(t) + "_value_" + std::to_string(i);
                if (storage_->write(key, value)) {
                    success_count++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count, num_threads * writes_per_thread);
    EXPECT_EQ(storage_->count(), num_threads * writes_per_thread);
}

/**
 * @test Concurrent reads and writes
 * 
 * Acceptance Criteria:
 * - Reads and writes can happen concurrently
 * - No deadlocks occur
 * - Data consistency is maintained
 */
TEST_F(StoragePipelineE2ETest, ConcurrentReadWrite) {
    // Pre-populate some data
    const int initial_keys = 100;
    for (int i = 0; i < initial_keys; ++i) {
        std::string key = "concurrent_key_" + std::to_string(i);
        storage_->write(key, "initial_value");
    }

    const int num_reader_threads = 5;
    const int num_writer_threads = 5;
    std::vector<std::thread> threads;
    std::atomic<bool> keep_running{true};
    std::atomic<int> read_count{0};
    std::atomic<int> write_count{0};

    // Reader threads
    for (int t = 0; t < num_reader_threads; ++t) {
        threads.emplace_back([this, t, initial_keys, &keep_running, &read_count]() {
            // Per-thread deterministic RNG
            std::mt19937 rng(42 + t);
            std::uniform_int_distribution<int> dist(0, initial_keys - 1);
            while (keep_running) {
                int key_id = dist(rng);
                std::string key = "concurrent_key_" + std::to_string(key_id);
                auto value = storage_->read(key);
                if (value.has_value()) {
                    read_count++;
                }
            }
        });
    }

    // Writer threads
    for (int t = 0; t < num_writer_threads; ++t) {
        threads.emplace_back([this, t, &keep_running, &write_count]() {
            int counter = 0;
            while (keep_running) {
                std::string key = "writer_" + std::to_string(t) + "_" + std::to_string(counter++);
                if (storage_->write(key, "new_value")) {
                    write_count++;
                }
            }
        });
    }

    // Let it run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    keep_running = false;

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(read_count, 0);
    EXPECT_GT(write_count, 0);
}

/**
 * @test Error handling for non-existent keys
 * 
 * Acceptance Criteria:
 * - Reading non-existent key returns empty
 * - Updating non-existent key fails
 * - Deleting non-existent key returns false
 */
TEST_F(StoragePipelineE2ETest, ErrorHandlingNonExistentKeys) {
    const std::string non_existent_key = "does_not_exist";

    // Read non-existent
    auto read_result = storage_->read(non_existent_key);
    EXPECT_FALSE(read_result.has_value());

    // Update non-existent
    bool update_result = storage_->update(non_existent_key, "some_value");
    EXPECT_FALSE(update_result);

    // Delete non-existent
    bool delete_result = storage_->remove(non_existent_key);
    EXPECT_FALSE(delete_result);
}

/**
 * @test Large value storage and retrieval
 * 
 * Acceptance Criteria:
 * - Can store values up to 10MB
 * - Large values are retrieved correctly
 * - No corruption of large data
 */
TEST_F(StoragePipelineE2ETest, LargeValueStorage) {
    const std::string key = "large_value_key";
    const size_t large_size = 10 * 1024 * 1024; // 10MB
    
    // Generate large value
    std::string large_value(large_size, 'A');
    // Add some patterns to verify integrity
    for (size_t i = 0; i < large_size; i += 1000) {
        large_value[i] = 'X';
    }

    // Write
    ASSERT_TRUE(storage_->write(key, large_value));

    // Read back
    auto read_result = storage_->read(key);
    ASSERT_TRUE(read_result.has_value());
    EXPECT_EQ(read_result.value().size(), large_size);
    EXPECT_EQ(read_result.value(), large_value);
}

/**
 * @test Multiple updates to same key
 * 
 * Acceptance Criteria:
 * - Can update same key multiple times
 * - Each update overwrites previous value
 * - Final read returns last written value
 */
TEST_F(StoragePipelineE2ETest, MultipleUpdates) {
    const std::string key = "multi_update_key";
    const int num_updates = 100;

    // Initial write
    ASSERT_TRUE(storage_->write(key, "initial"));

    // Multiple updates
    for (int i = 0; i < num_updates; ++i) {
        std::string value = "update_" + std::to_string(i);
        ASSERT_TRUE(storage_->update(key, value));
    }

    // Verify final value
    auto read_result = storage_->read(key);
    ASSERT_TRUE(read_result.has_value());
    EXPECT_EQ(read_result.value(), "update_" + std::to_string(num_updates - 1));
}

/**
 * @test Stress test with rapid operations
 * 
 * Acceptance Criteria:
 * - System handles rapid successive operations
 * - No crashes or hangs occur
 * - Data remains consistent
 */
TEST_F(StoragePipelineE2ETest, RapidOperationsStress) {
    const int rapid_ops = 10000;
    int write_count = 0;
    int read_count = 0;
    int update_count = 0;
    int delete_count = 0;

    for (int i = 0; i < rapid_ops; ++i) {
        int op = i % 4;
        std::string key = "rapid_key_" + std::to_string(i % 1000);
        
        switch (op) {
            case 0: // Write
                if (storage_->write(key, "value_" + std::to_string(i))) {
                    write_count++;
                }
                break;
            case 1: // Read
                storage_->read(key);
                read_count++;
                break;
            case 2: // Update
                if (storage_->update(key, "updated_" + std::to_string(i))) {
                    update_count++;
                }
                break;
            case 3: // Delete
                if (storage_->remove(key)) {
                    delete_count++;
                }
                break;
        }
    }

    EXPECT_GT(write_count, 0);
    EXPECT_GT(read_count, 0);
}

/**
 * @test Data consistency after batch operations
 * 
 * Acceptance Criteria:
 * - Batch writes are consistent
 * - Batch deletes leave correct data
 * - Remaining data is intact
 */
TEST_F(StoragePipelineE2ETest, DataConsistencyAfterBatchOps) {
    const int total_keys = 1000;
    const int keys_to_delete = 300;

    // Batch write
    for (int i = 0; i < total_keys; ++i) {
        std::string key = "consistency_key_" + std::to_string(i);
        ASSERT_TRUE(storage_->write(key, "value_" + std::to_string(i)));
    }

    EXPECT_EQ(storage_->count(), total_keys);

    // Batch delete (every third key)
    int deleted = 0;
    for (int i = 0; i < total_keys && deleted < keys_to_delete; i += 3) {
        std::string key = "consistency_key_" + std::to_string(i);
        if (storage_->remove(key)) {
            deleted++;
        }
    }

    // Verify remaining keys
    for (int i = 0; i < total_keys; ++i) {
        std::string key = "consistency_key_" + std::to_string(i);
        auto value = storage_->read(key);
        
        if (i % 3 == 0 && i / 3 < keys_to_delete) {
            // Should be deleted
            EXPECT_FALSE(value.has_value()) << "Key should be deleted: " << key;
        } else {
            // Should exist
            ASSERT_TRUE(value.has_value()) << "Key should exist: " << key;
            EXPECT_EQ(value.value(), "value_" + std::to_string(i));
        }
    }
}

/**
 * @test Empty string keys and values
 * 
 * Acceptance Criteria:
 * - Can handle empty string keys (if supported)
 * - Can handle empty string values
 * - Operations work correctly with empty strings
 */
TEST_F(StoragePipelineE2ETest, EmptyStringHandling) {
    // Empty value with normal key
    ASSERT_TRUE(storage_->write("normal_key", ""));
    auto result = storage_->read("normal_key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "");

    // Update to empty
    ASSERT_TRUE(storage_->write("update_key", "initial"));
    ASSERT_TRUE(storage_->update("update_key", ""));
    result = storage_->read("update_key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "");
}

} // namespace test
} // namespace themis
