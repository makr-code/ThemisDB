#include <gtest/gtest.h>

#include "sharding/consistent_hash.h"
#include "sharding/redundancy_strategy.h"
#include "sharding/shard_topology.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace themis::sharding;

namespace {

class MockShardStorage {
public:
    bool write(const std::string& shard_id,
               const std::string& doc_id,
               const std::vector<uint8_t>& data) {
        shard_data_[shard_id][doc_id] = data;
        return true;
    }

    std::optional<std::vector<uint8_t>> read(const std::string& shard_id,
                                             const std::string& doc_id) const {
        const auto shard_it = shard_data_.find(shard_id);
        if (shard_it == shard_data_.end()) {
            return std::nullopt;
        }
        const auto doc_it = shard_it->second.find(doc_id);
        if (doc_it == shard_it->second.end()) {
            return std::nullopt;
        }
        return doc_it->second;
    }

private:
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> shard_data_;
};

class RedundancyStrategyFocusedTest : public ::testing::Test {
protected:
    void SetUp() override {
        ring_ = std::make_unique<ConsistentHashRing>(100);
        for (int i = 0; i < 6; ++i) {
            const auto shard_id = std::string("shard-") + std::to_string(i);
            ring_->addNode(shard_id);

            ShardInfo shard;
            shard.shard_id = shard_id;
            shard.primary_endpoint = "localhost:" + std::to_string(9000 + i);
            shard.is_healthy = true;
            topology_.addShard(shard);
        }

        RedundancyConfig config;
        config.mode = RedundancyMode::MIRROR;
        config.replication_factor = 3;
        config.write_concern = WriteConcern::MAJORITY;

        strategy_ = std::make_unique<RedundancyStrategy>(config);
    }

    std::unique_ptr<ConsistentHashRing> ring_;
    ShardTopology topology_;
    MockShardStorage storage_;
    std::unique_ptr<RedundancyStrategy> strategy_;
};

}  // namespace

TEST_F(RedundancyStrategyFocusedTest, WriteRejectsEmptyDocumentId) {
    const std::vector<uint8_t> data{1, 2, 3, 4};
    const auto write_handler = [this](const std::string& shard_id,
                                      const std::string& doc_id,
                                      const std::vector<uint8_t>& payload) {
        return storage_.write(shard_id, doc_id, payload);
    };

    const auto result = strategy_->write("", data, "test_collection", *ring_, topology_, write_handler);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "document_id is empty");
}

TEST_F(RedundancyStrategyFocusedTest, WriteRejectsEmptyData) {
    const std::vector<uint8_t> empty_data;
    const auto write_handler = [this](const std::string& shard_id,
                                      const std::string& doc_id,
                                      const std::vector<uint8_t>& payload) {
        return storage_.write(shard_id, doc_id, payload);
    };

    const auto result = strategy_->write("doc123", empty_data, "test_collection", *ring_, topology_, write_handler);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "data is empty");
}

TEST_F(RedundancyStrategyFocusedTest, ReadRejectsEmptyDocumentId) {
    const auto read_handler = [this](const std::string& shard_id,
                                     const std::string& doc_id)
        -> std::optional<std::vector<uint8_t>> {
        return storage_.read(shard_id, doc_id);
    };

    const auto result = strategy_->read("", "test_collection", *ring_, topology_, read_handler);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "document_id is empty");
}

TEST_F(RedundancyStrategyFocusedTest, WriteMirrorRejectsZeroQuorumWithReplicas) {
    RedundancyConfig bad_config;
    bad_config.mode = RedundancyMode::MIRROR;
    bad_config.replication_factor = 3;
    bad_config.write_concern = WriteConcern::QUORUM;
    bad_config.enable_quorum_enforcement = true;
    bad_config.write_quorum = 0;

    auto bad_strategy = std::make_unique<RedundancyStrategy>(bad_config);
    const std::vector<uint8_t> data{1, 2, 3, 4};
    const auto write_handler = [this](const std::string& shard_id,
                                      const std::string& doc_id,
                                      const std::vector<uint8_t>& payload) {
        return storage_.write(shard_id, doc_id, payload);
    };

    const auto result = bad_strategy->write("doc123", data, "test_collection", *ring_, topology_, write_handler);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("write_quorum"), std::string::npos);
}
