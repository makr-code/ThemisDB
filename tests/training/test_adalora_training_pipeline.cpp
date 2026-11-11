// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "llm/applications/themis_help_lora.h"
#include "exporters/jsonl_llm_exporter.h"
#include "llm/llama_wrapper.h"
#include "llm/training_data_iterator.h"
#include "query/query_engine.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "training/adalora_training_pipeline.h"
#include "utils/rocksdb_open_compat.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <string>
#include <system_error>

namespace {

class TemporaryTrainingDirectory {
public:
    TemporaryTrainingDirectory() {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("themis_adalora_pipeline_" + std::to_string(nonce));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryTrainingDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

TEST(AdaLoRATrainingPipelineTest, TrainsFromAqlExportsAndServesTinyLlamaAdapter) {
    const char* model_path_env = std::getenv("THEMIS_TEST_MODEL_PATH");
    if (!model_path_env || !std::filesystem::is_regular_file(model_path_env)) {
        GTEST_SKIP() << "Set THEMIS_TEST_MODEL_PATH to a TinyLlama GGUF model";
    }

    TemporaryTrainingDirectory temporary_directory;
    const auto root = temporary_directory.path();
    const auto db_path = root / "docs.db";
    const auto adapter_path = root / "tinyllama_docs.gguf";
    const auto checkpoint_path = root / "tinyllama_docs.adalora";

    themis::RocksDBWrapper::Config db_config;
    db_config.db_path = db_path.string();
    db_config.create_if_missing = true;
    auto db = std::make_shared<themis::RocksDBWrapper>(db_config);
    ASSERT_TRUE(db->open());
    auto indexes = std::make_unique<themis::SecondaryIndexManager>(*db);
    auto query_engine = std::make_unique<themis::query::QueryEngine>(*db, *indexes);

    themis::BaseEntity sample("sample_001");
    sample.setField("instruction", std::string("Answer using ThemisDB documentation."));
    sample.setField("context", std::string("Question: What storage engine stores documents?"));
    sample.setField("output", std::string("ThemisDB stores documents in RocksDB."));
    sample.setField("weight", 1.0);
    sample.setField("kind", std::string("training"));
    ASSERT_TRUE(indexes->put("training_samples", sample).ok);

    themis::exporters::JSONLLLMConfig exporter_config;
    exporter_config.style = themis::exporters::JSONLFormat::Style::INSTRUCTION_TUNING;
    exporter_config.field_mapping.instruction_field = "instruction";
    exporter_config.field_mapping.input_field = "context";
    exporter_config.field_mapping.output_field = "output";
    exporter_config.weighting.enable_weights = true;
    exporter_config.weighting.weight_field = "weight";
    exporter_config.quality.min_text_length = 1;
    exporter_config.quality.max_text_length = 512;
    auto exporter = std::make_shared<themis::exporters::JSONLLLMExporter>(exporter_config);

    themis::llm::TrainingDataIterator::Config iterator_config;
    iterator_config.batch_size = 1;
    iterator_config.max_samples = 1;
    iterator_config.shuffle_data = false;
    themis::llm::TrainingDataIterator iterator(
        db, exporter, iterator_config, query_engine.get());

    themis::llm::LlamaWrapper::Config model_config;
    model_config.n_ctx = 64;
    model_config.n_batch = 64;
    model_config.n_gpu_layers = 0;
    model_config.n_threads = 4;
    model_config.multi_lora_config.lora_base_dir = root.string();
    themis::llm::LlamaWrapper model(model_config);

    themis::training::AdaLoRATrainingConfig training_config;
    training_config.adapter_id = "tinyllama_docs_e2e";
    training_config.base_model_path = model_path_env;
    training_config.aql_query = "FOR sample IN training_samples RETURN sample";
    training_config.adapter_output_path = adapter_path.string();
    training_config.checkpoint_path = checkpoint_path.string();
    training_config.max_samples = 1;
    training_config.max_sequence_length = 32;
    training_config.output_shard_count = 32;
    training_config.initial_rank = 1;
    training_config.rank_budget = 32;
    training_config.reallocate_every_steps = 1;
    training_config.epochs = 1;
    training_config.alpha = 2.0f;
    training_config.learning_rate = 0.01f;
    training_config.max_gradient_norm = 1.0f;

    const auto result = themis::training::AdaLoRATrainingPipeline::train(
        iterator, model, training_config);
    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.samples_read, 1u);
    EXPECT_EQ(result.samples_trained, 1u);
    EXPECT_GT(result.target_tokens_trained, 0u);
    EXPECT_EQ(result.final_rank, training_config.rank_budget);
    EXPECT_TRUE(std::isfinite(result.mean_loss));
    EXPECT_TRUE(std::filesystem::is_regular_file(checkpoint_path));
    ASSERT_TRUE(std::filesystem::is_regular_file(adapter_path));
    EXPECT_GT(std::filesystem::file_size(adapter_path), 1024u);

    themis::llm::InferenceRequest request;
    request.prompt = "ThemisDB documentation says its document storage engine is";
    request.max_tokens = 6;
    request.temperature = 0.0f;
    request.lora_adapter_id = training_config.adapter_id;
    const auto response = model.generate(request);
    EXPECT_FALSE(response.text.empty());
    ASSERT_TRUE(response.lora_used.has_value());
    EXPECT_EQ(*response.lora_used, training_config.adapter_id);

    query_engine.reset();
    indexes.reset();
    exporter.reset();
    db->close();
    db.reset();
}

TEST(AdaLoRATrainingPipelineTest, TrainsFromDocsDatabaseAndServesTinyLlamaAdapter) {
    const char* model_path_env = std::getenv("THEMIS_TEST_MODEL_PATH");
    if (!model_path_env || !std::filesystem::is_regular_file(model_path_env)) {
        GTEST_SKIP() << "Set THEMIS_TEST_MODEL_PATH to a TinyLlama GGUF model";
    }

    TemporaryTrainingDirectory temporary_directory;
    const auto root = temporary_directory.path();
    const auto db_path = root / "docs.db";
    const auto adapter_path = root / "tinyllama_docsdb.gguf";
    const auto checkpoint_path = root / "tinyllama_docsdb.adalora";
    const std::string documentation_text =
        "ThemisDB stores documents in RocksDB and exposes read-only AQL queries for "
        "documentation. Training providers read bounded documentation chunks without "
        "modifying the source database. AdaLoRA learns a small output adapter while "
        "TinyLlama remains frozen, and the exported GGUF can be loaded for inference.";

    {
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::DB* raw_db = nullptr;
        const auto open_status = themis::storage::detail::openDbCompat(
            options, db_path.string(), &raw_db);
        ASSERT_TRUE(open_status.ok()) << open_status.ToString();
        std::unique_ptr<rocksdb::DB> docs_db(raw_db);
        ASSERT_TRUE(docs_db);
        const nlohmann::json chunk = {
            {"chunk_id", "chunk-0001"},
            {"doc_id", "docs/training/rocksdb.md"},
            {"text", documentation_text},
            {"token_count", 56}
        };
        ASSERT_TRUE(docs_db->Put(rocksdb::WriteOptions{}, "chunk_000000", chunk.dump()).ok());
    }

    themis::llm::LlamaWrapper::Config model_config;
    model_config.n_ctx = 128;
    model_config.n_batch = 128;
    model_config.n_gpu_layers = 0;
    model_config.n_threads = 4;
    model_config.multi_lora_config.lora_base_dir = root.string();
    themis::llm::LlamaWrapper model(model_config);

    themis::training::AdaLoRATrainingConfig training_config;
    training_config.adapter_id = "tinyllama_docsdb_e2e";
    training_config.base_model_path = model_path_env;
    training_config.adapter_output_path = adapter_path.string();
    training_config.checkpoint_path = checkpoint_path.string();
    training_config.max_samples = 1;
    training_config.provider_batch_size = 1;
    training_config.max_sequence_length = 64;
    training_config.output_shard_count = 32;
    training_config.initial_rank = 1;
    training_config.rank_budget = 32;
    training_config.reallocate_every_steps = 1;
    training_config.epochs = 1;
    training_config.alpha = 2.0f;
    training_config.learning_rate = 0.01f;
    training_config.max_gradient_norm = 1.0f;

    const auto result = themis::training::AdaLoRATrainingPipeline::trainFromDocsDatabase(
        db_path.string(), model, training_config);
    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.samples_read, 1u);
    EXPECT_EQ(result.samples_trained, 1u);
    EXPECT_GT(result.target_tokens_trained, 0u);
    EXPECT_EQ(result.final_rank, training_config.rank_budget);
    EXPECT_TRUE(std::isfinite(result.mean_loss));
    EXPECT_TRUE(std::filesystem::is_regular_file(checkpoint_path));
    ASSERT_TRUE(std::filesystem::is_regular_file(adapter_path));

    themis::llm::InferenceRequest request;
    request.prompt = "ThemisDB stores documents in";
    request.max_tokens = 6;
    request.temperature = 0.0f;
    request.lora_adapter_id = training_config.adapter_id;
    const auto response = model.generate(request);
    EXPECT_FALSE(response.text.empty());
    ASSERT_TRUE(response.lora_used.has_value());
    EXPECT_EQ(*response.lora_used, training_config.adapter_id);
}

} // namespace