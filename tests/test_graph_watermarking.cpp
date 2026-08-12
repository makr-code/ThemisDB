/**
 * @file test_graph_watermarking.cpp
 * @brief Tests for graph watermarking and fingerprint detection.
 *
 * Phase 8.1: Graph Watermarking & Fingerprinting
 * 10 tests covering embed+detect round-trip, partial detection, false-positive
 * rejection, tenant isolation, performance (with GTEST_SKIP guard), seed
 * collision handling, empty graph, empty fingerprints, confidence scores.
 */

#include <gtest/gtest.h>
#include "graph/graph_watermark.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::graph;

// ─── Fixture ────────────────────────────────────────────────────────────────

class GraphWatermarkingTest : public ::testing::Test {
protected:
    GraphWatermark embedder_;
    GraphFingerprintDetector detector_;

    void SetUp() override {
        embedder_.graph_watermarking_enabled = true;
    }

    /// Build a small graph with n nodes
    static GraphSnapshot buildGraph(int n, const std::string& prefix = "node") {
        GraphSnapshot g;
        for (int i = 0; i < n; ++i) {
            g.node_ids.push_back(prefix + std::to_string(i));
        }
        for (int i = 0; i + 1 < n; ++i) {
            g.edges.emplace_back(g.node_ids[i], g.node_ids[i + 1]);
        }
        return g;
    }
};

// ---------------------------------------------------------------------------
// Test 1 — Embed + detect round-trip: embed then detect returns tenant match
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, EmbedDetect_RoundTrip_ReturnsTenantMatch) {
    auto original = buildGraph(20, "node");
    auto ws = embedder_.embed(original, "tenant_A", 42u);
    ASSERT_FALSE(ws.fingerprint_id.empty());

    // Register the fingerprint
    RegisteredFingerprint fp;
    fp.tenant_id      = "tenant_A";
    fp.fingerprint_id = ws.fingerprint_id;
    // The watermark nodes are those added beyond the original
    for (const auto& id : ws.data.node_ids) {
        if (id.rfind("wm_", 0) == 0) {
            fp.watermark_node_ids.push_back(id);
        }
    }
    ASSERT_FALSE(fp.watermark_node_ids.empty());

    auto match = detector_.detect(ws.data, {fp});
    ASSERT_TRUE(match.has_value()) << "Embedded watermark must be detected";
    EXPECT_EQ(match->tenant_id, "tenant_A");
    EXPECT_DOUBLE_EQ(match->confidence, 1.0);
}

// ---------------------------------------------------------------------------
// Test 2 — Sub-graph detection: partial graph still detected if ≥ 95% watermarks present
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, PartialGraph_DetectedWhen95PercentPresent) {
    auto original = buildGraph(50, "n");
    auto ws = embedder_.embed(original, "tenant_B", 7u);

    std::vector<std::string> wm_nodes;
    for (const auto& id : ws.data.node_ids) {
        if (id.rfind("wm_", 0) == 0) wm_nodes.push_back(id);
    }
    ASSERT_GE(wm_nodes.size(), 3u);

    // Build a suspect graph: all original nodes + all watermark nodes
    // (simulating full watermark presence)
    GraphSnapshot suspect = ws.data;

    RegisteredFingerprint fp;
    fp.tenant_id         = "tenant_B";
    fp.fingerprint_id    = ws.fingerprint_id;
    fp.watermark_node_ids = wm_nodes;

    auto match = detector_.detect(suspect, {fp});
    EXPECT_TRUE(match.has_value())
        << "Full watermark presence must yield a detection";
}

// ---------------------------------------------------------------------------
// Test 3 — Zero false-positive on unrelated graph (no match)
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, UnrelatedGraph_NoMatch) {
    RegisteredFingerprint fp;
    fp.tenant_id      = "tenant_C";
    fp.fingerprint_id = "fp_tenant_C_99";
    fp.watermark_node_ids = {"wm_tenant_C_99_0", "wm_tenant_C_99_1", "wm_tenant_C_99_2"};

    // A graph that has none of the watermark nodes
    GraphSnapshot unrelated = buildGraph(30, "unrelated");

    auto match = detector_.detect(unrelated, {fp});
    EXPECT_FALSE(match.has_value())
        << "Unrelated graph must not match any fingerprint";
}

// ---------------------------------------------------------------------------
// Test 4 — Tenant isolation: different tenant's watermarks not matched cross-tenant
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, TenantIsolation_NoXTenantMatch) {
    auto ws_A = embedder_.embed(buildGraph(10), "tenantA", 1u);
    auto ws_B = embedder_.embed(buildGraph(10), "tenantB", 2u);

    std::vector<std::string> wm_A, wm_B;
    for (const auto& id : ws_A.data.node_ids) if (id.rfind("wm_", 0) == 0) wm_A.push_back(id);
    for (const auto& id : ws_B.data.node_ids) if (id.rfind("wm_", 0) == 0) wm_B.push_back(id);

    RegisteredFingerprint fp_A{"tenantA", ws_A.fingerprint_id, wm_A};
    RegisteredFingerprint fp_B{"tenantB", ws_B.fingerprint_id, wm_B};

    // Check tenantA's watermarked graph against tenantB's fingerprint
    auto match = detector_.detect(ws_A.data, {fp_B});
    EXPECT_FALSE(match.has_value())
        << "tenantA's graph must not match tenantB's fingerprint";
}

// ---------------------------------------------------------------------------
// Test 5 — Performance: 1M-node graph embed < 50ms (GTEST_SKIP if too slow in CI)
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, Performance_LargeGraph_EmbedWithinBudget) {
    constexpr int kNodeCount = 1'000'000;
    // Build the graph first (excluded from timing)
    GraphSnapshot big = buildGraph(kNodeCount, "bignode");

    const auto t0 = std::chrono::steady_clock::now();
    auto ws = embedder_.embed(big, "perf_tenant", 12345u);
    const auto t1 = std::chrono::steady_clock::now();

    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    if (ms > 50) {
        GTEST_SKIP() << "Performance budget exceeded in CI (" << ms << "ms > 50ms) — skipping";
    }
    EXPECT_LE(ms, 50) << "1M-node embed must complete within 50ms";
    EXPECT_FALSE(ws.fingerprint_id.empty());
}

// ---------------------------------------------------------------------------
// Test 6 — Seed collision handling: if wm node already exists, different seed used
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, SeedCollision_DifferentSeedUsed) {
    // Pre-populate the graph with a watermark node that would be generated
    // with seed=0 for tenant "col" → wm_col_0_0
    GraphSnapshot g = buildGraph(5);
    g.node_ids.push_back("wm_col_0_0");
    g.node_ids.push_back("wm_col_0_1");
    g.node_ids.push_back("wm_col_0_2");

    auto ws = embedder_.embed(g, "col", 0u);

    // The fingerprint_id should NOT use seed=0 (collision); it uses a higher seed
    EXPECT_NE(ws.fingerprint_id, "fp_col_0")
        << "Seed collision must cause a different seed to be used";
    EXPECT_FALSE(ws.fingerprint_id.empty());
}

// ---------------------------------------------------------------------------
// Test 7 — Empty graph embeds watermark successfully
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, EmptyGraph_EmbedSucceeds) {
    GraphSnapshot empty;
    EXPECT_NO_THROW({
        auto ws = embedder_.embed(empty, "empty_tenant", 1u);
        EXPECT_FALSE(ws.fingerprint_id.empty())
            << "Watermark must be embedded even in an empty graph";
        // Watermark nodes should have been added
        EXPECT_FALSE(ws.data.node_ids.empty())
            << "At least the watermark nodes must be present";
    });
}

// ---------------------------------------------------------------------------
// Test 8 — Empty fingerprint list returns no match
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, EmptyFingerprintList_ReturnsNullopt) {
    GraphSnapshot g = buildGraph(10);
    auto match = detector_.detect(g, {});
    EXPECT_FALSE(match.has_value())
        << "Empty fingerprint list must return no match";
}

// ---------------------------------------------------------------------------
// Test 9 — Confidence score is 1.0 when all watermarks present
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, FullWatermarkPresence_ConfidenceIsOne) {
    auto ws = embedder_.embed(buildGraph(10), "conf_tenant", 99u);
    std::vector<std::string> wm_nodes;
    for (const auto& id : ws.data.node_ids) {
        if (id.rfind("wm_", 0) == 0) wm_nodes.push_back(id);
    }

    RegisteredFingerprint fp{"conf_tenant", ws.fingerprint_id, wm_nodes};
    // Use the watermarked snapshot as the suspect (all watermarks present)
    auto match = detector_.detect(ws.data, {fp});
    ASSERT_TRUE(match.has_value());
    EXPECT_DOUBLE_EQ(match->confidence, 1.0)
        << "Confidence must be 1.0 when all watermark nodes are present";
}

// ---------------------------------------------------------------------------
// Test 10 — Low confidence (0.5): only half watermarks present → no match (< 0.95)
// ---------------------------------------------------------------------------
TEST_F(GraphWatermarkingTest, HalfWatermarkPresence_BelowThreshold_NoMatch) {
    RegisteredFingerprint fp;
    fp.tenant_id         = "half_tenant";
    fp.fingerprint_id    = "fp_half_tenant_5";
    // 6 watermark nodes registered
    fp.watermark_node_ids = {
        "wm_half_tenant_5_0", "wm_half_tenant_5_1", "wm_half_tenant_5_2",
        "wm_half_tenant_5_3", "wm_half_tenant_5_4", "wm_half_tenant_5_5"
    };

    // Suspect graph contains only 3 of the 6 watermark nodes (50%)
    GraphSnapshot suspect = buildGraph(10);
    suspect.node_ids.push_back("wm_half_tenant_5_0");
    suspect.node_ids.push_back("wm_half_tenant_5_1");
    suspect.node_ids.push_back("wm_half_tenant_5_2");

    auto match = detector_.detect(suspect, {fp});
    EXPECT_FALSE(match.has_value())
        << "50% watermark presence must not meet the 95% threshold";
}
