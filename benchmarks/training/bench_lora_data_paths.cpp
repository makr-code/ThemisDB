// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_lora_data_paths.cpp
 * @brief Per-source LoRA training data preparation, DB loading, and training benchmarks.
 */

#include <benchmark/benchmark.h>

#include "../benchmark_artifact_preflight.h"
#include "aql/aql_lora_finetuner.h"
#include "index/secondary_index.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/training_data_iterator.h"
#include "query/query_engine.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "training/incremental_lora_trainer.h"
#include "training/training_pipeline.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

class TemporaryTrainingDatabase {
public:
    TemporaryTrainingDatabase() {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = fs::temp_directory_path() / ("themis_lora_data_paths_" + std::to_string(timestamp));
        fs::create_directories(path_);

        themis::RocksDBWrapper::Config config;
        config.db_path = (path_ / "db").string();
        config.create_if_missing = true;
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open benchmark RocksDB fixture");
        }
        indexes_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        query_engine_ = std::make_unique<themis::query::QueryEngine>(*db_, *indexes_);
    }

    ~TemporaryTrainingDatabase() {
        query_engine_.reset();
        indexes_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        std::error_code error;
        fs::remove_all(path_, error);
    }

    TemporaryTrainingDatabase(const TemporaryTrainingDatabase&) = delete;
    TemporaryTrainingDatabase& operator=(const TemporaryTrainingDatabase&) = delete;

    themis::RocksDBWrapper& db() const { return *db_; }
    themis::SecondaryIndexManager& indexes() const { return *indexes_; }
    themis::query::QueryEngine& queryEngine() const { return *query_engine_; }

    void seedTrainingSamples(const std::string& collection, size_t count) {
        for (size_t index = 0; index < count; ++index) {
            themis::BaseEntity entity("sample_" + std::to_string(index));
            entity.setField("instruction", "Explain database record " + std::to_string(index));
            entity.setField("input", "record context " + std::to_string(index));
            entity.setField("output", "A deterministic training answer for record " + std::to_string(index));
            entity.setField("weight", 1.0);
            entity.setField("kind", std::string("training"));
            const auto status = indexes_->put(collection, entity);
            if (!status.ok) {
                throw std::runtime_error("Failed to seed training sample: " + status.message);
            }
        }
    }

    void seedLegalDocuments(size_t count) {
        for (size_t index = 0; index < count; ++index) {
            themis::BaseEntity entity("legal_doc_" + std::to_string(index));
            entity.setField("text", "Der Vertrag muss den Antrag innerhalb der gesetzlichen Frist bearbeiten. "
                                    "Die Behörde soll eine begründete Antwort dokumentieren.");
            entity.setField("source", std::string("benchmark_fixture"));
            const auto status = indexes_->put("legal_documents", entity);
            if (!status.ok) {
                throw std::runtime_error("Failed to seed legal document: " + status.message);
            }
        }
    }

private:
    fs::path path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> indexes_;
    std::unique_ptr<themis::query::QueryEngine> query_engine_;
};

nlohmann::json makeJsonDataset(size_t count) {
    auto dataset = nlohmann::json::array();
    for (size_t index = 0; index < count; ++index) {
        dataset.push_back({
            {"input", "Explain database record " + std::to_string(index)},
            {"output", "A deterministic answer for record " + std::to_string(index)},
            {"metadata", {{"source", "benchmark"}}}
        });
    }
    return dataset;
}

void recordPreparedDataset(benchmark::State& state, const themis::llm::lora::TrainingData& dataset) {
    benchmark::DoNotOptimize(dataset);
    state.counters["samples_per_dataset"] = static_cast<double>(dataset.size());
    state.counters["path_preparation_only"] = 1.0;
}

void benchmarkModelBackedTraining(
    benchmark::State& state,
    const themis::llm::lora::TrainingData& dataset,
    const std::string& path_name) {
    const auto model_path = themis::bench::resolveModelPath();
    if (model_path.empty()) {
        state.SkipWithError("Model-backed training requires a real GGUF base model");
        return;
    }

    themis::llm::lora::LoRATrainingService::Config config;
    config.base_model_path = model_path;
    config.enable_checkpointing = false;
    config.default_hyperparameters.rank = 4;
    config.default_hyperparameters.alpha = 8.0f;
    config.default_hyperparameters.learning_rate = 0.001f;
    config.default_hyperparameters.num_epochs = 1;
    config.default_hyperparameters.batch_size = 4;
    themis::llm::lora::LoRATrainingService service(config);

    double elapsed_ms = 0.0;
    size_t trained_samples = 0;
    for (auto _ : state) {
        const auto start = std::chrono::steady_clock::now();
        const auto result = service.trainOnTheFly(
            "benchmark_" + path_name, dataset, config.default_hyperparameters);
        elapsed_ms += std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        if (!result.success) {
            state.SkipWithError(result.error_message.c_str());
            return;
        }
        trained_samples += dataset.size();
        benchmark::DoNotOptimize(result.final_loss);
    }
    state.SetItemsProcessed(static_cast<int64_t>(trained_samples));
    state.counters["training_samples"] = static_cast<double>(dataset.size());
    state.counters["training_elapsed_ms"] = elapsed_ms;
    state.counters["source_preparation_included"] = 0.0;
}

}  // namespace

static void BM_LoRADataPath_AQLBuiltin(benchmark::State& state) {
    size_t sample_count = 0;
    for (auto _ : state) {
        themis::aql::AQLDatasetBuilder builder;
        auto dataset = builder.addBuiltinSamples().build("benchmark_builtin");
        sample_count = dataset.size();
        benchmark::DoNotOptimize(dataset);
    }
    state.counters["samples_per_dataset"] = static_cast<double>(sample_count);
    state.counters["path_preparation_only"] = 1.0;
}
BENCHMARK(BM_LoRADataPath_AQLBuiltin)->Iterations(100);

static void BM_LoRADataPath_CustomSamples(benchmark::State& state) {
    size_t sample_count = 0;
    for (auto _ : state) {
        themis::aql::AQLDatasetBuilder builder;
        for (size_t index = 0; index < 128; ++index) {
            builder.addCustomSample("Explain record " + std::to_string(index),
                                    "Expected answer " + std::to_string(index));
        }
        auto dataset = builder.build("benchmark_custom");
        sample_count = dataset.size();
        benchmark::DoNotOptimize(dataset);
    }
    state.counters["samples_per_dataset"] = static_cast<double>(sample_count);
    state.counters["path_preparation_only"] = 1.0;
}
BENCHMARK(BM_LoRADataPath_CustomSamples)->Iterations(100);

static void BM_LoRADataPath_JsonObject(benchmark::State& state) {
    const auto json_dataset = makeJsonDataset(128);
    size_t sample_count = 0;
    for (auto _ : state) {
        themis::aql::AQLDatasetBuilder builder;
        auto dataset = builder.loadFromJsonObject(json_dataset).build("benchmark_json_object");
        sample_count = dataset.size();
        benchmark::DoNotOptimize(dataset);
    }
    state.counters["samples_per_dataset"] = static_cast<double>(sample_count);
    state.counters["path_preparation_only"] = 1.0;
}
BENCHMARK(BM_LoRADataPath_JsonObject)->Iterations(100);

static void BM_LoRADataPath_JsonFile(benchmark::State& state) {
    const auto path = fs::temp_directory_path() / "themis_lora_benchmark_dataset.json";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << makeJsonDataset(128).dump();
        if (!output) {
            state.SkipWithError("failed to write benchmark JSON dataset fixture");
            return;
        }
    }

    size_t sample_count = 0;
    for (auto _ : state) {
        themis::aql::AQLDatasetBuilder builder;
        auto dataset = builder.loadFromJson(path.string()).build("benchmark_json_file");
        sample_count = dataset.size();
        benchmark::DoNotOptimize(dataset);
    }
    std::error_code error;
    fs::remove(path, error);
    state.counters["samples_per_dataset"] = static_cast<double>(sample_count);
    state.counters["path_preparation_only"] = 1.0;
}
BENCHMARK(BM_LoRADataPath_JsonFile)->Iterations(100);

static void BM_LoRADataPath_PreparedBatchMerge(benchmark::State& state) {
    std::vector<themis::llm::lora::TrainingData> datasets;
    for (size_t dataset_index = 0; dataset_index < 4; ++dataset_index) {
        themis::aql::AQLDatasetBuilder builder;
        for (size_t sample_index = 0; sample_index < 32; ++sample_index) {
            builder.addCustomSample("Input " + std::to_string(dataset_index) + ":" + std::to_string(sample_index),
                                    "Output " + std::to_string(sample_index));
        }
        datasets.push_back(builder.build("prepared_batch_" + std::to_string(dataset_index)));
    }

    size_t merged_count = 0;
    for (auto _ : state) {
        themis::llm::lora::TrainingData combined;
        combined.dataset_name = "combined_batch";
        for (const auto& dataset : datasets) {
            combined.samples.insert(combined.samples.end(), dataset.samples.begin(), dataset.samples.end());
        }
        merged_count = combined.size();
        benchmark::DoNotOptimize(combined);
    }
    state.counters["samples_per_batch"] = static_cast<double>(merged_count);
    state.counters["path_preparation_only"] = 1.0;
}
BENCHMARK(BM_LoRADataPath_PreparedBatchMerge)->Iterations(100);

static void BM_LoRATrainFromAQLBuiltin(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveModelPath(), "base GGUF model");
    themis::aql::AQLDatasetBuilder builder;
    const auto dataset = builder.addBuiltinSamples().build("benchmark_builtin");
    benchmarkModelBackedTraining(state, dataset, "aql_builtin");
}
BENCHMARK(BM_LoRATrainFromAQLBuiltin)->Iterations(1);

static void BM_LoRATrainFromCustomSamples(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveModelPath(), "base GGUF model");
    themis::aql::AQLDatasetBuilder builder;
    for (size_t index = 0; index < 16; ++index) {
        builder.addCustomSample("Explain record " + std::to_string(index),
                                "Expected answer " + std::to_string(index));
    }
    const auto dataset = builder.build("benchmark_custom");
    benchmarkModelBackedTraining(state, dataset, "custom_samples");
}
BENCHMARK(BM_LoRATrainFromCustomSamples)->Iterations(1);

static void BM_LoRATrainFromJsonObject(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveModelPath(), "base GGUF model");
    const auto json_dataset = makeJsonDataset(16);
    themis::aql::AQLDatasetBuilder builder;
    const auto dataset = builder.loadFromJsonObject(json_dataset).build("benchmark_json_object");
    benchmarkModelBackedTraining(state, dataset, "json_object");
}
BENCHMARK(BM_LoRATrainFromJsonObject)->Iterations(1);

static void BM_LoRATrainFromJsonFile(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveModelPath(), "base GGUF model");
    const auto path = fs::temp_directory_path() / "themis_lora_benchmark_training_dataset.json";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << makeJsonDataset(16).dump();
        if (!output) {
            state.SkipWithError("failed to write model-backed JSON dataset fixture");
            return;
        }
    }
    themis::aql::AQLDatasetBuilder builder;
    const auto dataset = builder.loadFromJson(path.string()).build("benchmark_json_file");
    std::error_code error;
    fs::remove(path, error);
    benchmarkModelBackedTraining(state, dataset, "json_file");
}
BENCHMARK(BM_LoRATrainFromJsonFile)->Iterations(1);

static void BM_LoRATrainFromPreparedBatch(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, themis::bench::resolveModelPath(), "base GGUF model");
    std::vector<themis::llm::lora::TrainingData> datasets;
    for (size_t dataset_index = 0; dataset_index < 2; ++dataset_index) {
        themis::aql::AQLDatasetBuilder builder;
        for (size_t sample_index = 0; sample_index < 8; ++sample_index) {
            builder.addCustomSample("Input " + std::to_string(dataset_index) + ":" + std::to_string(sample_index),
                                    "Output " + std::to_string(sample_index));
        }
        datasets.push_back(builder.build("prepared_batch_" + std::to_string(dataset_index)));
    }
    themis::llm::lora::TrainingData combined;
    combined.dataset_name = "combined_batch";
    for (const auto& dataset : datasets) {
        combined.samples.insert(combined.samples.end(), dataset.samples.begin(), dataset.samples.end());
    }
    benchmarkModelBackedTraining(state, combined, "prepared_batch");
}
BENCHMARK(BM_LoRATrainFromPreparedBatch)->Iterations(1);

static void BM_LoRADataPath_TrainingDataIteratorAql(benchmark::State& state) {
    const size_t sample_count = static_cast<size_t>(state.range(0));
    TemporaryTrainingDatabase database;
    database.seedTrainingSamples("iterator_samples", sample_count);

    themis::exporters::JSONLLLMConfig exporter_config;
    exporter_config.field_mapping.instruction_field = "instruction";
    exporter_config.field_mapping.input_field = "input";
    exporter_config.field_mapping.output_field = "output";
    exporter_config.quality.min_text_length = 1;
    exporter_config.quality.max_text_length = 4096;
    auto exporter = std::make_shared<themis::exporters::JSONLLLMExporter>(exporter_config);

    themis::llm::TrainingDataIterator::Config config;
    config.batch_size = 32;
    config.max_samples = sample_count;
    config.shuffle_data = false;

    double total_read_ms = 0.0;
    double total_batch_ms = 0.0;
    size_t total_samples = 0;
    themis::llm::AdapterMetadata metadata;
    for (auto _ : state) {
        themis::llm::TrainingDataIterator iterator(
            std::shared_ptr<themis::RocksDBWrapper>(&database.db(), [](themis::RocksDBWrapper*) {}),
            exporter,
            config,
            &database.queryEngine());
        const auto read_start = std::chrono::steady_clock::now();
        if (!iterator.initialize("FOR sample IN iterator_samples RETURN sample", metadata)) {
            state.SkipWithError(iterator.lastError().c_str());
            return;
        }
        const auto read_end = std::chrono::steady_clock::now();
        while (iterator.hasNext()) {
            auto batch = iterator.getNextBatch();
            benchmark::DoNotOptimize(batch);
        }
        const auto batch_end = std::chrono::steady_clock::now();
        total_read_ms += std::chrono::duration<double, std::milli>(read_end - read_start).count();
        total_batch_ms += std::chrono::duration<double, std::milli>(batch_end - read_end).count();
        total_samples += iterator.getStats().processed_samples;
    }
    state.SetItemsProcessed(static_cast<int64_t>(total_samples));
    state.counters["query_and_convert_ms"] = benchmark::Counter(total_read_ms, benchmark::Counter::kAvgIterations);
    state.counters["batch_iteration_ms"] = benchmark::Counter(total_batch_ms, benchmark::Counter::kAvgIterations);
    state.counters["samples_per_second"] = benchmark::Counter(
        static_cast<double>(total_samples), benchmark::Counter::kIsRate);
    state.counters["dataset_samples"] = static_cast<double>(sample_count);
}
BENCHMARK(BM_LoRADataPath_TrainingDataIteratorAql)->Arg(128)->Arg(512);

static void BM_LoRADataPath_IncrementalTrainerCollection(benchmark::State& state) {
    const size_t sample_count = static_cast<size_t>(state.range(0));
    TemporaryTrainingDatabase database;
    database.seedTrainingSamples("incremental_samples", sample_count);

    themis::training::IncrementalTrainingConfig config;
    config.training_data_collection = "incremental_samples";
    config.max_training_samples = sample_count;
    config.rank = 4;
    config.alpha = 8.0f;
    config.learning_rate = 0.001f;
    config.batch_size = 8;
    config.num_epochs = 1;
    config.max_seq_length = 32;
    config.device = "cpu";

    double total_read_ms = 0.0;
    double total_train_ms = 0.0;
    size_t total_samples = 0;
    for (auto _ : state) {
        themis::training::IncrementalLoRATrainer trainer(config, "", &database.queryEngine());
        const auto start = std::chrono::steady_clock::now();
        const auto result = trainer.train(themis::training::TrainingMode::INITIAL);
        const auto end = std::chrono::steady_clock::now();
        if (!result.success || result.samples_trained != sample_count) {
            state.SkipWithError(result.error_message.empty()
                ? "Incremental trainer did not consume the expected DB samples"
                : result.error_message.c_str());
            return;
        }
        total_read_ms += result.data_load_time_seconds * 1000.0;
        total_train_ms += std::chrono::duration<double, std::milli>(end - start).count() -
                          result.data_load_time_seconds * 1000.0;
        total_samples += result.samples_trained;
        benchmark::DoNotOptimize(result.training_loss);
    }
    state.SetItemsProcessed(static_cast<int64_t>(total_samples));
    state.counters["db_read_ms"] = benchmark::Counter(total_read_ms, benchmark::Counter::kAvgIterations);
    state.counters["training_update_ms"] = benchmark::Counter(total_train_ms, benchmark::Counter::kAvgIterations);
    state.counters["samples_per_second"] = benchmark::Counter(
        static_cast<double>(total_samples), benchmark::Counter::kIsRate);
    state.counters["dataset_samples"] = static_cast<double>(sample_count);
}
BENCHMARK(BM_LoRADataPath_IncrementalTrainerCollection)->Arg(32)->Arg(128);

static void BM_LoRADataPath_TrainingPipelineEndToEnd(benchmark::State& state) {
    TemporaryTrainingDatabase database;
    database.seedLegalDocuments(8);
    size_t iteration = 0;
    double total_db_read_ms = 0.0;
    double total_pipeline_ms = 0.0;
    size_t total_samples = 0;

    for (auto _ : state) {
        const auto collection = "pipeline_samples_" + std::to_string(iteration++);
        themis::training::PipelineConfig config;
        config.labeler_config.source_collection = "legal_documents";
        config.labeler_config.target_collection = collection;
        config.labeler_config.min_confidence = 0.3f;
        config.labeler_config.flag_low_confidence = false;
        config.labeler_config.batch_size = 16;
        config.trainer_config.training_data_collection = collection;
        config.trainer_config.max_training_samples = 256;
        config.trainer_config.rank = 4;
        config.trainer_config.alpha = 8.0f;
        config.trainer_config.learning_rate = 0.001f;
        config.trainer_config.batch_size = 4;
        config.trainer_config.num_epochs = 1;
        config.trainer_config.max_seq_length = 32;
        config.trainer_config.device = "cpu";
        config.data_selection_config.min_length_tokens = 1;
        config.data_selection_config.enable_pii_check = false;
        config.data_selection_config.target_samples = 2;
        config.data_selection_config.audit = false;
        config.enable_enrichment = false;
        config.enable_data_selection = true;
        config.enable_quality_checks = false;
        config.enable_drift_detection = false;

        themis::training::TrainingPipeline pipeline(
            config, "", &database.queryEngine(), &database.indexes());
        const auto start = std::chrono::steady_clock::now();
        const auto stats = pipeline.run();
        const auto end = std::chrono::steady_clock::now();
        state.counters["samples_persisted"] = static_cast<double>(stats.samples_persisted);
        state.counters["samples_selected"] = static_cast<double>(stats.selection_output_count);
        state.counters["samples_trained"] = static_cast<double>(stats.training_samples_consumed);
        if (!stats.error_message.empty() || !stats.training_success ||
            stats.selection_output_count == 0 ||
            stats.training_samples_consumed != stats.selection_output_count) {
            const std::string error = stats.error_message.empty()
                ? "DB pipeline selected=" + std::to_string(stats.selection_output_count) +
                    " persisted=" + std::to_string(stats.samples_persisted) +
                    " trained=" + std::to_string(stats.training_samples_consumed) +
                    " success=" + std::to_string(stats.training_success)
                : stats.error_message;
            state.SkipWithError(error.c_str());
            return;
        }
        total_db_read_ms += stats.data_load_time_seconds * 1000.0;
        total_pipeline_ms += std::chrono::duration<double, std::milli>(end - start).count();
        total_samples += stats.training_samples_consumed;
        benchmark::DoNotOptimize(stats.training_loss);
    }
    state.SetItemsProcessed(static_cast<int64_t>(total_samples));
    state.counters["db_read_ms"] = benchmark::Counter(total_db_read_ms, benchmark::Counter::kAvgIterations);
    state.counters["pipeline_end_to_end_ms"] = benchmark::Counter(
        total_pipeline_ms, benchmark::Counter::kAvgIterations);
    state.counters["samples_per_second"] = total_pipeline_ms > 0.0
        ? static_cast<double>(total_samples) * 1000.0 / total_pipeline_ms
        : 0.0;
}
BENCHMARK(BM_LoRADataPath_TrainingPipelineEndToEnd)->Iterations(5);

static void BM_LoRADataPath_TrainBatchWithModel(benchmark::State& state) {
    THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(
        state, themis::bench::resolveModelPath(), "base GGUF model");

    struct ScopedCheckpointDirectory {
        fs::path path = fs::temp_directory_path() /
            ("themis_lora_train_batch_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));

        ScopedCheckpointDirectory() { fs::create_directories(path); }
        ~ScopedCheckpointDirectory() {
            std::error_code error;
            fs::remove_all(path, error);
        }
    } checkpoint_directory;

    const auto model_path = themis::bench::resolveModelPath();
    themis::llm::lora::LoRATrainingService::Config config;
    config.base_model_path = model_path;
    config.enable_checkpointing = true;
    config.checkpoint_dir = checkpoint_directory.path.string();
    themis::llm::lora::LoRATrainingService service(config);

    std::vector<themis::llm::lora::TrainingData> datasets;
    for (size_t dataset_index = 0; dataset_index < 4; ++dataset_index) {
        themis::llm::lora::TrainingData data;
        data.dataset_name = "train_batch_" + std::to_string(dataset_index);
        for (size_t sample_index = 0; sample_index < 16; ++sample_index) {
            themis::llm::lora::TrainingDataSample sample;
            sample.input = "Explain sample " + std::to_string(sample_index);
            sample.output = "Expected answer " + std::to_string(sample_index);
            data.samples.push_back(std::move(sample));
        }
        datasets.push_back(std::move(data));
    }

    size_t total_samples = 0;
    for (auto _ : state) {
        const auto result = service.trainBatch("benchmark_train_batch", datasets);
        if (!result.success) {
            state.SkipWithError(result.error_message.c_str());
            return;
        }
        total_samples += 64;
        benchmark::DoNotOptimize(result.success);
    }
    state.SetItemsProcessed(static_cast<int64_t>(total_samples));
}
BENCHMARK(BM_LoRADataPath_TrainBatchWithModel)->Iterations(1);

BENCHMARK_MAIN();
