/**
 * @file bench_ingestion_extraction.cpp
 * @brief Extraction throughput and LLM-latency benchmarks for the ingestion module
 *
 * Validates:
 *   INGESTION-PERF: Extraction throughput + LLM-latency benchmark
 *
 * Scenarios:
 *   - DeonticExtractor: regex extraction throughput (single sentence)
 *   - DeonticExtractor: regex extraction on multi-sentence German legal text
 *   - DeonticExtractor: batch extraction (N sentences, measure scaling)
 *   - LegalLlmAdapter: buildExtractorFn() overhead (stub mode, no model)
 *   - LegalLlmAdapter: extracted function call latency (regex fallback)
 *   - FilesystemIngester: detectBinaryMimeType() throughput (magic-byte path)
 *   - IngestionCoordinator / InMemorySharedCheckpointStore: checkpoint save/load overhead
 *
 * Note: LLM path uses the regex fallback in Phase 1 (no model loaded).
 * When THEMIS_BENCH_LLM_MODEL_PATH is defined, real LLM latency is measured.
 */

#include <benchmark/benchmark.h>
#include "ingestion/deontic_extractor.h"
#include "ingestion/llm_adapter.h"
#include "ingestion/filesystem_ingester.h"
#include "ingestion/ingestion_coordinator.h"
#include "ingestion/ingestion_manager.h"

#include <cstdlib>
#include <string>
#include <vector>
#include <random>

using namespace themis::ingestion;

// ─── helpers ─────────────────────────────────────────────────────────────────

#define THEMIS_BENCH_STRINGIFY_INNER(x) #x
#define THEMIS_BENCH_STRINGIFY(x) THEMIS_BENCH_STRINGIFY_INNER(x)

static std::string resolveCompileTimeLlmModelPath() {
#ifdef THEMIS_BENCH_LLM_MODEL_PATH
    std::string value = THEMIS_BENCH_STRINGIFY(THEMIS_BENCH_LLM_MODEL_PATH);
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            value = value.substr(1, value.size() - 2);
        }
    }
    return value;
#else
    return {};
#endif
}

static std::string resolveLlmModelPath() {
    const char* runtime = std::getenv("THEMIS_BENCH_LLM_MODEL_PATH");
    if (runtime != nullptr && *runtime != '\0') {
        return runtime;
    }
    return resolveCompileTimeLlmModelPath();
}

// Sample German legal sentences covering all deontic categories.
static const std::vector<std::string> kLegalSentences = {
    "Der Antragsteller muss das Formular vollständig ausfüllen.",
    "Die Behörde darf die Unterlagen anfordern.",
    "Es ist verboten, ohne Genehmigung zu handeln.",
    "Im Sinne dieses Gesetzes gilt als Unternehmen jede juristische Person.",
    "Wenn die Voraussetzungen erfüllt sind, wird die Genehmigung erteilt.",
    "Ausgenommen sind Betriebe mit weniger als zehn Beschäftigten.",
    "Gemäß § 3 Absatz 2 sind folgende Unterlagen einzureichen.",
    "Der Bescheid ist innerhalb von 30 Tagen zu erlassen.",
    "Die Behörde kann nach pflichtgemäßem Ermessen entscheiden.",
    "Die Genehmigung darf nicht übertragen werden.",
};

static const std::string kLongLegalText = []() {
    std::string text;
    text.reserve(kLegalSentences.size() * 120);
    for (int i = 0; i < 10; ++i) {
        for (const auto& s : kLegalSentences) {
            text += s;
            text += " ";
        }
    }
    return text;
}();

// ─── DeonticExtractor fixtures ────────────────────────────────────────────────

class DeonticExtractionFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        extractor = std::make_unique<DeonticExtractor>();
    }

    void TearDown(const benchmark::State& /*s*/) override {
        extractor.reset();
    }

    std::unique_ptr<DeonticExtractor> extractor;
};

// ─── 1. Single sentence extraction throughput ────────────────────────────────

BENCHMARK_F(DeonticExtractionFixture, SingleSentence_Obligation)(benchmark::State& state) {
    const std::string sentence = kLegalSentences[0];

    for (auto _ : state) {
        auto result = extractor->extract(sentence);
        benchmark::DoNotOptimize(result.primaryCategory());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Single OBLIGATION sentence — regex path");
}

BENCHMARK_F(DeonticExtractionFixture, SingleSentence_AllCategories)(benchmark::State& state) {
    size_t idx = 0;

    for (auto _ : state) {
        const auto& sentence = kLegalSentences[idx % kLegalSentences.size()];
        auto result = extractor->extract(sentence);
        benchmark::DoNotOptimize(result.primaryCategory());
        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Single sentence — rotating through all 8 deontic categories");
}

// ─── 2. Batch extraction scaling ─────────────────────────────────────────────

BENCHMARK_F(DeonticExtractionFixture, BatchExtraction_Scaling)(benchmark::State& state) {
    const int kBatch = static_cast<int>(state.range(0));

    std::vector<std::string> sentences;
    sentences.reserve(static_cast<size_t>(kBatch));
    for (int i = 0; i < kBatch; ++i) {
        sentences.push_back(kLegalSentences[i % kLegalSentences.size()]);
    }

    for (auto _ : state) {
        std::vector<DeonticExtraction> results;
        results.reserve(sentences.size());
        for (const auto& s : sentences) {
            results.push_back(extractor->extract(s));
        }
        benchmark::DoNotOptimize(results.size());
    }

    state.SetItemsProcessed(state.iterations() * kBatch);
    state.SetLabel("Batch extraction — " + std::to_string(kBatch) + " sentences");
}
BENCHMARK_REGISTER_F(DeonticExtractionFixture, BatchExtraction_Scaling)
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ─── 3. Long text extraction ──────────────────────────────────────────────────

BENCHMARK_F(DeonticExtractionFixture, LongText_MultiParagraph)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = extractor->extract(kLongLegalText);
        benchmark::DoNotOptimize(result.primaryCategory());
    }

    state.SetLabel("Long text: " + std::to_string(kLongLegalText.size()) + " bytes");
}

// ─── 4. extractEntities() over full document ─────────────────────────────────

BENCHMARK_F(DeonticExtractionFixture, ExtractEntities_FullDocument)(benchmark::State& state) {
    for (auto _ : state) {
        auto entities = extractor->extractEntities(kLongLegalText);
        benchmark::DoNotOptimize(entities.size());
    }

    state.SetLabel("extractEntities() on 1 000-byte legal document");
}

// ─── LegalLlmAdapter fixtures ─────────────────────────────────────────────────

class LlmAdapterFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        adapter = std::make_unique<LegalLlmAdapter>();
        LlmAdapterConfig cfg;
        cfg.model_path = resolveLlmModelPath();  // empty → regex fallback
        adapter->setConfig(cfg);
    }

    void TearDown(const benchmark::State& /*s*/) override {
        adapter.reset();
    }

    std::unique_ptr<LegalLlmAdapter> adapter;
};

// ─── 5. buildExtractorFn() overhead ──────────────────────────────────────────

BENCHMARK_F(LlmAdapterFixture, BuildExtractorFn)(benchmark::State& state) {
    for (auto _ : state) {
        auto fn = adapter->buildExtractorFn();
        // fn may be empty in Phase 1 (no model); bool conversion checks validity
        bool valid = static_cast<bool>(fn);
        benchmark::DoNotOptimize(valid);
    }
    state.SetLabel("buildExtractorFn() — returns ExtractorFn (may be empty in Phase 1)");
}

// ─── 6. buildExtractor() convenience factory ─────────────────────────────────

BENCHMARK_F(LlmAdapterFixture, BuildExtractor_Factory)(benchmark::State& state) {
    for (auto _ : state) {
        auto extractor = adapter->buildExtractor(0.75);
        benchmark::DoNotOptimize(extractor.getConfidenceThreshold());
    }
    state.SetLabel("buildExtractor() convenience factory call");
}

// ─── 7. Extractor throughput via adapter-configured extractor ─────────────────

BENCHMARK_F(LlmAdapterFixture, ExtractorFn_Throughput)(benchmark::State& state) {
    // Build a DeonticExtractor configured with this adapter's fn (Phase 1 = regex)
    auto extractor = adapter->buildExtractor(0.75);
    const int kN = static_cast<int>(state.range(0));

    for (auto _ : state) {
        uint64_t matched = 0;
        for (int i = 0; i < kN; ++i) {
            auto result = extractor.extract(kLegalSentences[i % kLegalSentences.size()]);
            if (result.hasCategory()) {
              ++matched;
            }
        }
        benchmark::DoNotOptimize(matched);
    }

    state.SetItemsProcessed(state.iterations() * kN);
    state.SetLabel("Extractor throughput — " + std::to_string(kN) + " calls");
}
BENCHMARK_REGISTER_F(LlmAdapterFixture, ExtractorFn_Throughput)
    ->Arg(100)->Arg(500)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ─── 8. detectBinaryMimeType() throughput ────────────────────────────────────

static void BM_DetectBinaryMimeType(benchmark::State& state) {
    // PDF magic bytes → first 8 chars suffice for detection
    const std::string pdf_header  = "%PDF-1.4 some content follows here";
    // DOCX magic bytes (ZIP PK header)
    const std::string docx_header = std::string(2, '\x50') + '\x4B' + '\x03' + '\x04'
                                  + " rest of ZIP content";
    // Plain text
    const std::string text_header = "Hello World, this is plain text content";

    size_t idx = 0;
    const std::vector<const std::string*> headers = {
        &pdf_header, &docx_header, &text_header
    };

    for (auto _ : state) {
        auto type = detectBinaryMimeType(*headers[idx % headers.size()]);
        benchmark::DoNotOptimize(type);
        ++idx;
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("detectBinaryMimeType() magic-byte inspection");
}
BENCHMARK(BM_DetectBinaryMimeType);

// ─── 9. InMemorySharedCheckpointStore overhead ───────────────────────────────

static void BM_CheckpointStore(benchmark::State& state) {
    const int kWorkers = static_cast<int>(state.range(0));
    InMemorySharedCheckpointStore store;

    for (auto _ : state) {
        for (int w = 0; w < kWorkers; ++w) {
            IngestionCheckpoint cp;
            cp.source_id        = "worker_" + std::to_string(w);
            cp.processed_count  = static_cast<size_t>(w * 1000);
            cp.byte_offset      = static_cast<size_t>(w * 512);

            bool written = store.write(cp);
            benchmark::DoNotOptimize(written);

            IngestionCheckpoint out;
            bool read = store.read(cp.source_id, out);
            benchmark::DoNotOptimize(read);
        }
    }

    state.SetItemsProcessed(state.iterations() * kWorkers * 2LL);
    state.SetLabel("Checkpoint write+read × " + std::to_string(kWorkers) + " workers");
}
BENCHMARK(BM_CheckpointStore)->Arg(1)->Arg(4)->Arg(16)->Arg(64);
