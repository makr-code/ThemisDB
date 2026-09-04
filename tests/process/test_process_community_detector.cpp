/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_process_community_detector.cpp
 * Module:  tests/process/
 * Purpose: GTest unit tests LCD-01 .. LCD-10 for ProcessCommunityDetector.
 */

#include <gtest/gtest.h>

#include "index/process_graph.h"
#include "process/process_community_detector.h"
#include "process/process_model_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json   = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class ProcessCommunityDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_community_detector";
        fs::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 32;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 1;

        db_       = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        themis::registerProcessEdgeTypes();
        mgr_      = std::make_unique<themis::process::ProcessModelManager>(*db_);
        detector_ = std::make_unique<themis::process::ProcessCommunityDetector>(*db_);
    }

    void TearDown() override {
        detector_.reset();
        mgr_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    /// Persist a model with given nodes/edges in normalized form.
    void saveModel(const std::string& model_id,
                   const json& nodes, const json& edges) {
        themis::process::ProcessModelRecord rec;
        rec.id          = model_id;
        rec.name        = model_id;
        rec.normalized  = {{"nodes", nodes}, {"edges", edges}};
        mgr_->save(rec);
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper>               db_;
    std::unique_ptr<themis::process::ProcessModelManager> mgr_;
    std::unique_ptr<themis::process::ProcessCommunityDetector> detector_;
};

// ─────────────────────────────────────────────────────────────────────────────
// LCD-01: Empty graph → 0 communities
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD01_EmptyGraph) {
    saveModel("m_empty", json::array(), json::array());
    auto comms = detector_->detect("m_empty");
    EXPECT_TRUE(comms.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-02: Single node → 1 community
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD02_SingleNode) {
    saveModel("m_single",
              json::array({json{{"id", "n1"}, {"name", "Start"}}}),
              json::array());
    auto comms = detector_->detect("m_single");
    ASSERT_EQ(comms.size(), 1u);
    EXPECT_EQ(comms[0].node_ids.size(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-03: Two disconnected nodes → 2 communities
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD03_TwoDisconnectedNodes) {
    saveModel("m_disconnected",
              json::array({json{{"id","n1"},{"name","A"}},
                           json{{"id","n2"},{"name","B"}}}),
              json::array());
    auto comms = detector_->detect("m_disconnected");
    EXPECT_EQ(comms.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-04: Linear chain of 5 nodes → 1-2 communities
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD04_LinearChain) {
    json nodes = json::array();
    json edges = json::array();
    for (int i = 1; i <= 5; ++i) {
        nodes.push_back({{"id", "n" + std::to_string(i)},
                         {"name", "Node" + std::to_string(i)}});
    }
    for (int i = 1; i < 5; ++i) {
        edges.push_back({{"from", "n" + std::to_string(i)},
                         {"to",   "n" + std::to_string(i + 1)}});
    }
    saveModel("m_chain", nodes, edges);
    auto comms = detector_->detect("m_chain");
    EXPECT_GE(comms.size(), 1u);
    EXPECT_LE(comms.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-05: Two cliques connected by single edge → 2 communities
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD05_TwoCliques) {
    // Clique A: a1-a2-a3 (all pairs connected)
    // Clique B: b1-b2-b3 (all pairs connected)
    // Bridge: a3 → b1
    json nodes = {
        {{"id","a1"},{"name","A1"}}, {{"id","a2"},{"name","A2"}},
        {{"id","a3"},{"name","A3"}}, {{"id","b1"},{"name","B1"}},
        {{"id","b2"},{"name","B2"}}, {{"id","b3"},{"name","B3"}}
    };
    json edges = {
        {{"from","a1"},{"to","a2"}}, {{"from","a2"},{"to","a3"}},
        {{"from","a1"},{"to","a3"}}, {{"from","b1"},{"to","b2"}},
        {{"from","b2"},{"to","b3"}}, {{"from","b1"},{"to","b3"}},
        {{"from","a3"},{"to","b1"}}  // bridge
    };
    saveModel("m_cliques", nodes, edges);
    auto comms = detector_->detect("m_cliques");
    // The two dense cliques should form distinct communities
    EXPECT_GE(comms.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-06: Resolution > 1.0 produces ≥ communities as resolution == 1.0
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD06_HigherResolutionMoreCommunities) {
    // 6-node two-clique graph from LCD-05
    json nodes = {
        {{"id","a1"},{"name","A1"}}, {{"id","a2"},{"name","A2"}},
        {{"id","a3"},{"name","A3"}}, {{"id","b1"},{"name","B1"}},
        {{"id","b2"},{"name","B2"}}, {{"id","b3"},{"name","B3"}}
    };
    json edges = {
        {{"from","a1"},{"to","a2"}}, {{"from","a2"},{"to","a3"}},
        {{"from","a1"},{"to","a3"}}, {{"from","b1"},{"to","b2"}},
        {{"from","b2"},{"to","b3"}}, {{"from","b1"},{"to","b3"}},
        {{"from","a3"},{"to","b1"}}
    };
    saveModel("m_res", nodes, edges);

    const auto comms_1   = detector_->detect("m_res", 1.0f);
    const auto comms_high = detector_->detect("m_res", 2.0f);
    // Higher resolution should produce at least as many communities
    EXPECT_GE(comms_high.size(), comms_1.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-07: persistCommunities + loadCommunities roundtrip
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD07_PersistAndLoad) {
    saveModel("m_rtrip",
              json::array({json{{"id","n1"},{"name","Alpha"}},
                           json{{"id","n2"},{"name","Beta"}}}),
              json::array());

    auto comms = detector_->detect("m_rtrip");
    ASSERT_FALSE(comms.empty());

    ASSERT_TRUE(detector_->persistCommunities("m_rtrip", comms));

    const auto loaded = detector_->loadCommunities("m_rtrip");
    EXPECT_EQ(loaded.size(), comms.size());
    for (const auto& lc : loaded) {
        EXPECT_FALSE(lc.community_id.empty());
        EXPECT_FALSE(lc.node_ids.empty());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-08: Community IDs are unique
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD08_CommunityIdsUnique) {
    json nodes = json::array();
    json edges = json::array();
    for (int i = 1; i <= 6; ++i) {
        nodes.push_back({{"id","n"+std::to_string(i)},{"name","N"+std::to_string(i)}});
    }
    saveModel("m_unique", nodes, edges);

    const auto comms = detector_->detect("m_unique");
    std::vector<std::string> ids;
    ids.reserve(comms.size());
    for (const auto& c : comms) {
      ids.push_back(c.community_id);
    }
    const auto unique_ids = std::set<std::string>(ids.begin(), ids.end());
    EXPECT_EQ(ids.size(), unique_ids.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-09: generateReport returns non-empty string
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD09_GenerateReportNonEmpty) {
    themis::process::ProcessCommunity c;
    c.community_id = "community_0";
    c.node_ids     = {"n1", "n2", "n3"};

    const std::string report = detector_->generateReport(c, "model_x", "", "de");
    EXPECT_FALSE(report.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-10: detect() on a 20-node graph completes in < 500 ms
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD10_PerformanceTwentyNodes) {
    json nodes = json::array();
    json edges = json::array();
    for (int i = 0; i < 20; ++i) {
        nodes.push_back({{"id","n"+std::to_string(i)},{"name","N"+std::to_string(i)}});
    }
    for (int i = 0; i < 19; ++i) {
        edges.push_back({{"from","n"+std::to_string(i)},
                         {"to","n"+std::to_string(i+1)}});
    }
    saveModel("m_perf20", nodes, edges);

    const auto t0 = std::chrono::steady_clock::now();
    const auto comms = detector_->detect("m_perf20");
    const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0).count();

    EXPECT_LT(dt, 500) << "detect() took " << dt << " ms for 20-node graph";
    EXPECT_FALSE(comms.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-11: generateReport includes node count and modularity score (stub #238)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD11_GenerateReport_IncludesModularityAndCount) {
    themis::process::ProcessCommunity c;
    c.community_id     = "community_0";
    c.node_ids         = {"n1", "n2", "n3", "n4", "n5"};
    c.label            = "n1; n2; n3";
    c.modularity_score = 0.1234f;

    const std::string report_en = detector_->generateReport(c, "m", "", "en");
    // Must contain node count
    EXPECT_NE(report_en.find("5"), std::string::npos)
        << "Report must mention the node count (5). Report: " << report_en;
    // Must contain modularity
    EXPECT_NE(report_en.find("modularity"), std::string::npos)
        << "English report must contain 'modularity'. Report: " << report_en;
    // Must contain the community_id
    EXPECT_NE(report_en.find("community_0"), std::string::npos);

    // German variant
    const std::string report_de = detector_->generateReport(c, "m", "", "de");
    EXPECT_NE(report_de.find("Modularität"), std::string::npos)
        << "German report must contain 'Modularität'. Report: " << report_de;
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD-12: generateReport lists all nodes (up to 10) with ellipsis beyond that
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessCommunityDetectorTest, LCD12_GenerateReport_EllipsisForLargeCommunit) {
    themis::process::ProcessCommunity c;
    c.community_id = "community_1";
    for (int i = 0; i < 15; ++i) {
        c.node_ids.push_back("node_" + std::to_string(i));
    }
    c.modularity_score = 0.05f;

    const std::string report = detector_->generateReport(c, "m", "", "en");
    // Ellipsis must appear because >10 nodes
    EXPECT_NE(report.find("more"), std::string::npos)
        << "Report should contain ellipsis for communities with >10 nodes. Report: " << report;
    // First 10 nodes should be listed
    EXPECT_NE(report.find("node_0"), std::string::npos);
    EXPECT_NE(report.find("node_9"), std::string::npos);
}
