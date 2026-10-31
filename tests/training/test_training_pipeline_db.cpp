// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "training/training_pipeline.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace {

std::string makeTempDbPath() {
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() /
            ("themis_training_pipeline_db_" + std::to_string(timestamp))).string();
}

class TrainingPipelineDbTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = makeTempDbPath();
        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.create_if_missing = true;
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        indexes_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        query_engine_ = std::make_unique<themis::query::QueryEngine>(*db_, *indexes_);

        insertDocument("doc_1", "Der Verwalter muss den Antrag innerhalb der gesetzlichen Frist bearbeiten.");
        insertDocument("doc_2", "Die Behörde darf die Unterlagen nach Prüfung anfordern.");
        insertDocument("doc_3", "Der Vertrag soll eine nachvollziehbare Begründung enthalten.");
    }

    void TearDown() override {
        query_engine_.reset();
        indexes_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    void insertDocument(const std::string& key, const std::string& text) {
        themis::BaseEntity document(key);
        document.setField("text", text);
        document.setField("source", std::string("training_benchmark_fixture"));
        ASSERT_TRUE(indexes_->put("legal_documents", document).ok);
    }

    themis::training::PipelineConfig makeConfig() const {
        themis::training::PipelineConfig config;
        config.labeler_config.source_collection = "legal_documents";
        config.labeler_config.target_collection = "legal_training_samples";
        config.labeler_config.min_confidence = 0.3f;
        config.labeler_config.flag_low_confidence = false;
        config.labeler_config.batch_size = 2;

        config.trainer_config.training_data_collection = "legal_training_samples";
        config.trainer_config.max_training_samples = 64;
        config.trainer_config.rank = 2;
        config.trainer_config.alpha = 4.0f;
        config.trainer_config.learning_rate = 0.001f;
        config.trainer_config.batch_size = 2;
        config.trainer_config.num_epochs = 1;
        config.trainer_config.max_seq_length = 16;
        config.trainer_config.device = "cpu";

        config.enable_labeling = true;
        config.enable_enrichment = false;
        config.enable_data_selection = false;
        config.enable_training = true;
        config.enable_quality_checks = false;
        config.enable_drift_detection = false;
        return config;
    }

    std::string db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> indexes_;
    std::unique_ptr<themis::query::QueryEngine> query_engine_;
};

TEST_F(TrainingPipelineDbTest, LabelsPersistsAndTrainsOnTheSameDatabaseSamples) {
    auto config = makeConfig();
    themis::training::TrainingPipeline pipeline(
        config, "", query_engine_.get(), indexes_.get());

    const auto stats = pipeline.run();

    EXPECT_TRUE(stats.error_message.empty()) << stats.error_message;
    EXPECT_EQ(stats.documents_labeled, 3u);
    EXPECT_GT(stats.samples_created, 0u);
    EXPECT_EQ(stats.samples_persisted, stats.samples_created);
    EXPECT_TRUE(stats.training_success);
    EXPECT_EQ(stats.training_samples_consumed, stats.samples_persisted);
    EXPECT_GE(stats.data_load_time_seconds, 0.0);
}

TEST_F(TrainingPipelineDbTest, MissingSampleWriterFailsBeforeTraining) {
    auto config = makeConfig();
    themis::training::TrainingPipeline pipeline(config, "", query_engine_.get(), nullptr);

    const auto stats = pipeline.run();

    EXPECT_FALSE(stats.error_message.empty());
    EXPECT_FALSE(stats.training_success);
    EXPECT_EQ(stats.samples_persisted, 0u);
}

}  // namespace
