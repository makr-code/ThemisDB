/**
 * @file bench_legal_lora_pipeline.cpp
 * @brief Performance benchmarks for Legal LoRA Training Pipeline
 * 
 * Benchmarks key operations:
 * - Document ingestion throughput
 * - Auto-labeling performance
 * - Graph enrichment speed
 * - Training throughput
 */

#include <benchmark/benchmark.h>
#include "ingestion/ingestion_manager.h"
#include "ingestion/filesystem_ingester.h"
#include "ingestion/web_crawler_connector.h"
#include "training/auto_labeler.h"
#include "training/knowledge_graph_enricher.h"
#include "training/incremental_lora_trainer.h"

#if !defined(THEMIS_ENABLE_LEGAL_TRAINING)

static void BM_LegalLoRAPipeline_Disabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("Legal LoRA pipeline benchmarks are disabled in this build");
        break;
    }
}
// Disabled: legal LoRA pipeline requires model artifacts not available in CI | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_LegalLoRAPipeline_Disabled);

BENCHMARK_MAIN();

#else

using namespace themis;

// =============================================================================
// Ingestion Benchmarks
// =============================================================================

static void BM_FileSystemIngestion(benchmark::State& state) {
    // Setup
    ingestion::FileSystemIngester ingester;
    ingestion::SourceConfig config;
    config.source_id = "bench";
    config.type = ingestion::SourceType::FILESYSTEM;
    config.location = "/tmp/bench_docs";
    ingester.initialize(config);
    
    // Benchmark
    for (auto _ : state) {
        auto stats = ingester.ingest("bench_collection", nullptr);
        benchmark::DoNotOptimize(stats);
    }
    
    // Metrics
    state.SetItemsProcessed(state.iterations() * 1000);  // Assume 1000 docs
    state.SetLabel("docs/sec");
}
BENCHMARK(BM_FileSystemIngestion)->Unit(benchmark::kMillisecond);

static void BM_IngestionParallel(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    ingestion::IngestionManager mgr("bench_db");
    mgr.setParallelProcessing(true, num_threads);
    
    for (auto _ : state) {
        auto report = mgr.ingestAll(nullptr);
        benchmark::DoNotOptimize(report);
    }
    
    state.SetLabel(std::to_string(num_threads) + " threads");
}
BENCHMARK(BM_IngestionParallel)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Web Crawler Ingestion Benchmark
// =============================================================================

/**
 * @brief Benchmark: WebCrawlerConnector HTML extraction throughput.
 *
 * Uses a mock HTTP fetch function that returns pre-generated HTML pages so
 * the benchmark measures the BFS + text-extraction logic without real network
 * I/O.  Target: ≥ 10 000 pages/sec on a single thread.
 */
static void BM_WebCrawlerIngestion(benchmark::State& state) {
    const int num_pages = state.range(0);

    // Build a simple site: seed links to /p1 .. /pN, each page has text content.
    std::string seed_html = {};
    seed_html.reserve(64 + num_pages * 30);
    seed_html = "<html><body>";
    for (int i = 1; i <= num_pages; ++i) {
        seed_html += "<a href=\"/p" + std::to_string(i) + "\">page</a>";
    }
    seed_html += "seed content</body></html>";
    const std::string leaf_html = "<html><body>leaf page content</body></html>";

    ingestion::SourceConfig cfg;
    cfg.source_id = "bench_crawl";
    cfg.type      = ingestion::SourceType::WEB_CRAWLER;
    cfg.location  = "http://bench.example.com";
    cfg.options["max_depth"]       = "1";
    cfg.options["max_pages"]       = std::to_string(num_pages + 1);
    cfg.options["follow_sitemaps"] = "false";
    cfg.options["respect_robots"]  = "false";

    for (auto _ : state) {
        ingestion::WebCrawlerConnector conn;
        conn.initialize(cfg);
        conn.setHttpFetchForTesting([&](const std::string& url)
                -> std::pair<int, std::string> {
            if (url == "http://bench.example.com") return {200, seed_html};
            return {200, leaf_html};
        });
        auto stats = conn.ingest("bench_collection", nullptr);
        benchmark::DoNotOptimize(stats);
    }

    state.SetItemsProcessed(state.iterations() * (num_pages + 1));
    state.SetLabel("pages/sec");
}
BENCHMARK(BM_WebCrawlerIngestion)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(5000)
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Auto-Labeling Benchmarks
// =============================================================================

static void BM_AutoLabeling(benchmark::State& state) {
    training::AutoLabelConfig config;
    config.source_collection = "bench_docs";
    config.target_collection = "bench_samples";
    config.batch_size = state.range(0);
    
    training::LegalAutoLabeler labeler(config, "bench_db");
    
    for (auto _ : state) {
        auto stats = labeler.labelAll(nullptr);
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetLabel("batch_size=" + std::to_string(state.range(0)));
}
BENCHMARK(BM_AutoLabeling)
    ->Arg(50)
    ->Arg(100)
    ->Arg(200)
    ->Unit(benchmark::kMillisecond);

static void BM_LegalModalityExtraction(benchmark::State& state) {
    training::AutoLabelConfig config;
    training::LegalAutoLabeler labeler(config, "bench_db");
    
    const std::string test_doc_id = "bench_doc_123";
    
    for (auto _ : state) {
        auto samples = labeler.labelDocument(test_doc_id);
        benchmark::DoNotOptimize(samples);
    }
    
    state.SetLabel("per document");
}
BENCHMARK(BM_LegalModalityExtraction)->Unit(benchmark::kMicrosecond);

// =============================================================================
// Graph Enrichment Benchmarks
// =============================================================================

static void BM_GraphTraversal(benchmark::State& state) {
    training::EnrichmentConfig config;
    config.graph_name = "bench_graph";
    config.traversal_depth = state.range(0);
    
    training::KnowledgeGraphEnricher enricher(config, "bench_db");
    
    const std::string test_sample_id = "bench_sample_123";
    
    for (auto _ : state) {
        auto context = enricher.enrichSample(test_sample_id);
        benchmark::DoNotOptimize(context);
    }
    
    state.SetLabel("depth=" + std::to_string(state.range(0)));
}
BENCHMARK(BM_GraphTraversal)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Unit(benchmark::kMicrosecond);

static void BM_SemanticSimilaritySearch(benchmark::State& state) {
    training::EnrichmentConfig config;
    config.similarity_threshold = 0.7f;
    
    training::KnowledgeGraphEnricher enricher(config, "bench_db");
    
    const std::string test_doc_id = "bench_doc_123";
    const size_t max_results = state.range(0);
    
    for (auto _ : state) {
        auto similar = enricher.findSimilarDocuments(test_doc_id, max_results);
        benchmark::DoNotOptimize(similar);
    }
    
    state.SetLabel("top_k=" + std::to_string(max_results));
}
BENCHMARK(BM_SemanticSimilaritySearch)
    ->Arg(5)
    ->Arg(10)
    ->Arg(20)
    ->Unit(benchmark::kMicrosecond);

// =============================================================================
// Training Benchmarks
// =============================================================================

static void BM_LoRATraining(benchmark::State& state) {
    training::IncrementalTrainingConfig config;
    config.training_data_collection = "bench_samples";
    config.base_model_path = "/tmp/bench_model.gguf";
    config.batch_size = state.range(0);
    config.num_epochs = 1;
    
    training::IncrementalLoRATrainer trainer(config, "bench_db");
    
    for (auto _ : state) {
        auto result = trainer.train(training::TrainingMode::INITIAL, nullptr);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("batch_size=" + std::to_string(state.range(0)));
}
BENCHMARK(BM_LoRATraining)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Unit(benchmark::kSecond);

// =============================================================================
// End-to-End Pipeline Benchmarks
// =============================================================================

static void BM_FullPipeline(benchmark::State& state) {
    const size_t num_documents = state.range(0);
    
    for (auto _ : state) {
        // 1. Ingestion
        ingestion::IngestionManager mgr("bench_db");
        // TODO: Ingest num_documents documents
        
        // 2. Auto-labeling
        training::AutoLabelConfig label_config;
        training::LegalAutoLabeler labeler(label_config, "bench_db");
        // TODO: Label documents
        
        // 3. Enrichment
        training::EnrichmentConfig enrich_config;
        training::KnowledgeGraphEnricher enricher(enrich_config, "bench_db");
        // TODO: Enrich samples
        
        // 4. Training
        training::IncrementalTrainingConfig train_config;
        training::IncrementalLoRATrainer trainer(train_config, "bench_db");
        // TODO: Train adapter
    }
    
    state.SetLabel("docs=" + std::to_string(num_documents));
}
BENCHMARK(BM_FullPipeline)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kSecond);

// =============================================================================
// Performance Targets (from requirements)
// =============================================================================

// Target: >1000 docs/sec for HuggingFace ingestion
static void BM_TargetIngestionThroughput(benchmark::State& state) {
    // TODO: Verify meets >1000 docs/sec target
    BENCHMARK(BM_FileSystemIngestion);
}

// Target: >100 docs/sec for auto-labeling
static void BM_TargetLabelingThroughput(benchmark::State& state) {
    // TODO: Verify meets >100 docs/sec target
    BENCHMARK(BM_AutoLabeling);
}

// Target: <2 hours for 50k samples (single GPU)
static void BM_TargetTrainingTime(benchmark::State& state) {
    // TODO: Verify meets <2 hour target for 50k samples
    BENCHMARK(BM_LoRATraining);
}

// =============================================================================
// Main
// =============================================================================

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_LEGAL_TRAINING
