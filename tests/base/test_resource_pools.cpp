#include "base/buffer_pool.h"
#include "base/resource_pool_manager.h"

#include <gtest/gtest.h>

namespace themis::resource {
namespace {

TEST(BufferPool, AcquiresAndReusesSlabBuffer) {
    BufferPool::Config config;
    config.initial_per_class = 1;
    config.max_per_class = 2;
    BufferPool pool(config);

    {
        auto buffer = pool.acquire(64);
        ASSERT_TRUE(buffer.valid());
        EXPECT_EQ(buffer.size(), 128u);
        EXPECT_EQ(pool.statistics().current_live, 1u);
    }

    EXPECT_EQ(pool.statistics().current_live, 0u);
    auto reused = pool.acquire(128);
    ASSERT_TRUE(reused.valid());
    EXPECT_EQ(reused.size(), 128u);
    EXPECT_EQ(pool.statistics().slab_hits, 2u);
}

TEST(BufferPool, UsesSystemAllocatorForOversizedRequest) {
    BufferPool pool;
    auto buffer = pool.acquire(BufferPool::kMaxSlabSize + 1);

    ASSERT_TRUE(buffer.valid());
    EXPECT_EQ(buffer.size(), BufferPool::kMaxSlabSize + 1);
    EXPECT_EQ(pool.statistics().os_fallbacks, 1u);
}

TEST(ResourcePoolManager, ExposesPoolStatisticsAndShutdown) {
    ResourcePoolManager::Config config;
    config.conn_pool.min_size = 1;
    config.conn_pool.max_size = 1;
    ResourcePoolManager manager(config);

    int slot_id = -1;
    ASSERT_TRUE(manager.connectionPool().acquire(std::chrono::milliseconds(0), slot_id));
    auto buffer = manager.bufferPool().acquire(64);
    ASSERT_TRUE(buffer.valid());

    const auto stats = manager.statistics();
    EXPECT_EQ(stats.conn.in_use, 1u);
    EXPECT_EQ(stats.buffer.current_live, 1u);

    manager.connectionPool().release(slot_id);
    manager.shutdown();
    EXPECT_TRUE(manager.is_shutdown());
    EXPECT_FALSE(manager.bufferPool().acquire(64).valid());
}

}  // namespace
}  // namespace themis::resource
