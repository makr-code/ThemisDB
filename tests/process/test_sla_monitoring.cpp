/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_sla_monitoring.cpp
 * Module:  tests/process/
 * Purpose: GTest unit tests SLA-01..SLA-08 for ProcessGraphRag SLA monitoring.
 */

#include <gtest/gtest.h>

#include "analytics/cep_engine.h"
#include "index/process_graph.h"
#include "process/process_graph_rag.h"
#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace themisdb::analytics;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class SlaMonitoringTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_sla_monitoring_" + std::to_string(::getpid());
        fs::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 32;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 1;

        db_ = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        themis::registerProcessEdgeTypes();

        mgr_    = std::make_unique<themis::process::ProcessModelManager>(*db_);
        linker_ = std::make_unique<themis::process::ProcessLinker>(*db_);

        // ProcessGraphManager lives in namespace themis (not themis::process).
        engine_ = std::make_unique<themis::ProcessGraphManager>(*db_);

        rag_ = std::make_unique<themis::process::ProcessGraphRag>(
            *db_, *engine_, *mgr_, *linker_);
    }

    void TearDown() override {
        CEPEngine& cep = CEPEngine::getInstance();
        if (cep.isInitialized()) {
            cep.shutdown();
        }
        rag_.reset();
        engine_.reset();
        linker_.reset();
        mgr_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    /// Build an initialized CEPEngine for tests that need it.
    static CEPEngine& initializedCep() {
        auto& cep = CEPEngine::getInstance();
        if (!cep.isInitialized()) {
            CEPConfig cfg;
            cfg.worker_threads         = 1;
            cfg.io_threads             = 1;
            cfg.checkpointing_enabled  = false;
            cfg.metrics_enabled        = false;
            cep.initialize(cfg);
        }
        return cep;
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper>               db_;
    std::unique_ptr<themis::process::ProcessModelManager> mgr_;
    std::unique_ptr<themis::process::ProcessLinker>       linker_;
    std::unique_ptr<themis::ProcessGraphManager>          engine_;
    std::unique_ptr<themis::process::ProcessGraphRag>     rag_;
};

// ─────────────────────────────────────────────────────────────────────────────
// SLA-01: registerSlaRule with CEP not initialized → no crash
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SlaMonitoringTest, SLA01_NotInitializedNoCrash) {
    // Use a fresh, uninitialized CEPEngine instance via singleton after shutdown.
    // We test the guard by calling registerSlaRule directly; the method returns
    // early when !cep.isInitialized().
    CEPEngine& cep = CEPEngine::getInstance();
    // If already initialized from a prior test, the early-return is sla_ms guard.
    // We pass sla_ms=5000 but if it's already initialized we rely on no-crash.
    EXPECT_NO_THROW(rag_->registerSlaRule("inst-sla01", 5000, "TestProc", cep));
}

// ─────────────────────────────────────────────────────────────────────────────
// SLA-02: registerSlaRule with sla_ms <= 0 → no crash (returns early)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SlaMonitoringTest, SLA02_ZeroSlaNoCrash) {
    CEPEngine& cep = initializedCep();
    EXPECT_NO_THROW(rag_->registerSlaRule("inst-sla02", 0, "TestProc", cep));
    EXPECT_NO_THROW(rag_->registerSlaRule("inst-sla02", -1, "TestProc", cep));
}

// ─────────────────────────────────────────────────────────────────────────────
// SLA-03: registerSlaRule then deregisterSlaRule → no active entry remains
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SlaMonitoringTest, SLA03_RegisterThenDeregister) {
    CEPEngine& cep = initializedCep();
    rag_->registerSlaRule("inst-sla03", 10000, "TestProc", cep);

    // After deregister, the rules should be removed from CEP and internal map.
    EXPECT_NO_THROW(rag_->deregisterSlaRule("inst-sla03", cep));

    // Rules should no longer be present in the engine.
    EXPECT_FALSE(cep.getRule("sla_at_risk_inst-sla03").has_value());
    EXPECT_FALSE(cep.getRule("sla_overdue_inst-sla03").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// SLA-04: deregisterSlaRule for non-existent instance → no crash
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SlaMonitoringTest, SLA04_DeregisterNonExistentNoCrash) {
    CEPEngine& cep = initializedCep();
    EXPECT_NO_THROW(rag_->deregisterSlaRule("no-such-instance", cep));
}

// ─────────────────────────────────────────────────────────────────────────────
// SLA-05: registerSlaRule → at-risk rule ID is "sla_at_risk_<id>"
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SlaMonitoringTest, SLA05_AtRiskRuleIdCorrect) {
    CEPEngine& cep = initializedCep();
    rag_->registerSlaRule("inst-sla05", 20000, "ProcA", cep);

    auto rule = cep.getRule("sla_at_risk_inst-sla05");
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->rule_id, "sla_at_risk_inst-sla05");

    rag_->deregisterSlaRule("inst-sla05", cep);
}

// ─────────────────────────────────────────────────────────────────────────────
// SLA-06: registerSlaRule → overdue rule ID is "sla_overdue_<id>"
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SlaMonitoringTest, SLA06_OverdueRuleIdCorrect) {
    CEPEngine& cep = initializedCep();
    rag_->registerSlaRule("inst-sla06", 20000, "ProcB", cep);

    auto rule = cep.getRule("sla_overdue_inst-sla06");
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->rule_id, "sla_overdue_inst-sla06");

    rag_->deregisterSlaRule("inst-sla06", cep);
}

// ─────────────────────────────────────────────────────────────────────────────
// SLA-07: registerSlaRule with callback → callback stored (invoked by fireSlaAlert_)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SlaMonitoringTest, SLA07_CallbackStored) {
    CEPEngine& cep = initializedCep();

    bool called = false;
    themis::process::ProcessGraphRag::SlaAlert received;

    rag_->registerSlaRule(
        "inst-sla07", 15000, "ProcC", cep,
        [&](const themis::process::ProcessGraphRag::SlaAlert& a) {
            called    = true;
            received  = a;
        });

    // Rules are registered; callback is wired up.
    EXPECT_TRUE(cep.getRule("sla_at_risk_inst-sla07").has_value());
    EXPECT_TRUE(cep.getRule("sla_overdue_inst-sla07").has_value());

    rag_->deregisterSlaRule("inst-sla07", cep);
    // Callback should not have been called yet (no event submitted).
    EXPECT_FALSE(called);
}

// ─────────────────────────────────────────────────────────────────────────────
// SLA-08: registerSlaRule twice for same instance → second replaces first
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SlaMonitoringTest, SLA08_DoubleRegisterReplaces) {
    CEPEngine& cep = initializedCep();

    rag_->registerSlaRule("inst-sla08", 10000, "ProcD", cep);
    // Second registration with different sla_ms — should replace.
    EXPECT_NO_THROW(
        rag_->registerSlaRule("inst-sla08", 20000, "ProcD-v2", cep));

    // Both rule IDs should still exist (new ones registered, old removed by CEP).
    EXPECT_TRUE(cep.getRule("sla_at_risk_inst-sla08").has_value());
    EXPECT_TRUE(cep.getRule("sla_overdue_inst-sla08").has_value());

    rag_->deregisterSlaRule("inst-sla08", cep);
}
