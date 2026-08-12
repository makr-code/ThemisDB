/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_object_centric_tracer.cpp
 * Module:  tests/process/
 * Purpose: GTest unit tests OCT-01 .. OCT-10 for ObjectCentricTracer.
 */

#include <gtest/gtest.h>

#include "index/process_graph.h"
#include "process/object_centric_tracer.h"
#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json   = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class ObjectCentricTracerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_object_centric_tracer";
        fs::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 32;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 1;

        db_     = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        themis::registerProcessEdgeTypes();
        mgr_    = std::make_unique<themis::process::ProcessModelManager>(*db_);
        linker_ = std::make_unique<themis::process::ProcessLinker>(*db_);
        tracer_ = std::make_unique<themis::process::ObjectCentricTracer>(*linker_, *mgr_);
    }

    void TearDown() override {
        tracer_.reset();
        linker_.reset();
        mgr_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    /// Persist a model with given nodes/edges.
    void saveModel(const std::string& model_id,
                   const json& nodes, const json& edges) {
        themis::process::ProcessModelRecord rec;
        rec.id         = model_id;
        rec.name       = model_id;
        rec.normalized = {{"nodes", nodes}, {"edges", edges}};
        mgr_->save(rec);
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper>               db_;
    std::unique_ptr<themis::process::ProcessModelManager> mgr_;
    std::unique_ptr<themis::process::ProcessLinker>       linker_;
    std::unique_ptr<themis::process::ObjectCentricTracer> tracer_;
};

// ─────────────────────────────────────────────────────────────────────────────
// OCT-01: buildOcelLog() on empty attachments → valid OCEL JSON with empty events
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT01_EmptyAttachments) {
    const auto log = tracer_->buildOcelLog("inst_empty");

    ASSERT_TRUE(log.contains("ocel:global-log"));
    ASSERT_TRUE(log.contains("ocel:events"));
    ASSERT_TRUE(log.contains("ocel:objects"));
    EXPECT_TRUE(log["ocel:events"].empty());
    EXPECT_TRUE(log["ocel:objects"].empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-02: buildOcelLog() with 3 attachments → 3 events with correct fields
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT02_ThreeAttachments) {
    const std::string inst = "inst_three";
    for (int i = 1; i <= 3; ++i) {
        linker_->attachObject(inst,
                              "doc_" + std::to_string(i),
                              "documents",
                              themis::process::ProcessLinkType::HAS_DOCUMENT,
                              std::nullopt, {}, "user");
    }

    const auto log = tracer_->buildOcelLog(inst);
    ASSERT_EQ(log["ocel:events"].size(), 3u);

    for (const auto& ev : log["ocel:events"]) {
        EXPECT_TRUE(ev.contains("ocel:id"));
        EXPECT_TRUE(ev.contains("ocel:activity"));
        EXPECT_TRUE(ev.contains("ocel:timestamp"));
        EXPECT_TRUE(ev.contains("ocel:omap"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-03: OCEL JSON has required top-level keys
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT03_RequiredKeys) {
    const auto log = tracer_->buildOcelLog("inst_keys");
    EXPECT_TRUE(log.contains("ocel:global-log"));
    EXPECT_TRUE(log.contains("ocel:events"));
    EXPECT_TRUE(log.contains("ocel:objects"));
    EXPECT_TRUE(log["ocel:global-log"].contains("ocel:attribute-names"));
    EXPECT_TRUE(log["ocel:global-log"].contains("ocel:object-types"));
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-04: computeDfmg() on model with no nodes → empty arcs
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT04_EmptyModelEmptyArcs) {
    saveModel("m_empty_oct", json::array(), json::array());
    const auto dfmg = tracer_->computeDfmg("m_empty_oct", "documents");
    EXPECT_TRUE(dfmg["arcs"].empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-05: computeDfmg() on 3-node linear model → 2 arcs
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT05_ThreeNodeLinearModel) {
    json nodes = {
        {{"id","n1"},{"name","A"}},
        {{"id","n2"},{"name","B"}},
        {{"id","n3"},{"name","C"}}
    };
    // Edges labelled with object_type so computeDfmg picks them up
    json edges = {
        {{"from","n1"},{"to","n2"},{"label","documents"}},
        {{"from","n2"},{"to","n3"},{"label","documents"}}
    };
    saveModel("m_linear_oct", nodes, edges);

    const auto dfmg = tracer_->computeDfmg("m_linear_oct", "documents");
    EXPECT_EQ(dfmg["arcs"].size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-06: analyze() returns empty lists for a graph with no branching
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT06_NoBranchingNoDivergence) {
    json nodes = {
        {{"id","s"},{"name","Start"}},
        {{"id","e"},{"name","End"}}
    };
    json edges = {
        {{"from","s"},{"to","e"},{"label","doc"}}
    };
    saveModel("m_nobranch", nodes, edges);

    const auto res = tracer_->analyze("m_nobranch");
    EXPECT_TRUE(res.convergence_nodes.empty());
    EXPECT_TRUE(res.divergence_nodes.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-07: analyze() identifies convergence node (2 incoming paths join)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT07_ConvergenceNode) {
    // n1 → n3, n2 → n3: n3 has in-degree 2 (same label → convergence)
    json nodes = {
        {{"id","n1"},{"name","A"}},
        {{"id","n2"},{"name","B"}},
        {{"id","n3"},{"name","Join"}}
    };
    json edges = {
        {{"from","n1"},{"to","n3"},{"label","flow"}},
        {{"from","n2"},{"to","n3"},{"label","flow"}}
    };
    saveModel("m_conv", nodes, edges);

    const auto res = tracer_->analyze("m_conv");
    const bool has_n3 = std::find(res.convergence_nodes.begin(),
                                  res.convergence_nodes.end(), "n3")
                        != res.convergence_nodes.end();
    EXPECT_TRUE(has_n3) << "Expected n3 to be identified as a convergence node";
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-08: analyze() identifies divergence node (1 node splits to 2)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT08_DivergenceNode) {
    // n1 → n2, n1 → n3: n1 has out-degree 2 (same label → divergence)
    json nodes = {
        {{"id","n1"},{"name","Split"}},
        {{"id","n2"},{"name","A"}},
        {{"id","n3"},{"name","B"}}
    };
    json edges = {
        {{"from","n1"},{"to","n2"},{"label","flow"}},
        {{"from","n1"},{"to","n3"},{"label","flow"}}
    };
    saveModel("m_div", nodes, edges);

    const auto res = tracer_->analyze("m_div");
    const bool has_n1 = std::find(res.divergence_nodes.begin(),
                                  res.divergence_nodes.end(), "n1")
                        != res.divergence_nodes.end();
    EXPECT_TRUE(has_n1) << "Expected n1 to be identified as a divergence node";
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-09: OcelEvent fields are preserved in buildOcelLog output
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT09_EventFieldsPreserved) {
    const std::string inst = "inst_fields";
    linker_->attachObject(inst, "doc_99", "documents",
                          themis::process::ProcessLinkType::HAS_DOCUMENT,
                          std::nullopt, {}, "user");

    const auto log = tracer_->buildOcelLog(inst);
    ASSERT_EQ(log["ocel:events"].size(), 1u);

    const auto& ev = log["ocel:events"][0];
    // Activity should be the string form of HAS_DOCUMENT
    EXPECT_FALSE(ev["ocel:activity"].get<std::string>().empty());
    // Object map should reference "documents" collection
    EXPECT_TRUE(ev["ocel:omap"].contains("documents"));
}

// ─────────────────────────────────────────────────────────────────────────────
// OCT-10: computeDfmg() unique arcs only (no duplicates)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ObjectCentricTracerTest, OCT10_UniqueArcs) {
    // Two parallel edges from n1→n2 (same pair, same label)
    json nodes = {
        {{"id","n1"},{"name","A"}},
        {{"id","n2"},{"name","B"}}
    };
    json edges = {
        {{"from","n1"},{"to","n2"},{"label","docs"}},
        {{"from","n1"},{"to","n2"},{"label","docs"}}  // duplicate
    };
    saveModel("m_dupl", nodes, edges);

    const auto dfmg = tracer_->computeDfmg("m_dupl", "docs");

    // The frequency map aggregates; only one arc entry should exist for n1→n2
    int count = 0;
    for (const auto& arc : dfmg["arcs"]) {
        if (arc["from"] == "n1" && arc["to"] == "n2") ++count;
    }
    EXPECT_EQ(count, 1) << "Expected exactly one arc for n1→n2 (with frequency 2)";
    if (!dfmg["arcs"].empty()) {
        const auto& arc = dfmg["arcs"][0];
        if (arc.contains("frequency")) {
            EXPECT_EQ(arc["frequency"].get<int>(), 2);
        }
    }
}
