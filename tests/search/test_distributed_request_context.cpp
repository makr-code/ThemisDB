/**
 * @file test_distributed_request_context.cpp
 * @brief Request-context (deadline + cancellation) propagation through
 *        DistributedHybridSearch fan-out.
 *
 * Uses DistributedHybridSearch::setShardSearchFunction to drive deterministic
 * shard behavior. Real in-flight transport cancellation is covered separately
 * by tests/sharding/test_mtls_client_cancellation.cpp.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "search/distributed_hybrid_search.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"

namespace themis {
namespace {

using namespace std::chrono_literals;
using Search = DistributedHybridSearch;

struct Fixture {
    std::shared_ptr<sharding::ShardTopology> topology =
        std::make_shared<sharding::ShardTopology>();
    std::shared_ptr<sharding::ConsistentHashRing> ring =
        std::make_shared<sharding::ConsistentHashRing>();
    std::unique_ptr<sharding::URNResolver> resolver;

    explicit Fixture(size_t shards) {
        for (size_t i = 0; i < shards; ++i) {
            sharding::ShardInfo info{};
            info.shard_id = "shard_" + std::to_string(i);
            info.primary_endpoint = "127.0.0.1:" + std::to_string(9000 + i);
            info.is_healthy = true;
            topology->addShard(info);
        }
        resolver = std::make_unique<sharding::URNResolver>(topology, ring, "");
    }
};

HybridSearch::Result doc(const std::string& id) {
    HybridSearch::Result r;
    r.document_id = id;
    r.hybrid_score = 1.0;
    return r;
}

Search::ShardSearchResult okShard(const std::string& id, std::vector<std::string> docs) {
    Search::ShardSearchResult r;
    r.shard_id = id;
    r.success = true;
    for (auto& d : docs) r.results.push_back(doc(d));
    return r;
}

Search::Config baseConfig(size_t concurrency = 10) {
    Search::Config c;
    c.max_concurrent_shards = concurrency;
    c.shard_timeout_ms = 5000;
    c.local_shard_id = "local";
    return c;
}

}  // namespace

TEST(DistributedRequestContextTest, ShardBudgetIsClampedByRequestDeadline) {
    Fixture fx(2);
    Search search(nullptr, fx.resolver.get(), nullptr, baseConfig());

    std::mutex m;
    std::vector<std::chrono::milliseconds> remaining;
    search.setShardSearchFunction(
        [&](const sharding::ShardInfo& s, const std::string&, const std::vector<float>&,
            size_t, const query::RequestContext& ctx) {
            std::lock_guard<std::mutex> lk(m);
            remaining.push_back(ctx.remaining().value_or(std::chrono::hours(1)));
            return okShard(s.shard_id, {"d"});
        });

    Search::SearchStats stats;
    // Request deadline (300 ms) is tighter than shard_timeout_ms (5000 ms).
    search.search("q", {}, query::RequestContext::withTimeout(300ms), &stats);
    ASSERT_EQ(remaining.size(), 2U);
    for (auto r : remaining) EXPECT_LE(r.count(), 300);

    // Unbounded request: the per-shard timeout becomes the budget.
    remaining.clear();
    search.search("q", {}, query::RequestContext{}, &stats);
    ASSERT_EQ(remaining.size(), 2U);
    for (auto r : remaining) {
        EXPECT_LE(r.count(), 5000);
        EXPECT_GT(r.count(), 0);
    }
}

TEST(DistributedRequestContextTest, PreCancelledRequestLaunchesNothing) {
    Fixture fx(4);
    Search search(nullptr, fx.resolver.get(), nullptr, baseConfig());

    std::atomic<int> calls{0};
    search.setShardSearchFunction([&](const sharding::ShardInfo& s, const std::string&,
                                      const std::vector<float>&, size_t,
                                      const query::RequestContext&) {
        ++calls;
        return okShard(s.shard_id, {"d"});
    });

    auto token = std::make_shared<query::QueryCancellationToken>();
    token->cancel();
    Search::SearchStats stats;
    const auto results = search.search("q", {}, query::RequestContext(token), &stats);

    EXPECT_EQ(calls.load(), 0);
    EXPECT_TRUE(results.empty());
    EXPECT_TRUE(stats.cancelled);
    EXPECT_FALSE(stats.deadline_exceeded);
    EXPECT_EQ(stats.shards_skipped, 5U);  // local + 4 remote never started
    EXPECT_EQ(stats.shards_queried, 0U);
}

TEST(DistributedRequestContextTest, CancelBetweenBatchesStopsLaterBatchesAndKeepsPartialResults) {
    Fixture fx(6);
    Search search(nullptr, fx.resolver.get(), nullptr, baseConfig(/*concurrency=*/2));

    auto token = std::make_shared<query::QueryCancellationToken>();
    std::atomic<int> calls{0};
    search.setShardSearchFunction([&](const sharding::ShardInfo& s, const std::string&,
                                      const std::vector<float>&, size_t,
                                      const query::RequestContext&) {
        ++calls;
        token->cancel();  // client goes away while the first batch is running
        return okShard(s.shard_id, {"doc_" + s.shard_id});
    });

    Search::SearchStats stats;
    const auto results = search.search("q", {}, query::RequestContext(token), &stats);

    EXPECT_EQ(calls.load(), 2);                 // only the first batch was launched
    EXPECT_EQ(stats.shards_skipped, 4U);
    EXPECT_TRUE(stats.cancelled);
    EXPECT_TRUE(stats.partial_result);          // success + skipped
    EXPECT_FALSE(results.empty());              // completed shards are kept
}

TEST(DistributedRequestContextTest, StrictModeReturnsEmptyWhenShardsSkipped) {
    Fixture fx(4);
    auto cfg = baseConfig(2);
    cfg.skip_failed_shards = false;
    Search search(nullptr, fx.resolver.get(), nullptr, cfg);

    auto token = std::make_shared<query::QueryCancellationToken>();
    search.setShardSearchFunction([&](const sharding::ShardInfo& s, const std::string&,
                                      const std::vector<float>&, size_t,
                                      const query::RequestContext&) {
        token->cancel();
        return okShard(s.shard_id, {"x"});
    });
    const auto results = search.search("q", {}, query::RequestContext(token));
    EXPECT_TRUE(results.empty());
}

TEST(DistributedRequestContextTest, InFlightShardObservesCancelAndAllTasksAreJoined) {
    Fixture fx(3);
    Search search(nullptr, fx.resolver.get(), nullptr, baseConfig());

    auto token = std::make_shared<query::QueryCancellationToken>();
    std::atomic<int> started{0};
    std::atomic<int> finished{0};
    search.setShardSearchFunction([&](const sharding::ShardInfo& s, const std::string&,
                                      const std::vector<float>&, size_t,
                                      const query::RequestContext& ctx) {
        ++started;
        struct Done { std::atomic<int>& c; ~Done() { ++c; } } done{finished};
        Search::ShardSearchResult r;
        r.shard_id = s.shard_id;
        // Cooperative "transport": poll the context like MTLSClient does.
        const auto limit = std::chrono::steady_clock::now() + 10s;
        while (std::chrono::steady_clock::now() < limit) {
            if (const auto reason = ctx.stopReason(); reason != query::StopReason::None) {
                r.success = false;
                r.stop_reason = reason;
                r.error_msg = "aborted";
                return r;
            }
            std::this_thread::sleep_for(2ms);
        }
        r.success = true;
        return r;
    });

    std::thread canceller([&] {
        std::this_thread::sleep_for(50ms);
        token->cancel();
    });

    Search::SearchStats stats;
    const auto t0 = std::chrono::steady_clock::now();
    const auto results = search.search("q", {}, query::RequestContext(token), &stats);
    const auto elapsed = std::chrono::steady_clock::now() - t0;
    canceller.join();

    EXPECT_LT(elapsed, 2s);
    EXPECT_TRUE(results.empty());
    EXPECT_EQ(stats.shards_cancelled, 3U);
    EXPECT_TRUE(stats.cancelled);
    // Cleanup: every launched task has completed by the time search() returns.
    EXPECT_EQ(started.load(), 3);
    EXPECT_EQ(finished.load(), 3);
}

TEST(DistributedRequestContextTest, ShardDeadlineIsTimeoutNotEmptyResult) {
    Fixture fx(2);
    auto cfg = baseConfig();
    cfg.shard_timeout_ms = 40;
    Search search(nullptr, fx.resolver.get(), nullptr, cfg);

    search.setShardSearchFunction([&](const sharding::ShardInfo& s, const std::string&,
                                      const std::vector<float>&, size_t,
                                      const query::RequestContext& ctx) {
        if (s.shard_id == "shard_0") {
            return okShard(s.shard_id, {});  // healthy but empty
        }
        Search::ShardSearchResult r;
        r.shard_id = s.shard_id;
        while (!ctx.shouldStop()) std::this_thread::sleep_for(2ms);
        r.success = false;
        r.stop_reason = ctx.stopReason();
        r.error_msg = "shard timeout";
        return r;
    });

    Search::SearchStats stats;
    search.search("q", {}, query::RequestContext{}, &stats);

    EXPECT_EQ(stats.shards_succeeded, 2U);  // local (empty) + shard_0 (empty)
    EXPECT_EQ(stats.shards_failed, 1U);
    EXPECT_EQ(stats.shards_timed_out, 1U);  // timeout reported, distinct from empty
    EXPECT_TRUE(stats.deadline_exceeded);
    EXPECT_FALSE(stats.cancelled);
    EXPECT_TRUE(stats.partial_result);
}

TEST(DistributedRequestContextTest, OrdinaryShardFailureIsPartialWithoutStopFlags) {
    Fixture fx(3);
    Search search(nullptr, fx.resolver.get(), nullptr, baseConfig());
    search.setShardSearchFunction([&](const sharding::ShardInfo& s, const std::string&,
                                      const std::vector<float>&, size_t,
                                      const query::RequestContext&) {
        if (s.shard_id == "shard_1") {
            Search::ShardSearchResult r;
            r.shard_id = s.shard_id;
            r.success = false;
            r.error_msg = "HTTP 500";
            return r;
        }
        return okShard(s.shard_id, {"doc_" + s.shard_id});
    });

    Search::SearchStats stats;
    const auto results = search.search("q", {}, query::RequestContext{}, &stats);
    EXPECT_EQ(results.size(), 2U);
    EXPECT_TRUE(stats.partial_result);
    EXPECT_EQ(stats.shards_failed, 1U);
    EXPECT_FALSE(stats.cancelled);
    EXPECT_FALSE(stats.deadline_exceeded);
    ASSERT_EQ(stats.failed_shard_reasons.size(), 1U);
}

TEST(DistributedRequestContextTest, LegacyOverloadStillWorks) {
    Fixture fx(2);
    Search search(nullptr, fx.resolver.get(), nullptr, baseConfig());
    search.setShardSearchFunction([&](const sharding::ShardInfo& s, const std::string&,
                                      const std::vector<float>&, size_t,
                                      const query::RequestContext& ctx) {
        EXPECT_FALSE(ctx.shouldStop());
        return okShard(s.shard_id, {"doc_" + s.shard_id});
    });
    Search::SearchStats stats;
    const auto results = search.search("q", {}, &stats);
    EXPECT_EQ(results.size(), 2U);
    EXPECT_FALSE(stats.partial_result);
}

}  // namespace themis
