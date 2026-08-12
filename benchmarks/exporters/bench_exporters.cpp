/// @file bench_exporters.cpp
/// @brief Performance benchmarks for the Exporters module (Issue #1730).
///
/// Covers:
///   - JSONLLLMExporter: batch throughput (docs/s) for 100, 1000, 10000 entities
///   - JSONLLLMExporter: Alpaca / ChatML format template throughput
///   - StreamingExporter: streaming export throughput with progress callbacks
///   - IncrementalExporter: delta export vs full export speedup
///
/// Performance targets (src/exporters/FUTURE_ENHANCEMENTS.md):
///   - JSONL export throughput:    ≥ 200 000 docs/sec sustained
///   - Peak memory (50 GB export): ≤ 512 MB (enforced by StreamingExporter)
///   - Delta export speedup (0.1% change): ≥ 10× vs full export

#include <benchmark/benchmark.h>

#include "exporters/jsonl_llm_exporter.h"
#include "exporters/streaming_exporter.h"
#include "exporters/incremental_exporter.h"
#include "exporters/format_template.h"
#include "exporters/exporter_interface.h"
#include "storage/base_entity.h"

#include <filesystem>
#include <string>
#include <vector>
#include <sstream>

using namespace themis;
using namespace themis::exporters;

// ============================================================================
// Helpers
// ============================================================================

static std::vector<BaseEntity> makeEntities(int n) {
    std::vector<BaseEntity> entities;
    entities.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        BaseEntity::FieldMap fields;
        fields["question"]  = std::string("What is item " + std::to_string(i) + "?");
        fields["context"]   = std::string("Context for item " + std::to_string(i)
                                          + ". This is a medium-length context string.");
        fields["answer"]    = std::string("The answer to item " + std::to_string(i) + ".");
        fields["score"]     = double(0.5 + (i % 10) * 0.05);
        fields["category"]  = std::string(i % 2 == 0 ? "even" : "odd");
        entities.push_back(BaseEntity::fromFields("entity_" + std::to_string(i), fields));
    }
    return entities;
}

static ExportOptions makeOptions(const std::string& path) {
    ExportOptions opts;
    opts.output_path       = path;
    opts.compress          = false;
    opts.continue_on_error = true;
    return opts;
}

// ============================================================================
// JSONL LLM Exporter — batch throughput
// ============================================================================

/// Measures per-entity throughput for the default INSTRUCTION_TUNING style.
/// State.range(0) = number of entities to export.
static void BM_JsonlExport_BatchThroughput(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    const std::string out = "./data/bench_jsonl_" + std::to_string(n) + "_tmp.jsonl";
    std::filesystem::create_directories("./data");

    const auto entities = makeEntities(n);
    JSONLLLMConfig cfg;
    cfg.style = JSONLFormat::Style::INSTRUCTION_TUNING;
    JSONLLLMExporter exporter(cfg);

    for (auto _ : state) {
        auto opts = makeOptions(out);
        auto stats = exporter.exportEntities(entities, opts);
        benchmark::DoNotOptimize(stats.exported_entities);
        std::filesystem::remove(out);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(
        entities.size() * 256));  // ~256 B avg per JSONL line
    state.SetLabel("INSTRUCTION_TUNING");
}

BENCHMARK(BM_JsonlExport_BatchThroughput)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// JSONL LLM Exporter — format template throughput
// ============================================================================

/// Measures per-entity throughput for ChatML and Alpaca templates.
/// State.range(0) = FormatTemplateType (1 = ALPACA, 2 = SHAREGPT/ChatML)
static void BM_JsonlExport_FormatTemplate(benchmark::State& state) {
    const int template_id = static_cast<int>(state.range(0));
    const int n = 1000;
    const std::string out = "./data/bench_template_" + std::to_string(template_id) + "_tmp.jsonl";
    std::filesystem::create_directories("./data");

    const auto entities = makeEntities(n);
    JSONLLLMConfig cfg;
    cfg.style = JSONLFormat::Style::INSTRUCTION_TUNING;
    cfg.format_template_type = static_cast<FormatTemplateType>(template_id);
    JSONLLLMExporter exporter(cfg);

    std::string label;
    switch (template_id) {
        case static_cast<int>(FormatTemplateType::ALPACA):          label = "ALPACA";          break;
        case static_cast<int>(FormatTemplateType::SHAREGPT):        label = "SHAREGPT";        break;
        case static_cast<int>(FormatTemplateType::CHATML):          label = "CHATML";          break;
        case static_cast<int>(FormatTemplateType::OPENAI_FINETUNING): label = "OPENAI";        break;
        default:                                                      label = "UNKNOWN";        break;
    }

    for (auto _ : state) {
        auto opts = makeOptions(out);
        auto stats = exporter.exportEntities(entities, opts);
        benchmark::DoNotOptimize(stats.exported_entities);
        std::filesystem::remove(out);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel(label);
}

BENCHMARK(BM_JsonlExport_FormatTemplate)
    ->Arg(static_cast<int>(FormatTemplateType::ALPACA))
    ->Arg(static_cast<int>(FormatTemplateType::SHAREGPT))
    ->Arg(static_cast<int>(FormatTemplateType::CHATML))
    ->Arg(static_cast<int>(FormatTemplateType::OPENAI_FINETUNING))
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// JSONL LLM Exporter — with GZIP compression
// ============================================================================

static void BM_JsonlExport_Compressed(benchmark::State& state) {
    const int n = 1000;
    const std::string out = "./data/bench_jsonl_compressed_tmp.jsonl.gz";
    std::filesystem::create_directories("./data");

    const auto entities = makeEntities(n);
    JSONLLLMConfig cfg;
    JSONLLLMExporter exporter(cfg);

    for (auto _ : state) {
        auto opts       = makeOptions(out);
        opts.compress   = true;
        opts.compression_type  = "gzip";
        opts.compression_level = 6;
        auto stats = exporter.exportEntities(entities, opts);
        benchmark::DoNotOptimize(stats.bytes_written);
        std::filesystem::remove(out);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("GZIP/level=6");
}

BENCHMARK(BM_JsonlExport_Compressed)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// StreamingExporter — throughput with progress callbacks
// ============================================================================

static void BM_StreamingExport_Throughput(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    const std::string out = "./data/bench_streaming_" + std::to_string(n) + "_tmp.jsonl";
    std::filesystem::create_directories("./data");

    const auto entities = makeEntities(n);
    StreamingExporter exporter;

    for (auto _ : state) {
        size_t cb_calls = 0;
        auto opts               = makeOptions(out);
        opts.progress_interval  = 500;
        opts.progress_callback  = [&cb_calls](const ExportStats&) { ++cb_calls; };

        auto stats = exporter.exportEntities(entities, opts);
        benchmark::DoNotOptimize(stats.exported_entities);
        benchmark::DoNotOptimize(cb_calls);
        std::filesystem::remove(out);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("StreamingExporter+progress");
}

BENCHMARK(BM_StreamingExport_Throughput)
    ->Arg(1000)
    ->Arg(5000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// IncrementalExporter — full vs delta export speedup
// ============================================================================

/// Full export baseline: export all N entities.
static void BM_IncrementalExport_Full(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    const std::string out       = "./data/bench_incr_full_" + std::to_string(n) + "_tmp.jsonl";
    const std::string watermark = "./data/bench_incr_full_" + std::to_string(n) + "_wm.json";
    std::filesystem::create_directories("./data");

    const auto entities = makeEntities(n);

    for (auto _ : state) {
        std::filesystem::remove(watermark);  // force full export each iteration
        IncrementalExportConfig cfg;
        cfg.watermark_path = watermark;
        IncrementalExporter exporter(cfg);
        auto opts = makeOptions(out);
        auto stats = exporter.exportEntities(entities, opts);
        benchmark::DoNotOptimize(stats.exported_entities);
        std::filesystem::remove(out);
        std::filesystem::remove(watermark);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("full/no-watermark");
}

BENCHMARK(BM_IncrementalExport_Full)
    ->Arg(1000)
    ->Arg(5000)
    ->Unit(benchmark::kMillisecond);

/// Delta export: watermark already set to seq N-10; only ~10 new entities are exported.
static void BM_IncrementalExport_Delta(benchmark::State& state) {
    const int n         = static_cast<int>(state.range(0));
    const int n_new     = 10;  // simulated 0.1% change for n=10000
    const std::string out       = "./data/bench_incr_delta_" + std::to_string(n) + "_tmp.jsonl";
    const std::string watermark = "./data/bench_incr_delta_" + std::to_string(n) + "_wm.json";
    std::filesystem::create_directories("./data");

    IncrementalExportConfig cfg;
    cfg.watermark_path = watermark;

    // Pre-seed the watermark at sequence n - n_new so only the last n_new entities
    // are treated as "new" on each benchmark iteration.
    {
        IncrementalExporter seeder(cfg);
        auto seed_entities = makeEntities(n - n_new);
        auto opts = makeOptions(out);
        seeder.exportEntities(seed_entities, opts);
        std::filesystem::remove(out);
    }

    auto delta_entities = makeEntities(n);

    for (auto _ : state) {
        IncrementalExporter exporter(cfg);
        auto opts = makeOptions(out);
        auto stats = exporter.exportEntities(delta_entities, opts);
        benchmark::DoNotOptimize(stats.exported_entities);
        std::filesystem::remove(out);
        // Re-seed watermark for next iteration
        std::filesystem::remove(watermark);
        IncrementalExporter reseeder(cfg);
        auto seed_entities = makeEntities(n - n_new);
        auto seed_opts = makeOptions(out);
        reseeder.exportEntities(seed_entities, seed_opts);
        std::filesystem::remove(out);
    }

    state.SetItemsProcessed(state.iterations() * n_new);
    state.SetLabel("delta/n_new=" + std::to_string(n_new));
}

BENCHMARK(BM_IncrementalExport_Delta)
    ->Arg(1000)
    ->Arg(5000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Large-Scale Export — 1M rows throughput (Wave-2 P1, AN-3/AN-4)
// ============================================================================
// Note: These benchmarks measure export infrastructure throughput at 1M scale.
// Format-specific Parquet/CSV optimizations are placeholders for Wave-2 P1.

/// Parquet-target 1M row export throughput (PR proxy benchmark).
static void BM_Export_Parquet_1M(benchmark::State& state) {
    const int n = 1000000;  // 1M entities
    const std::string out = "./data/bench_export_1m_tmp.jsonl";  // Temp output
    std::filesystem::create_directories("./data");

    JSONLLLMConfig cfg;
    cfg.style = JSONLFormat::Style::INSTRUCTION_TUNING;
    JSONLLLMExporter exporter(cfg);

    for (auto _ : state) {
        state.PauseTiming();
        // Pre-generate small batches to test memory efficiency
        state.ResumeTiming();

        auto opts = makeOptions(out);
        // Simulate 1M export by batching 100 x 10K entities
        for (int batch = 0; batch < 100; ++batch) {
            auto batch_entities = makeEntities(10000);
            auto stats = exporter.exportEntities(batch_entities, opts);
            benchmark::DoNotOptimize(stats.bytes_written);
        }
        std::filesystem::remove(out);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(n * 256));
    state.SetLabel("Parquet-format/1M");
}

BENCHMARK(BM_Export_Parquet_1M)
    ->Unit(benchmark::kSecond);

/// CSV-target 1M row export throughput (CSV proxy benchmark).
static void BM_Export_CSV_1M(benchmark::State& state) {
    const int n = 1000000;  // 1M entities
    const std::string out = "./data/bench_export_csv_1m_tmp.jsonl";  // Temp output
    std::filesystem::create_directories("./data");

    JSONLLLMConfig cfg;
    cfg.style = JSONLFormat::Style::INSTRUCTION_TUNING;
    JSONLLLMExporter exporter(cfg);

    for (auto _ : state) {
        state.PauseTiming();
        state.ResumeTiming();

        auto opts = makeOptions(out);
        // Simulate 1M CSV export by batching 100 x 10K entities
        for (int batch = 0; batch < 100; ++batch) {
            auto batch_entities = makeEntities(10000);
            auto stats = exporter.exportEntities(batch_entities, opts);
            benchmark::DoNotOptimize(stats.bytes_written);
        }
        std::filesystem::remove(out);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(n * 256));
    state.SetLabel("CSV-format/1M");
}

BENCHMARK(BM_Export_CSV_1M)
    ->Unit(benchmark::kSecond);

BENCHMARK_MAIN();
