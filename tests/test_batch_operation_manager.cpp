#include <gtest/gtest.h>
#include "utils/batch_operation_manager.h"
#include <vector>
#include <atomic>

using namespace themis::utils;

class BatchOperationManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        processed_items.clear();
        batch_count.store(0);
    }
    
    std::vector<int> processed_items;
    std::atomic<size_t> batch_count{0};
    
    size_t testProcessor(const std::vector<int>& items) {
        batch_count.fetch_add(1);
        processed_items.insert(processed_items.end(), items.begin(), items.end());
        return items.size();
    }
};

TEST_F(BatchOperationManagerTest, BasicBatching) {
    BatchOperationManager<int>::Config config;
    config.min_batch_size = 5;
    config.max_batch_size = 10;
    config.max_latency = std::chrono::milliseconds(50);
    config.adaptive_sizing = false;
    
    BatchOperationManager<int> manager(config, [this](const auto& items) {
        return testProcessor(items);
    });
    
    manager.start();
    
    // Enqueue items
    for (int i = 0; i < 15; ++i) {
        EXPECT_TRUE(manager.enqueue(i));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    manager.stop();
    
    EXPECT_EQ(processed_items.size(), 15);
    EXPECT_GE(batch_count.load(), 1);
}

TEST_F(BatchOperationManagerTest, FlushPending) {
    BatchOperationManager<int>::Config config;
    config.max_latency = std::chrono::seconds(10); // Long latency
    
    BatchOperationManager<int> manager(config, [this](const auto& items) {
        return testProcessor(items);
    });
    
    // Enqueue without starting
    for (int i = 0; i < 5; ++i) {
        manager.enqueue(i);
    }
    
    size_t flushed = manager.flush();
    EXPECT_EQ(flushed, 5);
    EXPECT_EQ(processed_items.size(), 5);
}

TEST_F(BatchOperationManagerTest, EnqueueBatch) {
    BatchOperationManager<int>::Config config;
    
    BatchOperationManager<int> manager(config, [this](const auto& items) {
        return testProcessor(items);
    });
    
    std::vector<int> items = {1, 2, 3, 4, 5};
    size_t enqueued = manager.enqueueBatch(items);
    
    EXPECT_EQ(enqueued, 5);
    
    manager.flush();
    EXPECT_EQ(processed_items.size(), 5);
}

TEST_F(BatchOperationManagerTest, QueueCapacity) {
    BatchOperationManager<int>::Config config;
    config.queue_capacity = 10;
    
    BatchOperationManager<int> manager(config, [this](const auto& items) {
        // Slow processor
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return testProcessor(items);
    });
    
    // Try to enqueue more than capacity
    size_t success = 0;
    for (int i = 0; i < 20; ++i) {
        if (manager.enqueue(i)) {
            success++;
        }
    }
    
    EXPECT_LE(success, 10);
}

TEST_F(BatchOperationManagerTest, AdaptiveSizing) {
    BatchOperationManager<int>::Config config;
    config.adaptive_sizing = true;
    config.min_batch_size = 10;
    config.max_batch_size = 100;
    config.initial_batch_size = 20;
    config.max_latency = std::chrono::milliseconds(50);
    
    BatchOperationManager<int> manager(config, [this](const auto& items) {
        return testProcessor(items);
    });
    
    size_t initial_size = manager.getCurrentBatchSize();
    EXPECT_EQ(initial_size, 20);
    
    manager.start();
    
    // Enqueue many items
    for (int i = 0; i < 200; ++i) {
        manager.enqueue(i);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    manager.stop();
    
    // Adaptive sizing should adjust batch size
    auto stats = manager.getStats();
    EXPECT_GT(stats.batches_processed, 0);
    EXPECT_EQ(stats.items_processed, 200);
}

TEST_F(BatchOperationManagerTest, Statistics) {
    BatchOperationManager<int>::Config config;
    
    BatchOperationManager<int> manager(config, [this](const auto& items) {
        return testProcessor(items);
    });
    
    manager.start();
    
    for (int i = 0; i < 50; ++i) {
        manager.enqueue(i);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    manager.stop();
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.items_processed, 50);
    EXPECT_GT(stats.batches_processed, 0);
    EXPECT_GT(stats.avg_batch_size, 0);
}

TEST_F(BatchOperationManagerTest, ConcurrentEnqueue) {
    BatchOperationManager<int>::Config config;
    
    BatchOperationManager<int> manager(config, [this](const auto& items) {
        return testProcessor(items);
    });
    
    manager.start();
    
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 20; ++i) {
                manager.enqueue(t * 100 + i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    manager.stop();
    
    EXPECT_EQ(processed_items.size(), 100);
}
