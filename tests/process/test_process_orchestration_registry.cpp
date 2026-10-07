#include <gtest/gtest.h>

#include "index/process_graph.h"
#include "process/process_graph_rag.h"
#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include "process/process_orchestration_registry.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <variant>

namespace fs = std::filesystem;

class ProcessOrchestrationRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_process_orchestration_registry";
        fs::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.memtable_size_mb = 32;
        cfg.block_cache_size_mb = 64;

        db_ = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        themis::registerProcessEdgeTypes();
        engine_ = std::make_unique<themis::ProcessGraphManager>(*db_);
        manager_ = std::make_unique<themis::process::ProcessModelManager>(*db_);
        linker_ = std::make_unique<themis::process::ProcessLinker>(*db_);
        rag_ = std::make_unique<themis::process::ProcessGraphRag>(
            *db_, *engine_, *manager_, *linker_);
        registry_ = std::make_unique<themis::process::ProcessOrchestrationRegistry>(*db_, *rag_);
    }

    void TearDown() override {
        registry_.reset();
        rag_.reset();
        linker_.reset();
        manager_.reset();
        engine_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::ProcessGraphManager> engine_;
    std::unique_ptr<themis::process::ProcessModelManager> manager_;
    std::unique_ptr<themis::process::ProcessLinker> linker_;
    std::unique_ptr<themis::process::ProcessGraphRag> rag_;
    std::unique_ptr<themis::process::ProcessOrchestrationRegistry> registry_;
};

TEST_F(ProcessOrchestrationRegistryTest, RejectsExternalRagFlag) {
    const nlohmann::json profile = {
        {"id", "ops-default"},
        {"domain", "administration"},
        {"internal_rag_stage", false}
    };

    auto result = registry_->upsertProfile(profile, "inline", "json");
    EXPECT_FALSE(result.ok);
}

TEST_F(ProcessOrchestrationRegistryTest, ActivatesAndArchivesProfiles) {
    const nlohmann::json profile_a = {
        {"id", "admin-default-v1"},
        {"domain", "administration"},
        {"version", "v1"}
    };

    const nlohmann::json profile_b = {
        {"id", "admin-default-v2"},
        {"domain", "administration"},
        {"version", "v2"}
    };

    auto save_a = registry_->upsertProfile(profile_a, "inline", "json");
    ASSERT_TRUE(save_a.ok);

    auto save_b = registry_->upsertProfile(profile_b, "inline", "json");
    ASSERT_TRUE(save_b.ok);

    auto activate = registry_->activateProfile("administration", "admin-default-v2");
    ASSERT_TRUE(activate.ok);

    auto active = registry_->getActiveProfile("ADMINISTRATION");
    ASSERT_TRUE(active.has_value());
    EXPECT_EQ(active->id, "admin-default-v2");
    EXPECT_EQ(active->state, themis::process::OrchestrationProfileState::ACTIVE);

    auto archived = registry_->archiveProfile("admin-default-v2");
    ASSERT_TRUE(archived.ok);
    EXPECT_FALSE(registry_->getActiveProfile("administration").has_value());

    auto list = registry_->listProfiles("ADMINISTRATION");
    EXPECT_EQ(list.size(), 2u);
}

TEST_F(ProcessOrchestrationRegistryTest, PredictOutcomeIncludesRecommendation) {
    const nlohmann::json profile = {
        {"id", "net-default-v1"},
        {"domain", "networks"},
        {"version", "v1"}
    };

    ASSERT_TRUE(registry_->upsertProfile(profile, "inline", "json", true).ok);

    const auto prediction = registry_->predictOutcome("instance-does-not-exist", "networks");
    EXPECT_GE(prediction.sla_breach_probability, 0.0);
    EXPECT_LE(prediction.sla_breach_probability, 1.0);
    EXPECT_GE(prediction.compliance_risk_score, 0.0);
    EXPECT_LE(prediction.compliance_risk_score, 1.0);
    EXPECT_GE(prediction.completion_confidence, 0.0);
    EXPECT_LE(prediction.completion_confidence, 1.0);
    EXPECT_EQ(prediction.recommended_submodel, "net-default-v1");
    ASSERT_TRUE(prediction.evidence.contains("knowledge_source_class"));
}

#if defined(HAVE_YAML_CPP)
TEST_F(ProcessOrchestrationRegistryTest, SyncsProcessEntriesFromCatalog) {
    const fs::path root = fs::path(db_path_) / "assets";
    const fs::path domain = root / "administration";
    ASSERT_TRUE(fs::create_directories(domain));

    const fs::path profile_file = domain / "default_administration_process.yaml";
    std::ofstream p(profile_file);
    p << "id: admin-catalog-v1\n"
      << "domain: administration\n"
      << "version: v1\n";
    p.close();

    const fs::path catalog_file = root / "model_catalog.yaml";
    std::ofstream c(catalog_file);
    c << "models:\n"
      << "  - type: process\n"
      << "    path: administration/default_administration_process.yaml\n";
    c.close();

    auto sync_result = registry_->syncFromCatalog(catalog_file.string(), false);
    ASSERT_TRUE(std::holds_alternative<size_t>(sync_result));
    EXPECT_EQ(std::get<size_t>(sync_result), 1u);

    auto loaded = registry_->getProfile("admin-catalog-v1");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->domain, "ADMINISTRATION");
}
#endif
