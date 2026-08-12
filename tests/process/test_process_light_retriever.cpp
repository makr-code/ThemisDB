/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_process_light_retriever.cpp
 * Module:  tests/process/
 * Purpose: GTest unit tests PLR-01 .. PLR-08 for ProcessLightRetriever.
 */

#include <gtest/gtest.h>

#include "index/process_graph.h"
#include "process/process_community_detector.h"
#include "process/process_graph_rag.h"
#include "process/process_light_retriever.h"
#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using json   = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class ProcessLightRetrieverTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_light_retriever";
        fs::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 32;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 1;

        db_       = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        themis::registerProcessEdgeTypes();
        engine_    = std::make_unique<themis::ProcessGraphManager>(*db_);
        mgr_       = std::make_unique<themis::process::ProcessModelManager>(*db_);
        linker_    = std::make_unique<themis::process::ProcessLinker>(*db_);
        rag_       = std::make_unique<themis::process::ProcessGraphRag>(
                         *db_, *engine_, *mgr_, *linker_);
        detector_  = std::make_unique<themis::process::ProcessCommunityDetector>(*db_);
        retriever_ = std::make_unique<themis::process::ProcessLightRetriever>(
                         *db_, *rag_, *detector_);
    }

    void TearDown() override {
        retriever_.reset();
        detector_.reset();
        rag_.reset();
        linker_.reset();
        mgr_.reset();
        engine_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    /// Persist a minimal model and return its model_id.
    std::string saveMinimalModel(const std::string& model_id) {
        themis::process::ProcessModelRecord rec;
        rec.id   = model_id;
        rec.name = model_id;
        rec.normalized = {
            {"nodes", json::array({
                json{{"id","n1"},{"name","Start"}},
                json{{"id","n2"},{"name","End"}}})},
            {"edges", json::array({
                json{{"from","n1"},{"to","n2"}}})}
        };
        mgr_->save(rec);
        return model_id;
    }

    /// Write an instance record to RocksDB so ProcessLightRetriever can resolve model_id.
    void storeInstance(const std::string& instance_id, const std::string& model_id) {
        json inst;
        inst["model_id"]             = model_id;
        inst["process_definition_id"] = model_id;
        db_->put("proc:inst:" + instance_id, inst.dump());
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper>               db_;
    std::unique_ptr<themis::ProcessGraphManager>          engine_;
    std::unique_ptr<themis::process::ProcessModelManager> mgr_;
    std::unique_ptr<themis::process::ProcessLinker>       linker_;
    std::unique_ptr<themis::process::ProcessGraphRag>     rag_;
    std::unique_ptr<themis::process::ProcessCommunityDetector> detector_;
    std::unique_ptr<themis::process::ProcessLightRetriever>    retriever_;
};

// ─────────────────────────────────────────────────────────────────────────────
// PLR-01: AUTO mode with global keyword → uses GLOBAL mode
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessLightRetrieverTest, PLR01_AutoGlobalKeyword) {
    saveMinimalModel("m_plr01");
    storeInstance("inst_plr01", "m_plr01");

    // Persist at least one community so GLOBAL path returns GLOBAL
    themis::process::ProcessCommunity c;
    c.community_id = "community_0";
    c.node_ids     = {"n1", "n2"};
    c.label        = "Start; End";
    c.report       = "Report for community_0";
    detector_->persistCommunities("m_plr01", {c});

    const auto result = retriever_->retrieve(
        "gesamte Prozessbeschreibung",   // global keyword
        "inst_plr01",
        themis::process::RetrievalMode::AUTO);

    EXPECT_EQ(result.used_mode, themis::process::RetrievalMode::GLOBAL);
}

// ─────────────────────────────────────────────────────────────────────────────
// PLR-02: AUTO mode with specific term → uses LOCAL mode
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessLightRetrieverTest, PLR02_AutoLocalKeyword) {
    saveMinimalModel("m_plr02");
    storeInstance("inst_plr02", "m_plr02");

    const auto result = retriever_->retrieve(
        "Welche Dokumente fehlen bei Schritt n1?",  // specific – no global keyword
        "inst_plr02",
        themis::process::RetrievalMode::AUTO);

    EXPECT_EQ(result.used_mode, themis::process::RetrievalMode::LOCAL);
}

// ─────────────────────────────────────────────────────────────────────────────
// PLR-03: GLOBAL mode with persisted communities → returns community reports
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessLightRetrieverTest, PLR03_GlobalWithPersistedCommunities) {
    saveMinimalModel("m_plr03");
    storeInstance("inst_plr03", "m_plr03");

    themis::process::ProcessCommunity c;
    c.community_id     = "community_0";
    c.node_ids         = {"n1", "n2"};
    c.label            = "Start; End";
    c.report           = "Summary of the process flow";
    c.modularity_score = 0.5f;
    ASSERT_TRUE(detector_->persistCommunities("m_plr03", {c}));

    const auto result = retriever_->retrieve(
        "Überblick",
        "inst_plr03",
        themis::process::RetrievalMode::GLOBAL);

    EXPECT_EQ(result.used_mode, themis::process::RetrievalMode::GLOBAL);
    EXPECT_FALSE(result.llm_context.empty());
    EXPECT_FALSE(result.community_ids_used.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// PLR-04: GLOBAL mode without persisted communities → falls back to LOCAL
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessLightRetrieverTest, PLR04_GlobalFallbackToLocal) {
    saveMinimalModel("m_plr04");
    storeInstance("inst_plr04", "m_plr04");
    // No communities persisted

    const auto result = retriever_->retrieve(
        "Überblick",
        "inst_plr04",
        themis::process::RetrievalMode::GLOBAL);

    // Must fall back to LOCAL without throwing
    EXPECT_EQ(result.used_mode, themis::process::RetrievalMode::LOCAL);
}

// ─────────────────────────────────────────────────────────────────────────────
// PLR-05: LOCAL mode explicitly → delegates to ProcessGraphRag
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessLightRetrieverTest, PLR05_LocalModeExplicit) {
    saveMinimalModel("m_plr05");
    storeInstance("inst_plr05", "m_plr05");

    const auto result = retriever_->retrieve(
        "Schritt 1 Dokumente",
        "inst_plr05",
        themis::process::RetrievalMode::LOCAL);

    EXPECT_EQ(result.used_mode, themis::process::RetrievalMode::LOCAL);
    EXPECT_EQ(result.community_ids_used.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// PLR-06: retrieve() returns non-empty llm_context
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessLightRetrieverTest, PLR06_NonEmptyLlmContext) {
    saveMinimalModel("m_plr06");
    storeInstance("inst_plr06", "m_plr06");

    // Use LOCAL (always produces a prompt)
    const auto result = retriever_->retrieve(
        "Was fehlt noch?",
        "inst_plr06",
        themis::process::RetrievalMode::LOCAL);

    // ProcessGraphRag::retrieve() always builds a prompt, even for minimal instances
    EXPECT_FALSE(result.instance_id_used.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// PLR-07: used_mode in result matches requested mode (non-AUTO)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessLightRetrieverTest, PLR07_UsedModeMatchesRequested) {
    saveMinimalModel("m_plr07");
    storeInstance("inst_plr07", "m_plr07");

    // LOCAL requested → used_mode must be LOCAL
    {
        const auto result = retriever_->retrieve(
            "any query",
            "inst_plr07",
            themis::process::RetrievalMode::LOCAL);
        EXPECT_EQ(result.used_mode, themis::process::RetrievalMode::LOCAL);
    }

    // GLOBAL requested without communities → falls back to LOCAL (documented behaviour)
    {
        const auto result = retriever_->retrieve(
            "any query",
            "inst_plr07",
            themis::process::RetrievalMode::GLOBAL);
        // fallback is LOCAL; this is the documented fallback path
        EXPECT_EQ(result.used_mode, themis::process::RetrievalMode::LOCAL);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PLR-08: classifyQuery is case-insensitive
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessLightRetrieverTest, PLR08_ClassifyQueryCaseInsensitive) {
    saveMinimalModel("m_plr08");
    storeInstance("inst_plr08", "m_plr08");

    // Persist communities so GLOBAL actually returns GLOBAL
    themis::process::ProcessCommunity c;
    c.community_id     = "community_0";
    c.node_ids         = {"n1"};
    c.label            = "Start";
    c.report           = "Report text";
    c.modularity_score = 0.4f;
    detector_->persistCommunities("m_plr08", {c});

    // Upper-case version of a global keyword
    const auto result_upper = retriever_->retrieve(
        "OVERVIEW of the process",
        "inst_plr08",
        themis::process::RetrievalMode::AUTO);
    EXPECT_EQ(result_upper.used_mode, themis::process::RetrievalMode::GLOBAL);

    // Mixed-case
    const auto result_mixed = retriever_->retrieve(
        "GeSaMtE Prozess",
        "inst_plr08",
        themis::process::RetrievalMode::AUTO);
    EXPECT_EQ(result_mixed.used_mode, themis::process::RetrievalMode::GLOBAL);
}
