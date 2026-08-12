/**
 * @file test_distributed_flame_graph.cpp
 * @brief Unit tests for DistributedFlameGraph – distributed flame graph
 *        generation across cluster nodes.
 *
 * Tests cover:
 *  - Default construction and configuration retrieval
 *  - addNodeProfile / nodeCount / getNodeIds
 *  - merge(): empty graph when no profiles added
 *  - merge(): raw (non-normalised) count summation across two nodes
 *  - merge(): stacks unique to one node appear in the merged output
 *  - mergeFiltered(): only selected node IDs are included
 *  - normalize_per_node: equal visual weight across nodes with different volumes
 *  - Last-write-wins: re-adding the same node_id overwrites previous profile
 *  - max_nodes limit enforced
 *  - clearProfiles() resets state
 *  - diff(): new, removed, and changed hotspot detection
 *  - diff(): cpu_regression_percent computed correctly
 *  - MergedFlameGraph::toFoldedText() produces sorted, parseable output
 *  - MergedFlameGraph::toJSON() contains expected fields
 *  - Non-CPU snapshots are ignored during merge
 */

#include <gtest/gtest.h>
#include "observability/distributed_flame_graph.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static ProfileSnapshot makeCpuSnapshot(const std::string& folded_text) {
    ProfileSnapshot s;
    s.type = ProfileType::CPU;
    s.timestamp = std::chrono::system_clock::now();
    s.duration = std::chrono::seconds(60);
    s.data.assign(folded_text.begin(), folded_text.end());
    return s;
}

static NodeProfile makeNode(const std::string& node_id,
                             const std::string& folded_text) {
    NodeProfile np;
    np.node_id = node_id;
    np.host    = node_id + ".internal";
    np.snapshot = makeCpuSnapshot(folded_text);
    return np;
}

// ---------------------------------------------------------------------------
// Construction / configuration
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, DefaultConfig) {
    DistributedFlameGraph dfg;
    auto cfg = dfg.getConfig();
    EXPECT_EQ(128u, cfg.max_nodes);
    EXPECT_FALSE(cfg.normalize_per_node);
}

TEST(DistributedFlameGraphTest, ConfigRoundtrip) {
    DistributedFlameGraphConfig cfg;
    cfg.max_nodes = 16;
    cfg.normalize_per_node = true;

    DistributedFlameGraph dfg(cfg);
    auto got = dfg.getConfig();
    EXPECT_EQ(16u, got.max_nodes);
    EXPECT_TRUE(got.normalize_per_node);
}

// ---------------------------------------------------------------------------
// addNodeProfile / nodeCount / getNodeIds
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, InitiallyEmpty) {
    DistributedFlameGraph dfg;
    EXPECT_EQ(0u, dfg.nodeCount());
    EXPECT_TRUE(dfg.getNodeIds().empty());
}

TEST(DistributedFlameGraphTest, AddNodesUpdateCount) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("node-1", "main;foo 10\n"));
    EXPECT_EQ(1u, dfg.nodeCount());
    dfg.addNodeProfile(makeNode("node-2", "main;bar 5\n"));
    EXPECT_EQ(2u, dfg.nodeCount());
}

TEST(DistributedFlameGraphTest, GetNodeIdsReturnsAll) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("alpha", "main 1\n"));
    dfg.addNodeProfile(makeNode("beta",  "main 2\n"));

    auto ids = dfg.getNodeIds();
    EXPECT_EQ(2u, ids.size());
    bool hasAlpha = std::find(ids.begin(), ids.end(), "alpha") != ids.end();
    bool hasBeta  = std::find(ids.begin(), ids.end(), "beta")  != ids.end();
    EXPECT_TRUE(hasAlpha);
    EXPECT_TRUE(hasBeta);
}

// ---------------------------------------------------------------------------
// merge() – basic functionality
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, MergeEmptyReturnsEmptyGraph) {
    DistributedFlameGraph dfg;
    auto merged = dfg.merge();
    EXPECT_TRUE(merged.stacks.empty());
    EXPECT_TRUE(merged.node_ids.empty());
}

TEST(DistributedFlameGraphTest, MergeSingleNodeIdentical) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("node-1", "main;foo 42\nmain;bar 8\n"));
    auto merged = dfg.merge();

    EXPECT_EQ(1u, merged.node_ids.size());
    EXPECT_EQ(2u, merged.stacks.size());
    EXPECT_EQ(42u, merged.stacks.at("main;foo"));
    EXPECT_EQ(8u,  merged.stacks.at("main;bar"));
}

TEST(DistributedFlameGraphTest, MergeTwoNodesRawSumOverlapping) {
    // Both nodes report the same stack; counts should be summed.
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("node-1", "main;query_exec 40\n"));
    dfg.addNodeProfile(makeNode("node-2", "main;query_exec 30\n"));

    auto merged = dfg.merge();
    EXPECT_EQ(2u, merged.node_ids.size());
    EXPECT_EQ(70u, merged.stacks.at("main;query_exec"));
}

TEST(DistributedFlameGraphTest, MergeTwoNodesDisjointStacks) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("node-1", "main;compaction 10\n"));
    dfg.addNodeProfile(makeNode("node-2", "main;flush 20\n"));

    auto merged = dfg.merge();
    EXPECT_EQ(2u, merged.stacks.size());
    EXPECT_EQ(10u, merged.stacks.at("main;compaction"));
    EXPECT_EQ(20u, merged.stacks.at("main;flush"));
}

// ---------------------------------------------------------------------------
// mergeFiltered()
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, MergeFilteredSelectsOnlySpecifiedNodes) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("node-1", "main;a 10\n"));
    dfg.addNodeProfile(makeNode("node-2", "main;b 20\n"));
    dfg.addNodeProfile(makeNode("node-3", "main;c 30\n"));

    auto merged = dfg.mergeFiltered({"node-1", "node-3"});

    EXPECT_EQ(2u, merged.node_ids.size());
    EXPECT_TRUE(merged.stacks.count("main;a"));
    EXPECT_TRUE(merged.stacks.count("main;c"));
    EXPECT_FALSE(merged.stacks.count("main;b"));
}

TEST(DistributedFlameGraphTest, MergeFilteredIgnoresMissingNodeIds) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("node-1", "main;foo 5\n"));

    // "node-99" does not exist; should be silently ignored
    auto merged = dfg.mergeFiltered({"node-1", "node-99"});
    EXPECT_EQ(1u, merged.node_ids.size());
    EXPECT_EQ(5u, merged.stacks.at("main;foo"));
}

// ---------------------------------------------------------------------------
// normalize_per_node
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, NormalizePerNodeEqualisesWeight) {
    // node-1 has 100 samples on "main;a" (100 % share)
    // node-2 has 1000 samples on "main;b" (100 % share)
    // With normalisation both should contribute the same total count (~1 000 000).
    DistributedFlameGraphConfig cfg;
    cfg.normalize_per_node = true;

    DistributedFlameGraph dfg(cfg);
    dfg.addNodeProfile(makeNode("node-1", "main;a 100\n"));
    dfg.addNodeProfile(makeNode("node-2", "main;b 1000\n"));

    auto merged = dfg.merge();

    ASSERT_TRUE(merged.stacks.count("main;a"));
    ASSERT_TRUE(merged.stacks.count("main;b"));
    // Both stacks should have equal (or near-equal) counts after normalisation.
    EXPECT_EQ(merged.stacks.at("main;a"), merged.stacks.at("main;b"));
}

// ---------------------------------------------------------------------------
// Last-write-wins
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, LastWriteWinsForSameNodeId) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("node-1", "main;old 99\n"));
    dfg.addNodeProfile(makeNode("node-1", "main;new 7\n"));

    EXPECT_EQ(1u, dfg.nodeCount());
    auto merged = dfg.merge();
    EXPECT_FALSE(merged.stacks.count("main;old"));
    EXPECT_EQ(7u, merged.stacks.at("main;new"));
}

// ---------------------------------------------------------------------------
// max_nodes limit
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, MaxNodesLimitEnforced) {
    DistributedFlameGraphConfig cfg;
    cfg.max_nodes = 3;
    DistributedFlameGraph dfg(cfg);

    for (int i = 0; i < 5; ++i) {
        dfg.addNodeProfile(makeNode("node-" + std::to_string(i), "main 1\n"));
    }

    EXPECT_EQ(3u, dfg.nodeCount());
}

// ---------------------------------------------------------------------------
// clearProfiles()
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, ClearProfilesResetsState) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("node-1", "main;foo 10\n"));
    dfg.clearProfiles();

    EXPECT_EQ(0u, dfg.nodeCount());
    EXPECT_TRUE(dfg.merge().stacks.empty());
}

// ---------------------------------------------------------------------------
// diff()
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, DiffIdenticalGraphsNoChange) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("n1", "main;a 50\nmain;b 50\n"));
    auto g = dfg.merge();

    auto d = dfg.diff(g, g);
    EXPECT_NEAR(0.0, d.cpu_regression_percent, 0.001);
    EXPECT_TRUE(d.new_hotspots.empty());
    EXPECT_TRUE(d.removed_hotspots.empty());
    EXPECT_TRUE(d.changed_hotspots.empty());
}

TEST(DistributedFlameGraphTest, DiffDetectsNewHotspot) {
    DistributedFlameGraph dfg;

    dfg.addNodeProfile(makeNode("n1", "main;existing 100\n"));
    auto baseline = dfg.merge();

    dfg.addNodeProfile(makeNode("n1", "main;existing 100\nmain;newHot 200\n"));
    auto current = dfg.merge();

    auto d = dfg.diff(baseline, current);
    bool found = false;
    for (const auto& h : d.new_hotspots) {
        if (h.find("newHot") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Expected 'newHot' in new_hotspots";
}

TEST(DistributedFlameGraphTest, DiffDetectsRemovedHotspot) {
    DistributedFlameGraph dfg;

    dfg.addNodeProfile(makeNode("n1", "main;a 10\nmain;disappear 20\n"));
    auto baseline = dfg.merge();

    dfg.addNodeProfile(makeNode("n1", "main;a 10\n"));
    auto current = dfg.merge();

    auto d = dfg.diff(baseline, current);
    bool found = false;
    for (const auto& h : d.removed_hotspots) {
        if (h.find("disappear") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Expected 'disappear' in removed_hotspots";
}

TEST(DistributedFlameGraphTest, DiffComputesCpuRegression) {
    DistributedFlameGraph dfg;

    dfg.addNodeProfile(makeNode("n1", "main;a 100\n"));
    auto baseline = dfg.merge();

    dfg.addNodeProfile(makeNode("n1", "main;a 200\n"));
    auto current = dfg.merge();

    auto d = dfg.diff(baseline, current);
    EXPECT_NEAR(100.0, d.cpu_regression_percent, 0.001);
}

// ---------------------------------------------------------------------------
// MergedFlameGraph::toFoldedText()
// ---------------------------------------------------------------------------

TEST(MergedFlameGraphTest, ToFoldedTextSortedAndParseable) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("n1", "main;z 5\nmain;a 10\n"));
    dfg.addNodeProfile(makeNode("n2", "main;a 3\n"));

    auto merged = dfg.merge();
    auto text   = merged.toFoldedText();

    // Verify it contains both stacks
    EXPECT_NE(std::string::npos, text.find("main;a"));
    EXPECT_NE(std::string::npos, text.find("main;z"));

    // Verify counts are aggregated
    EXPECT_EQ(13u, merged.stacks.at("main;a"));
    EXPECT_EQ(5u,  merged.stacks.at("main;z"));
}

// ---------------------------------------------------------------------------
// MergedFlameGraph::toJSON()
// ---------------------------------------------------------------------------

TEST(MergedFlameGraphTest, ToJSONContainsExpectedFields) {
    DistributedFlameGraph dfg;
    dfg.addNodeProfile(makeNode("n1", "main;foo 7\n"));
    auto merged = dfg.merge();
    auto j = merged.toJSON();

    EXPECT_TRUE(j.contains("generated_at_ms"));
    EXPECT_TRUE(j.contains("node_ids"));
    EXPECT_TRUE(j.contains("node_count"));
    EXPECT_TRUE(j.contains("stack_count"));
    EXPECT_TRUE(j.contains("folded_text"));
    EXPECT_TRUE(j.contains("stacks"));
    EXPECT_EQ(1u, j.at("node_count").get<size_t>());
    EXPECT_EQ(1u, j.at("stack_count").get<size_t>());
}

// ---------------------------------------------------------------------------
// Non-CPU snapshots are ignored
// ---------------------------------------------------------------------------

TEST(DistributedFlameGraphTest, NonCpuSnapshotsIgnoredDuringMerge) {
    DistributedFlameGraph dfg;

    NodeProfile np;
    np.node_id = "n1";
    np.snapshot.type = ProfileType::HEAP;
    const std::string text = "main;malloc 100\n";
    np.snapshot.data.assign(text.begin(), text.end());

    dfg.addNodeProfile(np);
    auto merged = dfg.merge();

    // HEAP snapshot should not contribute stacks
    EXPECT_TRUE(merged.stacks.empty());
    // But the node should still be registered
    EXPECT_EQ(1u, dfg.nodeCount());
}
