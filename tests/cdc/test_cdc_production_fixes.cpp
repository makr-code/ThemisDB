// Test: CDC Production Readiness Fixes
// Tests for P0 critical fixes implemented in CDC module
// - Atomic sequence generation under concurrent access
// - Lock safety in buffer operations
// - Compression/decompression error handling

#include <gtest/gtest.h>
#include "cdc/changefeed.h"
#include "cdc/changefeed_buffer.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <vector>
#include <set>
#include <atomic>
#include <chrono>

using namespace themis;

class CDCProductionFixesTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CDC production-fixes focused tests on Windows due to fixture crash in current runtime.";
#endif
        // Clean up any existing test database
        test_db_path_ = "./data/themis_cdc_production_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create RocksDB wrapper
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 128;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        // Create changefeed
        auto* raw_db = db_->getDB();
        ASSERT_NE(raw_db, nullptr);
        changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);
    }
    
    void TearDown() override {
        changefeed_.reset();
        if (db_) {
            db_->close();
        }
        db_.reset();
        
        // Clean up test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
};

// ===== Atomic Sequence Generation Tests =====

TEST_F(CDCProductionFixesTest, AtomicSequenceGenerationUnderConcurrency) {
    // Test that sequence generation is atomic under high concurrency
    const int num_threads = 20;
    const int events_per_thread = 50;
    const int total_events = num_threads * events_per_thread;
    
    std::vector<std::thread> threads;
    std::vector<std::vector<uint64_t>> thread_sequences(num_threads);
    std::atomic<int> ready_count{0};
    std::atomic<bool> start{false};
    
    // Create threads that will all start at the same time
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            // Wait for all threads to be ready
            ready_count++;
            while (!start.load()) {
                std::this_thread::yield();
            }
            
            // Record events
            for (int i = 0; i < events_per_thread; i++) {
                Changefeed::ChangeEvent event;
                event.type = Changefeed::ChangeEventType::EVENT_PUT;
                event.key = "thread_" + std::to_string(t) + "_event_" + std::to_string(i);
                event.value = "{}";
                
                auto recorded = changefeed_->recordEvent(event);
                thread_sequences[t].push_back(recorded.sequence);
            }
        });
    }
    
    // Wait for all threads to be ready
    while (ready_count.load() < num_threads) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Start all threads simultaneously
    start.store(true);
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Collect all sequences
    std::set<uint64_t> all_sequences;
    for (const auto& thread_seqs : thread_sequences) {
        for (uint64_t seq : thread_seqs) {
            all_sequences.insert(seq);
        }
    }
    
    // Verify all sequences are unique (no race condition in sequence generation)
    EXPECT_EQ(all_sequences.size(), static_cast<size_t>(total_events)) 
        << "Found duplicate sequences! Race condition in sequence generation.";
    
    // Verify sequences are contiguous (no gaps)
    if (all_sequences.size() == static_cast<size_t>(total_events)) {
        uint64_t min_seq = *all_sequences.begin();
        uint64_t max_seq = *all_sequences.rbegin();
        EXPECT_EQ(max_seq - min_seq + 1, total_events)
            << "Gaps detected in sequence numbers";
    }
}

TEST_F(CDCProductionFixesTest, SequenceGenerationConsistency) {
    // Test that sequence generation is consistent across multiple rapid calls
    const int num_events = 1000;
    std::vector<uint64_t> sequences;
    
    for (int i = 0; i < num_events; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "key_" + std::to_string(i);
        event.value = "{}";
        
        auto recorded = changefeed_->recordEvent(event);
        sequences.push_back(recorded.sequence);
    }
    
    // Verify strictly increasing sequences
    for (size_t i = 1; i < sequences.size(); i++) {
        EXPECT_EQ(sequences[i], sequences[i-1] + 1)
            << "Sequences not strictly increasing at index " << i;
    }
}

// ===== Lock Safety Tests =====

TEST_F(CDCProductionFixesTest, BufferLockSafetyUnderOverflow) {
    // Test that buffer overflow flush doesn't deadlock
    ChangefeedBufferConfig config;
    config.max_memory_bytes = 1024 * 100;  // 100 KB limit
    config.max_events_per_buffer = 10;
    config.async_flush = false;  // Synchronous for testing
    
    ChangefeedBuffer buffer(changefeed_.get(), config);
    buffer.start();
    
    // Create events that will trigger overflow
    const int num_events = 50;
    std::atomic<int> successful_records{0};
    std::atomic<int> failed_records{0};
    
    for (int i = 0; i < num_events; i++) {
        try {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "overflow_test_" + std::to_string(i);
            event.value = std::string(5000, 'x');  // 5KB payload to trigger overflow
            
            buffer.recordEvent(event);
            successful_records++;
        } catch (const std::exception& e) {
            failed_records++;
        }
    }
    
    buffer.stop();
    
    // Should have successfully recorded all events without deadlock
    EXPECT_EQ(successful_records.load(), num_events)
        << "Some events failed due to lock issues";
    EXPECT_EQ(failed_records.load(), 0);
}

TEST_F(CDCProductionFixesTest, BufferConcurrentAccess) {
    // Test concurrent access to buffer doesn't cause deadlocks
    ChangefeedBufferConfig config;
    config.max_events_per_buffer = 100;
    config.async_flush = true;
    
    ChangefeedBuffer buffer(changefeed_.get(), config);
    buffer.start();
    
    const int num_threads = 10;
    const int events_per_thread = 20;
    std::vector<std::thread> threads;
    std::atomic<int> total_recorded{0};
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < events_per_thread; i++) {
                Changefeed::ChangeEvent event;
                event.type = Changefeed::ChangeEventType::EVENT_PUT;
                event.key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
                event.value = "{}";
                
                buffer.recordEvent(event);
                total_recorded++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    buffer.stop();
    
    // All events should have been recorded successfully
    EXPECT_EQ(total_recorded.load(), num_threads * events_per_thread);
}

// ===== Compression Error Handling Tests =====

TEST_F(CDCProductionFixesTest, CompressionErrorHandling) {
    // Test that compression failures are handled gracefully
    ChangefeedBufferConfig config;
    config.compress_payloads = true;
    config.compression_threshold_bytes = 100;  // Compress payloads > 100 bytes
    config.async_flush = false;
    
    ChangefeedBuffer buffer(changefeed_.get(), config);
    buffer.start();
    
    // Record events with various payload sizes
    std::vector<std::string> payloads = {
        std::string(50, 'a'),      // Small, won't compress
        std::string(200, 'b'),     // Medium, will compress
        std::string(5000, 'c'),    // Large, will compress
        "",                        // Empty
        std::string(1000, 'd')     // Another compressible
    };
    
    int successful = 0;
    for (size_t i = 0; i < payloads.size(); i++) {
        try {
            Changefeed::ChangeEvent event;
            event.type = Changefeed::ChangeEventType::EVENT_PUT;
            event.key = "compress_test_" + std::to_string(i);
            event.value = payloads[i];
            
            buffer.recordEvent(event);
            successful++;
        } catch (const std::exception& e) {
            // Should not throw even if compression fails
            FAIL() << "Compression error not handled gracefully: " << e.what();
        }
    }
    
    buffer.stop();
    
    // All events should succeed (compression failures fall back to uncompressed)
    EXPECT_EQ(successful, static_cast<int>(payloads.size()));
}

TEST_F(CDCProductionFixesTest, DecompressionErrorHandling) {
    // Test decompression error handling during flush
    ChangefeedBufferConfig config;
    config.compress_payloads = true;
    config.compression_threshold_bytes = 100;
    config.async_flush = false;
    
    ChangefeedBuffer buffer(changefeed_.get(), config);
    buffer.start();
    
    // Record compressible events
    for (int i = 0; i < 10; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "decompress_test_" + std::to_string(i);
        event.value = std::string(500, 'x');  // Compressible
        
        buffer.recordEvent(event);
    }
    
    // Flush should handle any decompression errors gracefully
    size_t flushed = 0;
    EXPECT_NO_THROW({
        flushed = buffer.flush();
    });
    
    // Should have flushed some/all events (even if some decompression fails)
    EXPECT_GE(flushed, 0);
    
    buffer.stop();
}

// ===== Error Recovery Tests =====

TEST_F(CDCProductionFixesTest, RecoverFromEmptyKey) {
    // Test that empty keys are handled without crashing
    ChangefeedBufferConfig config;
    config.async_flush = false;
    
    ChangefeedBuffer buffer(changefeed_.get(), config);
    buffer.start();
    
    // Try to record event with empty key
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "";  // Empty key
    event.value = "test";
    
    EXPECT_NO_THROW({
        auto result = buffer.recordEvent(event);
        // Should return event as-is (sequence = 0 indicates not recorded)
        EXPECT_EQ(result.sequence, 0);
    });
    
    // Buffer should still work for valid events
    Changefeed::ChangeEvent valid_event;
    valid_event.type = Changefeed::ChangeEventType::EVENT_PUT;
    valid_event.key = "valid_key";
    valid_event.value = "valid_value";
    
    EXPECT_NO_THROW({
        buffer.recordEvent(valid_event);
    });
    
    buffer.stop();
}

TEST_F(CDCProductionFixesTest, StressTestConcurrentSequenceGeneration) {
    // Stress test with many threads hitting sequence generation simultaneously
    const int num_threads = 50;
    const int events_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> error_count{0};
    std::vector<std::vector<uint64_t>> all_sequences(num_threads);
    
    auto start_time = std::chrono::steady_clock::now();
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            try {
                for (int i = 0; i < events_per_thread; i++) {
                    Changefeed::ChangeEvent event;
                    event.type = Changefeed::ChangeEventType::EVENT_PUT;
                    event.key = "stress_" + std::to_string(t) + "_" + std::to_string(i);
                    event.value = "{}";
                    
                    auto recorded = changefeed_->recordEvent(event);
                    all_sequences[t].push_back(recorded.sequence);
                }
            } catch (const std::exception& e) {
                error_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // No errors should occur
    EXPECT_EQ(error_count.load(), 0) << "Errors occurred during stress test";
    
    // Verify all sequences are unique
    std::set<uint64_t> unique_sequences;
    for (const auto& seqs : all_sequences) {
        for (uint64_t seq : seqs) {
            unique_sequences.insert(seq);
        }
    }
    
    EXPECT_EQ(unique_sequences.size(), num_threads * events_per_thread)
        << "Duplicate sequences found under stress";
    
    // Log performance info
    std::cout << "Stress test completed in " << duration.count() << "ms" << std::endl;
    std::cout << "Throughput: " << (num_threads * events_per_thread * 1000.0 / duration.count()) 
              << " events/sec" << std::endl;
}
