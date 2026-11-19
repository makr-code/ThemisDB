#include "base/buffer_pool.h"
#include "base/resource_pool_manager.h"

#include <gtest/gtest.h>

#include <chrono>
#include <limits>
#include <stdexcept>
#include <utility>

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
    {
        auto buffer = pool.acquire(BufferPool::kMaxSlabSize + 1);

        ASSERT_TRUE(buffer.valid());
        EXPECT_EQ(buffer.size(), BufferPool::kMaxSlabSize + 1);
        EXPECT_EQ(pool.statistics().os_fallbacks, 1u);
        EXPECT_EQ(pool.statistics().current_live, 1u);

        auto moved = std::move(buffer);
        EXPECT_FALSE(buffer.valid());
        EXPECT_TRUE(moved.valid());
        EXPECT_EQ(pool.statistics().current_live, 1u);
    }
    EXPECT_EQ(pool.statistics().current_live, 0u);
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

TEST(AdaptiveConnectionPool, RejectsInvalidMinimumAndMaximumSizes) {
    AdaptiveConnectionPool::Config config;
    config.min_size = 2;
    config.max_size = 1;

    EXPECT_THROW(
        { AdaptiveConnectionPool pool(config); },
        std::invalid_argument);

    config.min_size = 0;
    config.max_size =
        static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1;
    EXPECT_THROW(
        { AdaptiveConnectionPool pool(config); },
        std::invalid_argument);
}

TEST(AdaptiveConnectionPool, RejectsUnacquiredAndDuplicateSlotRelease) {
    AdaptiveConnectionPool::Config config;
    config.min_size = 1;
    config.max_size = 1;
    AdaptiveConnectionPool pool(config);

    int slot_id = -1;
    ASSERT_TRUE(pool.acquire(std::chrono::milliseconds(0), slot_id));

    EXPECT_THROW(pool.release(slot_id + 1), std::invalid_argument);
    EXPECT_EQ(pool.available(), 0u);
    EXPECT_EQ(pool.in_use(), 1u);

    pool.release(slot_id);
    EXPECT_EQ(pool.available(), 1u);
    EXPECT_EQ(pool.in_use(), 0u);

    EXPECT_THROW(pool.release(slot_id), std::invalid_argument);
    const auto stats = pool.statistics();
    EXPECT_EQ(stats.pool_size, 1u);
    EXPECT_EQ(stats.available, 1u);
    EXPECT_EQ(stats.in_use, 0u);
}

}  // namespace
}  // namespace themis::resource
