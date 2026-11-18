#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "query/request_context.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace {

struct PollState {
    std::atomic<unsigned int> polls{0};
    unsigned int stop_after = 0;
};

bool stopAfterPolls(const void* opaque) noexcept {
    auto* state = const_cast<PollState*>(static_cast<const PollState*>(opaque));
    const auto poll = state->polls.fetch_add(1, std::memory_order_relaxed) + 1;
    return state->stop_after != 0 && poll >= state->stop_after;
}

class SecondaryIndexRequestCancellationTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = std::filesystem::temp_directory_path() /
                   ("themis_secondary_index_cancel_" + std::to_string(suffix));

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        index_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        ASSERT_TRUE(index_->createIndex("records", "group").ok);
        ASSERT_TRUE(index_->createRangeIndex("records", "rank").ok);
        ASSERT_TRUE(index_->createFulltextIndex("records", "content").ok);
        for (int i = 0; i < 128; ++i) {
            themis::BaseEntity entity("record-" + std::to_string(i));
            entity.setField("group", "all");
            entity.setField("rank", std::to_string(i));
            entity.setField(
                "content", i % 2 == 0 ? "deadline phrase" : "other phrase");
            ASSERT_TRUE(index_->put("records", entity).ok);
        }
    }

    void TearDown() override {
        index_.reset();
        db_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> index_;
};

TEST_F(SecondaryIndexRequestCancellationTest,
       EqualityScanStopsDuringIterationAndDiscardsPartialKeys) {
    PollState state;
    state.stop_after = 3;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    auto stop_reason = themis::query::StopReason::None;

    auto [status, keys] = index_->scanKeysEqual(
        "records", "group", "all", request, &stop_reason);

    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(keys.empty());
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_EQ(state.polls.load(std::memory_order_relaxed), 3u);
}

TEST_F(SecondaryIndexRequestCancellationTest,
       SequentialQueryPropagatesStopFromEqualityIndexScan) {
    themis::query::QueryEngine engine(*db_, *index_);
    PollState state;
    state.stop_after = 5;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    auto stop_reason = themis::query::StopReason::None;

    auto keys = engine.executeAndKeysSequential(
        "records", {{"group", "all"}}, request, &stop_reason);

    EXPECT_FALSE(keys);
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_GE(state.polls.load(std::memory_order_relaxed), state.stop_after);
}

TEST_F(SecondaryIndexRequestCancellationTest,
       FallbackQueryUsesRequestAwareEqualityIndexScan) {
    themis::query::QueryEngine engine(*db_, *index_);
    themis::query::ConjunctiveQuery query;
    query.table = "records";
    query.predicates.push_back({"group", "all"});

    PollState state;
    state.stop_after = 13;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    auto stop_reason = themis::query::StopReason::None;

    auto keys =
        engine.executeAndKeysWithFallback(query, true, request, &stop_reason);

    EXPECT_FALSE(keys);
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_GE(state.polls.load(std::memory_order_relaxed), state.stop_after);
}

TEST_F(SecondaryIndexRequestCancellationTest,
       NonOptimizedFallbackUsesRequestAwareEqualityIndexScan) {
    themis::query::QueryEngine engine(*db_, *index_);
    themis::query::ConjunctiveQuery query;
    query.table = "records";
    query.predicates.push_back({"group", "all"});

    PollState state;
    state.stop_after = 12;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    auto stop_reason = themis::query::StopReason::None;

    auto keys =
        engine.executeAndKeysWithFallback(query, false, request, &stop_reason);

    EXPECT_FALSE(keys);
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_GE(state.polls.load(std::memory_order_relaxed), state.stop_after);
}

TEST_F(SecondaryIndexRequestCancellationTest,
       RequestAwareFallbackPreservesPhrasePredicateSemantics) {
    themis::query::QueryEngine engine(*db_, *index_);
    themis::query::ConjunctiveQuery query;
    query.table = "records";
    query.predicates.push_back({"group", "all"});
    query.phrasePredicate =
        themis::query::PredicatePhrase{"content", "deadline phrase", 1000};

    auto keys = engine.executeAndKeysWithFallback(
        query, true, themis::query::RequestContext{});

    ASSERT_TRUE(keys);
    EXPECT_EQ(keys->size(), 64u);
}

TEST_F(SecondaryIndexRequestCancellationTest,
       RangeScanStopsDuringIterationAndDiscardsPartialKeys) {
    PollState state;
    state.stop_after = 3;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    auto stop_reason = themis::query::StopReason::None;

    auto [status, keys] = index_->scanKeysRange(
        "records",
        "rank",
        std::nullopt,
        std::nullopt,
        true,
        true,
        1000,
        false,
        request,
        &stop_reason);

    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(keys.empty());
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_EQ(state.polls.load(std::memory_order_relaxed), 3u);
}

TEST_F(SecondaryIndexRequestCancellationTest,
       StructuralQueryPropagatesStopFromRangeIndexScan) {
    themis::query::QueryEngine engine(*db_, *index_);
    themis::query::ConjunctiveQuery query;
    query.table = "records";
    query.rangePredicates.push_back(
        themis::query::PredicateRange{"rank", std::nullopt, std::nullopt});

    PollState state;
    state.stop_after = 10;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    auto stop_reason = themis::query::StopReason::None;

    auto keys =
        engine.executeAndKeysWithFallback(query, true, request, &stop_reason);

    EXPECT_FALSE(keys);
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_GE(state.polls.load(std::memory_order_relaxed), state.stop_after);
}

TEST_F(SecondaryIndexRequestCancellationTest,
       ExpiredRangeDeadlineReturnsNoKeys) {
    const themis::query::RequestContext request(
        nullptr,
        themis::query::RequestContext::Clock::now() -
            std::chrono::milliseconds{1});
    auto stop_reason = themis::query::StopReason::None;

    auto [status, keys] = index_->scanKeysRange(
        "records",
        "rank",
        std::nullopt,
        std::nullopt,
        true,
        true,
        1000,
        false,
        request,
        &stop_reason);

    EXPECT_FALSE(status.ok);
    EXPECT_TRUE(keys.empty());
    EXPECT_EQ(stop_reason, themis::query::StopReason::DeadlineExceeded);
}

} // namespace
