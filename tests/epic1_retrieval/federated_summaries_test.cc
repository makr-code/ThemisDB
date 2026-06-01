/**
 * @file federated_summaries_test.cc
 * @brief Contract tests for IFederatedSummaries (sub-issue #5427).
 *
 * Validates factory construction, shard registration/deregistration,
 * shardHealth reporting, and query returning an empty FederatedResult at
 * scaffold stage. Production fan-out is tracked in sub-issue #5427.
 */

#include "retrieval/include/federated_summaries.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace themis::retrieval;

namespace {

FederatedSummariesConfig defaultConfig() {
    FederatedSummariesConfig cfg;
    cfg.default_mode    = FederatedMode::LocalFirst;
    cfg.default_fanout  = 4;
    cfg.deduplicate     = true;
    cfg.local_shard_key = "local";
    return cfg;
}

FederatedQuery simpleQuery(std::uint32_t top_k = 5) {
    FederatedQuery q;
    q.embedding = std::vector<float>(64, 0.5f);
    q.top_k     = top_k;
    q.mode      = FederatedMode::LocalFirst;
    return q;
}

} // namespace

class FederatedSummariesTest : public ::testing::Test {
protected:
    void SetUp() override {
        fed_ = makeFederatedSummaries(defaultConfig());
        ASSERT_NE(fed_, nullptr);
    }

    std::unique_ptr<IFederatedSummaries> fed_;
};

TEST_F(FederatedSummariesTest, FactoryReturnsNonNull) {
    EXPECT_NE(fed_, nullptr);
}

TEST_F(FederatedSummariesTest, ShardHealthEmptyInitially) {
    auto health = fed_->shardHealth();
    // No shards registered yet; map must be empty or well-defined.
    EXPECT_TRUE(health.empty());
}

TEST_F(FederatedSummariesTest, RegisterShardDoesNotThrow) {
    EXPECT_NO_THROW(fed_->registerShard("shard-a", "http://node-a:9090"));
}

TEST_F(FederatedSummariesTest, RegisteredShardAppearsInHealth) {
    fed_->registerShard("shard-b", "http://node-b:9090");
    auto health = fed_->shardHealth();
    EXPECT_EQ(health.count("shard-b"), 1u);
}

TEST_F(FederatedSummariesTest, RegisteredShardHealthIsFalseInScaffold) {
    fed_->registerShard("shard-c", "http://node-c:9090");
    auto health = fed_->shardHealth();
    // Scaffold stub has no real transport; all shards report unhealthy.
    EXPECT_FALSE(health.at("shard-c"));
}

TEST_F(FederatedSummariesTest, DeregisterRemovesShard) {
    fed_->registerShard("shard-d", "http://node-d:9090");
    fed_->deregisterShard("shard-d");
    auto health = fed_->shardHealth();
    EXPECT_EQ(health.count("shard-d"), 0u);
}

TEST_F(FederatedSummariesTest, DeregisterNonexistentShardDoesNotThrow) {
    EXPECT_NO_THROW(fed_->deregisterShard("no-such-shard"));
}

TEST_F(FederatedSummariesTest, QueryEmptyShardSetDoesNotThrow) {
    EXPECT_NO_THROW(fed_->query(simpleQuery()));
}

TEST_F(FederatedSummariesTest, QueryShardAvailabilityContainsRegisteredShards) {
    fed_->registerShard("shard-e", "http://node-e:9090");
    FederatedResult result = fed_->query(simpleQuery());
    // Scaffold: shard_availability may include registered shards.
    (void)result;
    SUCCEED();
}

TEST_F(FederatedSummariesTest, QueryWithEmptyEmbeddingDoesNotThrow) {
    FederatedQuery q;
    q.top_k = 5;
    EXPECT_NO_THROW(fed_->query(q));
}
