// Cross-cluster federated AQL unit tests
//
// Tests CrossClusterFederator: cluster registration, cost estimation,
// execution plan creation, parallel query dispatch, and result merging.
// HTTP transport is replaced with an injectable mock.

#include <gtest/gtest.h>
#include <atomic>
#include <string>
#include <vector>

#include "query/cross_cluster_federation.h"

using namespace themis::query;

// ─── helpers ────────────────────────────────────────────────────────────────

/// Build a minimal ClusterEndpoint for testing.
static ClusterEndpoint makeEndpoint(const std::string& id,
                                    const std::string& url = "http://localhost:8080",
                                    uint64_t row_hint = 0,
                                    double latency_hint = 0.0) {
    ClusterEndpoint ep;
    ep.cluster_id              = id;
    ep.base_url                = url;
    ep.estimated_rows_hint     = row_hint;
    ep.network_latency_hint_ms = latency_hint;
    return ep;
}

/// Mock HTTP POST that always returns a fixed JSON response.
static CrossClusterFederator::HttpPostFn makeOkMock(
    const std::string& json_response,
    int                status_code = 200) {
    return [json_response, status_code](
               const std::string& /*url*/,
               const std::string& /*body*/,
               const std::string& /*auth*/,
               uint32_t           /*timeout_ms*/,
               std::string&       response) -> int {
        response = json_response;
        return status_code;
    };
}

/// Mock HTTP POST that always fails (transport error → returns 0).
static CrossClusterFederator::HttpPostFn makeFailMock() {
    return [](const std::string&, const std::string&, const std::string&,
              uint32_t, std::string&) -> int { return 0; };
}

// ════════════════════════════════════════════════════════════════════════════
// Cluster registry tests
// ════════════════════════════════════════════════════════════════════════════

TEST(CrossClusterFederatorTest, RegisterAndList) {
    CrossClusterFederator fed;
    EXPECT_EQ(fed.listClusters().size(), 0u);

    fed.registerCluster(makeEndpoint("c1", "http://c1:8080"));
    fed.registerCluster(makeEndpoint("c2", "http://c2:8080"));
    EXPECT_EQ(fed.listClusters().size(), 2u);
}

TEST(CrossClusterFederatorTest, RegisterReplaces) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1", "http://old:8080"));
    fed.registerCluster(makeEndpoint("c1", "http://new:8080"));

    auto clusters = fed.listClusters();
    ASSERT_EQ(clusters.size(), 1u);
    EXPECT_EQ(clusters[0].base_url, "http://new:8080");
}

TEST(CrossClusterFederatorTest, Unregister) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1"));
    fed.registerCluster(makeEndpoint("c2"));
    fed.unregisterCluster("c1");
    EXPECT_EQ(fed.listClusters().size(), 1u);
}

TEST(CrossClusterFederatorTest, UnregisterUnknown_NoOp) {
    CrossClusterFederator fed;
    EXPECT_NO_THROW(fed.unregisterCluster("nonexistent"));
}

TEST(CrossClusterFederatorTest, RegisterEmptyIdThrows) {
    CrossClusterFederator fed;
    ClusterEndpoint ep;
    ep.cluster_id = "";
    ep.base_url   = "http://x:8080";
    EXPECT_THROW(fed.registerCluster(ep), std::invalid_argument);
}

TEST(CrossClusterFederatorTest, RegisterEmptyUrlThrows) {
    CrossClusterFederator fed;
    ClusterEndpoint ep;
    ep.cluster_id = "c1";
    ep.base_url   = "";
    EXPECT_THROW(fed.registerCluster(ep), std::invalid_argument);
}

TEST(CrossClusterFederatorTest, RegisterInvalidUrlSchemeThrows) {
    CrossClusterFederator fed;
    for (const std::string& bad_url : {"file:///etc/passwd", "ftp://host/path",
                                       "ws://host:8080", "no-scheme"}) {
        ClusterEndpoint ep;
        ep.cluster_id = "c1";
        ep.base_url   = bad_url;
        EXPECT_THROW(fed.registerCluster(ep), std::invalid_argument)
            << "expected throw for url: " << bad_url;
    }
}

TEST(CrossClusterFederatorTest, RegisterHttpAndHttpsUrlAccepted) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1", "http://c1:8080"));
    fed.registerCluster(makeEndpoint("c2", "https://c2:443"));
    EXPECT_EQ(fed.listClusters().size(), 2u);
}

TEST(CrossClusterFederatorTest, RegisterAuthTokenWithCrLfThrows) {
    CrossClusterFederator fed;
    for (const std::string& bad_token : {"abc\rdef", "abc\ndef", "abc\r\nx"}) {
        ClusterEndpoint ep = makeEndpoint("c1", "https://c1:443");
        ep.auth_token = bad_token;
        EXPECT_THROW(fed.registerCluster(ep), std::invalid_argument)
            << "expected throw for token with control chars";
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Cost estimation tests
// ════════════════════════════════════════════════════════════════════════════

TEST(CrossClusterFederatorTest, EstimateCosts_NoClusters) {
    CrossClusterFederator fed;
    auto costs = fed.estimateCosts("FOR doc IN col RETURN doc");
    EXPECT_TRUE(costs.empty());
}

TEST(CrossClusterFederatorTest, EstimateCosts_UsesRowHint) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1", "http://c1:8080", /*rows=*/50'000));
    fed.registerCluster(makeEndpoint("c2", "http://c2:8080", /*rows=*/10'000));

    auto costs = fed.estimateCosts("FOR doc IN col RETURN doc");
    ASSERT_EQ(costs.size(), 2u);

    // c2 has fewer rows → should be cheapest
    EXPECT_EQ(costs[0].cluster_id, "c2");
    EXPECT_EQ(costs[0].estimated_rows, 10'000u);
    EXPECT_LT(costs[0].total_cost, costs[1].total_cost);
}

TEST(CrossClusterFederatorTest, EstimateCosts_UsesLatencyHint) {
    CrossClusterFederator fed;
    // Same row count, different latency
    fed.registerCluster(makeEndpoint("near", "http://near:8080", 10'000, 1.0));
    fed.registerCluster(makeEndpoint("far",  "http://far:8080",  10'000, 100.0));

    auto costs = fed.estimateCosts("FOR doc IN col RETURN doc");
    ASSERT_EQ(costs.size(), 2u);

    EXPECT_EQ(costs[0].cluster_id, "near");
    EXPECT_LT(costs[0].total_cost, costs[1].total_cost);
}

TEST(CrossClusterFederatorTest, EstimateCosts_DefaultsWhenNoHint) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1")); // no hints

    auto costs = fed.estimateCosts("FOR doc IN col RETURN doc");
    ASSERT_EQ(costs.size(), 1u);
    EXPECT_GT(costs[0].estimated_rows, 0u);
    EXPECT_GT(costs[0].network_latency_ms, 0.0);
    EXPECT_GT(costs[0].total_cost, 0.0);
    EXPECT_TRUE(costs[0].should_include);
}

// ════════════════════════════════════════════════════════════════════════════
// Execution plan tests
// ════════════════════════════════════════════════════════════════════════════

TEST(CrossClusterFederatorTest, ExecutionPlan_AllIncludedByDefault) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1"));
    fed.registerCluster(makeEndpoint("c2"));
    fed.registerCluster(makeEndpoint("c3"));

    auto plan = fed.createExecutionPlan("FOR doc IN col RETURN doc");
    EXPECT_EQ(plan.selected_clusters.size(), 3u);
    EXPECT_EQ(plan.merge_strategy, "union");
    EXPECT_GT(plan.total_estimated_cost, 0.0);
}

TEST(CrossClusterFederatorTest, ExecutionPlan_CostPruning) {
    CrossClusterFederator::Config cfg;
    cfg.cost_pruning_factor = 2.0; // prune clusters more than 2x the cheapest
    CrossClusterFederator fed(cfg);

    // c1 is very cheap (low rows, low latency), c2 is much more expensive
    fed.registerCluster(makeEndpoint("c1", "http://c1:8080",     100, 1.0));
    fed.registerCluster(makeEndpoint("c2", "http://c2:8080", 1'000'000, 200.0));

    auto plan = fed.createExecutionPlan("FOR doc IN col RETURN doc");

    // c2 should be pruned because its cost >> 2 * c1's cost
    EXPECT_EQ(plan.selected_clusters.size(), 1u);
    EXPECT_EQ(plan.selected_clusters[0], "c1");
}

TEST(CrossClusterFederatorTest, ExecutionPlan_NoClusters) {
    CrossClusterFederator fed;
    auto plan = fed.createExecutionPlan("FOR doc IN col RETURN doc");
    EXPECT_TRUE(plan.selected_clusters.empty());
    EXPECT_DOUBLE_EQ(plan.total_estimated_cost, 0.0);
}

// ════════════════════════════════════════════════════════════════════════════
// Execute tests (with HTTP mock)
// ════════════════════════════════════════════════════════════════════════════

TEST(CrossClusterFederatorTest, Execute_NoClusters_ReturnsEmptyArray) {
    CrossClusterFederator fed;
    auto result = fed.execute("FOR doc IN col RETURN doc");
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 0u);
}

TEST(CrossClusterFederatorTest, Execute_SingleCluster_ResultsArray) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1"));
    fed.setHttpPostForTesting(makeOkMock(R"([{"id":1},{"id":2}])"));

    auto result = fed.execute("FOR doc IN col RETURN doc");

    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0]["id"], 1);
    EXPECT_EQ(result[1]["id"], 2);
}

TEST(CrossClusterFederatorTest, Execute_SingleCluster_EnvelopeResponse) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1"));
    // Server wraps results in an envelope {"results": [...]}
    fed.setHttpPostForTesting(
        makeOkMock(R"({"results":[{"name":"Alice"},{"name":"Bob"}]})"));

    auto result = fed.execute("FOR doc IN col RETURN doc");
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2u);
}

TEST(CrossClusterFederatorTest, Execute_TwoClusters_ResultsMerged) {
    CrossClusterFederator::Config cfg;
    cfg.enable_parallel_execution = false; // sequential for determinism
    CrossClusterFederator fed(cfg);

    fed.registerCluster(makeEndpoint("c1", "http://c1:8080"));
    fed.registerCluster(makeEndpoint("c2", "http://c2:8080"));

    std::atomic<int> call_count{0};
    fed.setHttpPostForTesting(
        [&call_count](const std::string& url, const std::string&,
                       const std::string&, uint32_t,
                       std::string& resp) -> int {
            ++call_count;
            if (url.find("c1") != std::string::npos) {
                resp = R"([{"src":"c1","v":1}])";
            } else {
                resp = R"([{"src":"c2","v":2}])";
            }
            return 200;
        });

    auto result = fed.execute("FOR doc IN col RETURN doc");
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 2u);
    EXPECT_EQ(call_count.load(), 2);
}

TEST(CrossClusterFederatorTest, Execute_ParallelMerge) {
    CrossClusterFederator::Config cfg;
    cfg.enable_parallel_execution = true;
    CrossClusterFederator fed(cfg);

    for (int i = 1; i <= 4; ++i) {
        fed.registerCluster(
            makeEndpoint("c" + std::to_string(i),
                         "http://c" + std::to_string(i) + ":8080"));
    }

    fed.setHttpPostForTesting(
        [](const std::string&, const std::string&, const std::string&,
           uint32_t, std::string& resp) -> int {
            resp = R"([{"x":1}])";
            return 200;
        });

    auto result = fed.execute("FOR doc IN col RETURN doc");
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 4u); // 1 result per cluster
}

TEST(CrossClusterFederatorTest, Execute_FailedCluster_SkipEnabled) {
    CrossClusterFederator::Config cfg;
    cfg.skip_unreachable_clusters = true;
    cfg.enable_parallel_execution = false;
    CrossClusterFederator fed(cfg);

    fed.registerCluster(makeEndpoint("c1", "http://c1:8080"));
    fed.registerCluster(makeEndpoint("c2", "http://c2:8080"));

    fed.setHttpPostForTesting(
        [](const std::string& url, const std::string&, const std::string&,
           uint32_t, std::string& resp) -> int {
            if (url.find("c1") != std::string::npos) {
                return 0; // transport failure
            }
            resp = R"([{"ok":true}])";
            return 200;
        });

    auto result = fed.execute("FOR doc IN col RETURN doc");
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 1u); // only c2 succeeded
}

TEST(CrossClusterFederatorTest, Execute_AllClustersFail_SkipDisabled_Throws) {
    CrossClusterFederator::Config cfg;
    cfg.skip_unreachable_clusters = false;
    cfg.enable_parallel_execution = false;
    CrossClusterFederator fed(cfg);

    fed.registerCluster(makeEndpoint("c1"));
    fed.setHttpPostForTesting(makeFailMock());

    EXPECT_THROW(fed.execute("FOR doc IN col RETURN doc"),
                 std::runtime_error);
}

TEST(CrossClusterFederatorTest, Execute_Http4xx_CountsAsFailure) {
    CrossClusterFederator::Config cfg;
    cfg.skip_unreachable_clusters = true;
    cfg.enable_parallel_execution = false;
    CrossClusterFederator fed(cfg);

    fed.registerCluster(makeEndpoint("c1", "http://c1:8080"));
    fed.registerCluster(makeEndpoint("c2", "http://c2:8080"));

    fed.setHttpPostForTesting(
        [](const std::string& url, const std::string&, const std::string&,
           uint32_t, std::string& resp) -> int {
            if (url.find("c1") != std::string::npos) {
                resp = R"({"error":"not found"})";
                return 404; // HTTP error
            }
            resp = R"([{"ok":1}])";
            return 200;
        });

    auto result = fed.execute("FOR doc IN col RETURN doc");
    EXPECT_EQ(result.size(), 1u);
}

TEST(CrossClusterFederatorTest, Execute_MalformedJson_ClusterSkipped) {
    CrossClusterFederator::Config cfg;
    cfg.skip_unreachable_clusters = true;
    cfg.enable_parallel_execution = false;
    CrossClusterFederator fed(cfg);

    fed.registerCluster(makeEndpoint("c1", "http://c1:8080"));
    fed.registerCluster(makeEndpoint("c2", "http://c2:8080"));

    fed.setHttpPostForTesting(
        [](const std::string& url, const std::string&, const std::string&,
           uint32_t, std::string& resp) -> int {
            if (url.find("c1") != std::string::npos) {
                resp = "not valid json {{{{";
                return 200;
            }
            resp = R"([{"id":42}])";
            return 200;
        });

    auto result = fed.execute("FOR doc IN col RETURN doc");
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0]["id"], 42);
}

// ════════════════════════════════════════════════════════════════════════════
// Statistics tests
// ════════════════════════════════════════════════════════════════════════════

TEST(CrossClusterFederatorTest, Statistics_InitialState) {
    CrossClusterFederator fed;
    auto stats = fed.getStatistics();

    EXPECT_EQ(stats["total_queries"],           0);
    EXPECT_EQ(stats["successful_queries"],      0);
    EXPECT_EQ(stats["failed_cluster_requests"], 0);
    EXPECT_EQ(stats["registered_clusters"],     0);
}

TEST(CrossClusterFederatorTest, Statistics_AfterExecution) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1"));
    fed.setHttpPostForTesting(makeOkMock(R"([])"));

    fed.execute("FOR doc IN col RETURN doc");

    auto stats = fed.getStatistics();
    EXPECT_EQ(stats["total_queries"],      1);
    EXPECT_EQ(stats["successful_queries"], 1);
    EXPECT_EQ(stats["registered_clusters"], 1);
}

TEST(CrossClusterFederatorTest, Statistics_FailedClusters_Counted) {
    CrossClusterFederator::Config cfg;
    cfg.skip_unreachable_clusters = true;
    cfg.enable_parallel_execution = false;
    CrossClusterFederator fed(cfg);

    fed.registerCluster(makeEndpoint("c1", "http://c1:8080"));
    fed.registerCluster(makeEndpoint("c2", "http://c2:8080"));

    fed.setHttpPostForTesting(
        [](const std::string& url, const std::string&, const std::string&,
           uint32_t, std::string& resp) -> int {
            if (url.find("c1") != std::string::npos) return 0; // fail
            resp = R"([])";
            return 200;
        });

    fed.execute("FOR doc IN col RETURN doc");

    auto stats = fed.getStatistics();
    EXPECT_EQ(stats["failed_cluster_requests"], 1);
}

TEST(CrossClusterFederatorTest, Statistics_ContainsClusterList) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1", "http://c1:8080", 5000, 10.0));

    auto stats = fed.getStatistics();
    ASSERT_TRUE(stats["clusters"].is_array());
    ASSERT_EQ(stats["clusters"].size(), 1u);
    EXPECT_EQ(stats["clusters"][0]["cluster_id"],          "c1");
    EXPECT_EQ(stats["clusters"][0]["base_url"],            "http://c1:8080");
    EXPECT_EQ(stats["clusters"][0]["estimated_rows_hint"], 5000);
}

// ════════════════════════════════════════════════════════════════════════════
// Auth header tests
// ════════════════════════════════════════════════════════════════════════════

TEST(CrossClusterFederatorTest, AuthToken_SentInHeader) {
    CrossClusterFederator fed;
    ClusterEndpoint ep = makeEndpoint("c1", "http://c1:8080");
    ep.auth_token = "secret-token";
    fed.registerCluster(ep);

    std::string captured_auth;
    fed.setHttpPostForTesting(
        [&captured_auth](const std::string&, const std::string&,
                          const std::string& auth, uint32_t,
                          std::string& resp) -> int {
            captured_auth = auth;
            resp = R"([])";
            return 200;
        });

    fed.execute("FOR doc IN col RETURN doc");

    EXPECT_NE(captured_auth.find("secret-token"), std::string::npos);
    EXPECT_NE(captured_auth.find("Bearer"), std::string::npos);
}

TEST(CrossClusterFederatorTest, NoAuthToken_EmptyAuthHeader) {
    CrossClusterFederator fed;
    fed.registerCluster(makeEndpoint("c1")); // no auth token

    std::string captured_auth;
    fed.setHttpPostForTesting(
        [&captured_auth](const std::string&, const std::string&,
                          const std::string& auth, uint32_t,
                          std::string& resp) -> int {
            captured_auth = auth;
            resp = R"([])";
            return 200;
        });

    fed.execute("FOR doc IN col RETURN doc");
    EXPECT_TRUE(captured_auth.empty());
}
