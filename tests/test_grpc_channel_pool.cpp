#include <gtest/gtest.h>
#include "utils/grpc_channel_pool.h"
#include <thread>
#include <vector>

using namespace themis::utils;

class GrpcChannelPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        GrpcChannelPool::Config config;
        config.max_channels_per_target = 5;
        config.idle_timeout = std::chrono::seconds(2);
        pool = std::make_unique<GrpcChannelPool>(config);
    }
    
    std::unique_ptr<GrpcChannelPool> pool;
};

TEST_F(GrpcChannelPoolTest, AcquireAndReleaseChannel) {
    std::string target = "localhost:50051";
    
    auto channel = pool->acquireChannel(target);
    ASSERT_NE(channel, nullptr);
    EXPECT_EQ(pool->getStats().total_channels, 1);
    EXPECT_EQ(pool->getStats().in_use_channels, 1);
    
    pool->releaseChannel(target, channel);
    EXPECT_EQ(pool->getStats().available_channels, 1);
    EXPECT_EQ(pool->getStats().in_use_channels, 0);
}

TEST_F(GrpcChannelPoolTest, ReuseChannel) {
    std::string target = "localhost:50051";
    
    auto channel1 = pool->acquireChannel(target);
    pool->releaseChannel(target, channel1);
    
    auto channel2 = pool->acquireChannel(target);
    EXPECT_EQ(channel1, channel2); // Should reuse same channel
    EXPECT_EQ(pool->getStats().channels_reused, 1);
    
    pool->releaseChannel(target, channel2);
}

TEST_F(GrpcChannelPoolTest, MultipleTargets) {
    auto channel1 = pool->acquireChannel("localhost:50051");
    auto channel2 = pool->acquireChannel("localhost:50052");
    
    EXPECT_NE(channel1, channel2);
    EXPECT_EQ(pool->getStats().total_channels, 2);
    
    pool->releaseChannel("localhost:50051", channel1);
    pool->releaseChannel("localhost:50052", channel2);
}

TEST_F(GrpcChannelPoolTest, ConcurrentAccess) {
    std::string target = "localhost:50051";
    std::vector<std::thread> threads;
    std::atomic<size_t> success_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            try {
                auto channel = pool->acquireChannel(target);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                pool->releaseChannel(target, channel);
                success_count.fetch_add(1);
            } catch (...) {
                // Timeout expected if pool is full
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(success_count.load(), 0);
    EXPECT_LE(pool->getStats().total_channels, 5); // Should not exceed max
}

TEST_F(GrpcChannelPoolTest, StaleChannelPruning) {
    std::string target = "localhost:50051";
    
    auto channel = pool->acquireChannel(target);
    pool->releaseChannel(target, channel);
    
    // Wait for channel to become stale
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    pool->pruneStaleChannels();
    
    EXPECT_GT(pool->getStats().stale_channels_removed, 0);
}

TEST_F(GrpcChannelPoolTest, ClearPool) {
    pool->acquireChannel("localhost:50051");
    pool->acquireChannel("localhost:50052");
    
    EXPECT_EQ(pool->getStats().total_channels, 2);
    
    pool->clear();
    
    EXPECT_EQ(pool->getStats().total_channels, 0);
}
