#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include "cdc/changefeed.h"
#include "server/sse_connection_manager.h"
#include "storage/rocksdb_wrapper.h"

namespace {

class SseConnectionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("test_sse_connection_manager_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
                       .string();
        std::filesystem::create_directories(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.memtable_size_mb = 16;
        cfg.block_cache_size_mb = 16;
        storage_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        themis::Changefeed::RetentionPolicy rp;
        rp.enabled = false;
        changefeed_ = std::make_shared<themis::Changefeed>(storage_->getRawDB(), nullptr, rp);
    }

    void TearDown() override {
        changefeed_.reset();
        storage_->close();
        std::filesystem::remove_all(db_path_);
    }

    std::string db_path_;
    std::shared_ptr<themis::RocksDBWrapper> storage_;
    std::shared_ptr<themis::Changefeed> changefeed_;
    boost::asio::io_context ioc_;
};

TEST_F(SseConnectionManagerTest, UnregisterConnectionRemovesByIdAndIncrementsDisconnectCount) {
    themis::server::SseConnectionManager manager(changefeed_, ioc_);
    const auto conn_id = manager.registerConnection(/*from_seq=*/0);

    auto before = manager.getStats();
    EXPECT_EQ(before.active_connections, 1u);
    EXPECT_EQ(before.total_disconnects, 0u);

    manager.unregisterConnection(conn_id);
    auto after_first = manager.getStats();
    EXPECT_EQ(after_first.active_connections, 0u);
    EXPECT_EQ(after_first.total_disconnects, 1u);

    manager.unregisterConnection(conn_id);
    auto after_second = manager.getStats();
    EXPECT_EQ(after_second.active_connections, 0u);
    EXPECT_EQ(after_second.total_disconnects, 1u);
}

TEST_F(SseConnectionManagerTest, UnregisterUnknownConnectionDoesNotChangeStats) {
    themis::server::SseConnectionManager manager(changefeed_, ioc_);

    const auto before = manager.getStats();
    manager.unregisterConnection(/*conn_id=*/424242);
    const auto after = manager.getStats();

    EXPECT_EQ(after.active_connections, before.active_connections);
    EXPECT_EQ(after.total_disconnects, before.total_disconnects);
}

} // namespace
