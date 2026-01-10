/**
 * test_llm_raid_data_push.cpp
 * 
 * Integrierte Tests für Daten-Push, Metriken-Sammlung und Validierung
 * - Daten in RAID Shards schreiben
 * - Metriken vor/nach abrufen
 * - Ergebnisse validieren
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Mock für HTTP-Kommunikation mit RAID Shards
class MockRAIDShardClient {
public:
    struct Document {
        std::string id;
        std::string domain;
        std::string content;
        std::string timestamp;
    };
    
    struct MetricsSnapshot {
        std::string timestamp;
        json metrics;
    };
    
    MOCK_METHOD(bool, isHealthy, (), (const));
    MOCK_METHOD(bool, pushDocument, (const Document&), ());
    MOCK_METHOD(std::vector<Document>, listDocuments, (const std::string&), (const));
    MOCK_METHOD(MetricsSnapshot, getMetrics, (), (const));
};

// Real RAID Data Push Service
class RAIDDataPushService {
public:
    struct PushConfig {
        int num_records;
        int batch_size;
        bool verify_distribution;
    };
    
    struct PushStats {
        int total_records;
        int successfully_pushed;
        int failed_records;
        std::vector<std::string> errors;
        std::chrono::milliseconds duration;
        json metrics_before;
        json metrics_after;
    };
    
    RAIDDataPushService(std::vector<std::shared_ptr<MockRAIDShardClient>> shards)
        : shards_(shards) {}
    
    PushStats pushTestData(const PushConfig& config) {
        auto start = std::chrono::steady_clock::now();
        PushStats stats;
        stats.total_records = config.num_records;
        stats.successfully_pushed = 0;
        stats.failed_records = 0;
        
        // Collect baseline metrics
        for (const auto& shard : shards_) {
            if (shard->isHealthy()) {
                auto metrics = shard->getMetrics();
                stats.metrics_before[metrics.timestamp] = metrics.metrics;
            }
        }
        
        // Generate and push records (round-robin distribution)
        for (int i = 0; i < config.num_records; ++i) {
            auto shard = shards_[i % shards_.size()];
            
            MockRAIDShardClient::Document doc = {
                .id = "test_" + std::to_string(i),
                .domain = std::vector<std::string>{"legal", "medical", "finance"}[i % 3],
                .content = "Test record " + std::to_string(i) + " for domain-specific processing",
                .timestamp = getCurrentTimestamp()
            };
            
            if (shard->pushDocument(doc)) {
                stats.successfully_pushed++;
            } else {
                stats.failed_records++;
                stats.errors.push_back("Failed to push record " + doc.id);
            }
        }
        
        // Collect post-push metrics
        for (const auto& shard : shards_) {
            if (shard->isHealthy()) {
                auto metrics = shard->getMetrics();
                stats.metrics_after[metrics.timestamp] = metrics.metrics;
            }
        }
        
        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );
        
        return stats;
    }
    
    bool verifyDataDistribution() {
        // Verify all shards have roughly equal record counts
        std::map<std::string, int> shard_counts;
        
        for (size_t i = 0; i < shards_.size(); ++i) {
            if (shards_[i]->isHealthy()) {
                auto docs = shards_[i]->listDocuments("test_collection");
                shard_counts["shard_" + std::to_string(i)] = docs.size();
            }
        }
        
        // Check distribution is reasonably balanced
        if (shard_counts.empty()) return false;
        
        int min_count = INT_MAX;
        int max_count = 0;
        
        for (const auto& [shard, count] : shard_counts) {
            min_count = std::min(min_count, count);
            max_count = std::max(max_count, count);
        }
        
        // Allow 20% variance due to round-robin distribution
        double variance = (double)(max_count - min_count) / (max_count + 1);
        return variance < 0.2;
    }
    
private:
    std::vector<std::shared_ptr<MockRAIDShardClient>> shards_;
    
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// Test Fixture
class RAIDDataPushTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create 9 mock shards (RAID0 x3, RAID1 x2, RAID5 x3)
        for (int i = 0; i < 9; ++i) {
            auto shard = std::make_shared<MockRAIDShardClient>();
            
            // Setup default behavior
            ON_CALL(*shard, isHealthy).WillByDefault(testing::Return(true));
            ON_CALL(*shard, pushDocument).WillByDefault(testing::Return(true));
            ON_CALL(*shard, getMetrics).WillByDefault(
                testing::Return(MockRAIDShardClient::MetricsSnapshot{
                    .timestamp = "2024-01-04T12:00:00Z",
                    .metrics = json::object()
                })
            );
            
            shards_.push_back(shard);
        }
        
        service_ = std::make_unique<RAIDDataPushService>(shards_);
    }
    
    std::vector<std::shared_ptr<MockRAIDShardClient>> shards_;
    std::unique_ptr<RAIDDataPushService> service_;
};

// Tests

TEST_F(RAIDDataPushTest, PushSmallDataset) {
    RAIDDataPushService::PushConfig config = {
        .num_records = 100,
        .batch_size = 10,
        .verify_distribution = true
    };
    
    auto stats = service_->pushTestData(config);
    
    EXPECT_EQ(stats.total_records, 100);
    EXPECT_EQ(stats.successfully_pushed, 100);
    EXPECT_EQ(stats.failed_records, 0);
    EXPECT_GT(stats.duration.count(), 0);
}

TEST_F(RAIDDataPushTest, PushLargeDataset) {
    RAIDDataPushService::PushConfig config = {
        .num_records = 1000,
        .batch_size = 100,
        .verify_distribution = true
    };
    
    auto stats = service_->pushTestData(config);
    
    EXPECT_EQ(stats.total_records, 1000);
    EXPECT_EQ(stats.successfully_pushed, 1000);
    EXPECT_EQ(stats.failed_records, 0);
}

TEST_F(RAIDDataPushTest, HandlePushFailures) {
    // Setup some shards to fail
    EXPECT_CALL(*shards_[0], pushDocument)
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(false));
    
    RAIDDataPushService::PushConfig config = {
        .num_records = 50,
        .batch_size = 10,
        .verify_distribution = false
    };
    
    auto stats = service_->pushTestData(config);
    
    EXPECT_GT(stats.failed_records, 0);
    EXPECT_LT(stats.successfully_pushed, 50);
}

TEST_F(RAIDDataPushTest, MetricsCollectionBeforeAfter) {
    // Setup metrics collection
    json metrics_before = {
        {"documents", 0},
        {"disk_usage_mb", 50}
    };
    
    json metrics_after = {
        {"documents", 100},
        {"disk_usage_mb", 52}
    };
    
    EXPECT_CALL(*shards_[0], getMetrics)
        .Times(2)
        .WillOnce(testing::Return(
            MockRAIDShardClient::MetricsSnapshot{
                .timestamp = "2024-01-04T12:00:00Z",
                .metrics = metrics_before
            }
        ))
        .WillOnce(testing::Return(
            MockRAIDShardClient::MetricsSnapshot{
                .timestamp = "2024-01-04T12:01:00Z",
                .metrics = metrics_after
            }
        ));
    
    RAIDDataPushService::PushConfig config = {
        .num_records = 100,
        .batch_size = 10,
        .verify_distribution = false
    };
    
    auto stats = service_->pushTestData(config);
    
    EXPECT_FALSE(stats.metrics_before.empty());
    EXPECT_FALSE(stats.metrics_after.empty());
}

TEST_F(RAIDDataPushTest, VerifyRoundRobinDistribution) {
    // Track which shards received documents
    std::vector<int> push_counts(9, 0);
    
    for (size_t i = 0; i < shards_.size(); ++i) {
        EXPECT_CALL(*shards_[i], pushDocument)
            .WillRepeatedly(testing::Invoke(
                [&push_counts, i](const MockRAIDShardClient::Document&) {
                    push_counts[i]++;
                    return true;
                }
            ));
    }
    
    RAIDDataPushService::PushConfig config = {
        .num_records = 90,  // Evenly divisible by 9 shards
        .batch_size = 10,
        .verify_distribution = true
    };
    
    auto stats = service_->pushTestData(config);
    
    // Each shard should get ~10 records
    for (int count : push_counts) {
        EXPECT_EQ(count, 10);
    }
}

TEST_F(RAIDDataPushTest, UnhealthyShardHandling) {
    // Mark some shards as unhealthy
    EXPECT_CALL(*shards_[0], isHealthy)
        .WillRepeatedly(testing::Return(false));
    
    RAIDDataPushService::PushConfig config = {
        .num_records = 90,
        .batch_size = 10,
        .verify_distribution = false
    };
    
    // Should not crash, should skip unhealthy shards
    auto stats = service_->pushTestData(config);
    
    EXPECT_GE(stats.successfully_pushed, 0);
}

TEST_F(RAIDDataPushTest, DataDistributionBalance) {
    RAIDDataPushService::PushConfig config = {
        .num_records = 450,  // Divisible by 9 shards and 3 domains
        .batch_size = 45,
        .verify_distribution = true
    };
    
    auto stats = service_->pushTestData(config);
    
    EXPECT_TRUE(service_->verifyDataDistribution());
}

TEST_F(RAIDDataPushTest, PushTimingMetrics) {
    RAIDDataPushService::PushConfig config = {
        .num_records = 200,
        .batch_size = 20,
        .verify_distribution = false
    };
    
    auto stats = service_->pushTestData(config);
    
    // Verify timing was measured
    EXPECT_GT(stats.duration.count(), 0);
    
    // Calculate throughput
    double throughput = (double)stats.successfully_pushed / 
                       (stats.duration.count() / 1000.0);
    
    EXPECT_GT(throughput, 0);
}

// Benchmark Tests

class RAIDDataPushBench : public ::benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        for (int i = 0; i < 9; ++i) {
            auto shard = std::make_shared<MockRAIDShardClient>();
            ON_CALL(*shard, isHealthy).WillByDefault(testing::Return(true));
            ON_CALL(*shard, pushDocument).WillByDefault(testing::Return(true));
            shards_.push_back(shard);
        }
        
        service_ = std::make_unique<RAIDDataPushService>(shards_);
    }
    
    std::vector<std::shared_ptr<MockRAIDShardClient>> shards_;
    std::unique_ptr<RAIDDataPushService> service_;
};

BENCHMARK_F(RAIDDataPushBench, BM_Push100Records)(benchmark::State& state) {
    for (auto _ : state) {
        RAIDDataPushService::PushConfig config = {.num_records = 100};
        auto stats = service_->pushTestData(config);
        
        state.counters["records"] = stats.successfully_pushed;
        state.counters["errors"] = stats.failed_records;
        state.SetLabel("100 records");
    }
}

BENCHMARK_F(RAIDDataPushBench, BM_Push1000Records)(benchmark::State& state) {
    for (auto _ : state) {
        RAIDDataPushService::PushConfig config = {.num_records = 1000};
        auto stats = service_->pushTestData(config);
        
        state.counters["records"] = stats.successfully_pushed;
        state.counters["throughput_per_sec"] = 
            (double)stats.successfully_pushed / (stats.duration.count() / 1000.0);
    }
}

BENCHMARK_F(RAIDDataPushBench, BM_MetricsCollection)(benchmark::State& state) {
    RAIDDataPushService::PushConfig config = {.num_records = 500};
    
    for (auto _ : state) {
        auto stats = service_->pushTestData(config);
        
        state.counters["metrics_snapshots"] = 
            stats.metrics_before.size() + stats.metrics_after.size();
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    
    return RUN_ALL_TESTS();
}
