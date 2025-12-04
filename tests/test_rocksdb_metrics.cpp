#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include "storage/rocksdb_wrapper.h"

using namespace themis;

class RocksDBMetricsTest : public ::testing::Test {
protected:
    std::string db_path = "data/test_rocksdb_metrics";
    std::shared_ptr<RocksDBWrapper> db;
    
    void SetUp() override {
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = db_path;
        config.block_cache_size_mb = 256;
        
        db = std::make_shared<RocksDBWrapper>(config);
        ASSERT_TRUE(db->open());
    }
    
    void TearDown() override {
        db->close();
    }
};

TEST_F(RocksDBMetricsTest, ExportMetricsDoesntCrash) {
    // Populate some data
    for (int i = 0; i < 1000; ++i) {
        db->put("key_" + std::to_string(i), "value_" + std::to_string(i));
    }
    
    // Export metrics - should not throw or crash
    EXPECT_NO_THROW({
        db->exportMetricsToTelemetry();
    });
}

TEST_F(RocksDBMetricsTest, GetStatsReturnsValidJSON) {
    // Populate data
    for (int i = 0; i < 500; ++i) {
        db->put("test_key_" + std::to_string(i), "test_val_" + std::to_string(i));
    }
    
    std::string stats = db->getStats();
    
    // Verify it contains JSON-like structure
    EXPECT_NE(stats.find("{"), std::string::npos);
    EXPECT_NE(stats.find("}"), std::string::npos);
    EXPECT_NE(stats.find("rocksdb"), std::string::npos);
}

TEST_F(RocksDBMetricsTest, CompressionTypeQuery) {
    std::string compression = db->getCompressionType();
    
    // Should return a valid compression type
    EXPECT_FALSE(compression.empty());
    EXPECT_TRUE(compression.find("default=") != std::string::npos);
}

TEST_F(RocksDBMetricsTest, ConcurrentMetricsExport) {
    // Populate data
    for (int i = 0; i < 5000; ++i) {
        db->put("concurrent_key_" + std::to_string(i), "val");
    }
    
    // Export metrics from multiple threads
    std::vector<std::thread> threads;
    std::atomic<int> export_count{0};
    
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, &export_count]() {
            for (int i = 0; i < 10; ++i) {
                db->exportMetricsToTelemetry();
                export_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(export_count, 40);
}

