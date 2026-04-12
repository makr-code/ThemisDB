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

#include <string>
#include <vector>
#include <random>

using namespace themis::ingestion;

// ─── helpers ─────────────────────────────────────────────────────────────────

static const char* kLlmModel =
#ifdef THEMIS_BENCH_LLM_MODEL_PATH
    THEMIS_BENCH_LLM_MODEL_PATH;
#else
    "";  // empty → regex fallback
#endif

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
        benchmark::DoNotOptimize(result.category);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Single OBLIGATION sentence — regex path");
}

BENCHMARK_F(DeonticExtractionFixture, SingleSentence_AllCategories)(benchmark::State& state) {
    size_t idx = 0;

    for (auto _ : state) {
        const auto& sentence = kLegalSentences[idx % kLegalSentences.size()];
        auto result = extractor->extract(sentence);
        benchmark::DoNotOptimize(result.category);
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
        std::vector<ExtractionResult> results;
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
        benchmark::DoNotOptimize(result.category);
    }

    state.SetLabel("Long text: " + std::to_string(kLongLegalText.size()) + " bytes");
}

// ─── 4. extractAll() over multiple patterns ──────────────────────────────────

BENCHMARK_F(DeonticExtractionFixture, ExtractAll_FullDocument)(benchmark::State& state) {
    for (auto _ : state) {
        auto results = extractor->extractAll(kLongLegalText);
        benchmark::DoNotOptimize(results.size());
    }

    state.SetLabel("extractAll() on 1 000-byte legal document");
}

// ─── LegalLlmAdapter fixtures ─────────────────────────────────────────────────

class LlmAdapterFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        LlmAdapterConfig cfg;
        cfg.model_path = kLlmModel;  // empty → regex fallback
        adapter = std::make_unique<LegalLlmAdapter>(cfg);
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
        benchmark::DoNotOptimize(fn != nullptr);
    }
    state.SetLabel("buildExtractorFn() — returns std::function wrapping regex/LLM path");
}

// ─── 6. Extractor function call latency (regex fallback) ─────────────────────

BENCHMARK_F(LlmAdapterFixture, ExtractorFnCall_Latency)(benchmark::State& state) {
    auto fn = adapter->buildExtractorFn();
    const std::string sentence = kLegalSentences[0];

    for (auto _ : state) {
        auto result = fn(sentence);
        benchmark::DoNotOptimize(result.category);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Extractor fn call — regex path (no model)");
}

// ─── 7. Extractor throughput: 1000 sentences via LLM adapter ─────────────────

BENCHMARK_F(LlmAdapterFixture, ExtractorFn_Throughput)(benchmark::State& state) {
    auto fn = adapter->buildExtractorFn();
    const int kN = static_cast<int>(state.range(0));

    for (auto _ : state) {
        uint64_t matched = 0;
        for (int i = 0; i < kN; ++i) {
            auto result = fn(kLegalSentences[i % kLegalSentences.size()]);
            if (result.category != DeonticCategory::UNKNOWN) ++matched;
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
    // PDF magic bytes
    const std::vector<uint8_t> pdf_header = {0x25, 0x50, 0x44, 0x46, 0x2D, 0x31, 0x2E, 0x34};
    // DOCX magic bytes (ZIP PK header)
    const std::vector<uint8_t> docx_header = {0x50, 0x4B, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00};
    // Plain text
    const std::vector<uint8_t> text_header = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o'};

    size_t idx = 0;
    const std::vector<const std::vector<uint8_t>*> headers = {
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
            const std::string key = "worker_" + std::to_string(w);
            store.save(key, "offset_" + std::to_string(w * 1000));
            auto val = store.load(key);
            benchmark::DoNotOptimize(val);
        }
    }

    state.SetItemsProcessed(state.iterations() * kWorkers * 2LL);
    state.SetLabel("Checkpoint save+load × " + std::to_string(kWorkers) + " workers");
}
BENCHMARK(BM_CheckpointStore)->Arg(1)->Arg(4)->Arg(16)->Arg(64);
