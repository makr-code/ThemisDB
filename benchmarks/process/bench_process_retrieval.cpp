/*
 * ThemisDB — Process Retrieval Benchmark
 *
 * File:    bench_process_retrieval.cpp
 * Module:  benchmarks/
 * Purpose: Self-contained microbenchmarks for the process module embedding
 *          pipeline, HNSW-style top-k vector retrieval, BM25 full-text search,
 *          and state-change re-embedding hook.
 *
 * Validates:
 *   PROC-OPEN-01 — Auto-generate process model embeddings on import (local
 *                  deterministic path; LLM endpoint integration is external)
 *   PROC-OPEN-02 — Auto-generate process instance embeddings after state change
 *   PROC-OPEN-03 — Full-text inverted index over process model descriptions
 *
 * No Themis runtime headers required — purely standalone microbenchmarks.
 */

#include <benchmark/benchmark.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

constexpr int kEmbeddingDim = 256;
constexpr int kDefaultTopK  = 5;

// ─────────────────────────────────────────────────────────────────────────────
// generateEmbedding — deterministic 256-dim L2-normalised embedding from text
//
// Simulates the auto-embedding hook called by ProcessModelManager::save() when
// a local embedding back-end is configured (PROC-OPEN-01/02).  Uses an
// FNV-1a-seeded Mersenne Twister so results are fully reproducible.
// ─────────────────────────────────────────────────────────────────────────────

static std::vector<float> generateEmbedding(const std::string& text) {
    std::vector<float> emb(kEmbeddingDim, 0.0f);

    // FNV-1a hash over the input bytes → seed for the per-document PRNG
    uint64_t h = UINT64_C(0xcbf29ce484222325);
    for (unsigned char c : text) {
        h ^= c;
        h *= UINT64_C(0x100000001b3);
    }

    std::mt19937_64 prng(h);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto& v : emb) {
        v = dist(prng);
    }

    // L2-normalise so cosine similarity equals dot product
    float norm = 0.0f;
    for (float v : emb) { norm += v * v; }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (auto& v : emb) { v /= norm; }
    }

    return emb;
}

// ─────────────────────────────────────────────────────────────────────────────
// SimProcessModel — mirrors the embedding-relevant fields of ProcessModelRecord
// ─────────────────────────────────────────────────────────────────────────────

struct SimProcessModel {
    std::string id;
    std::string name;
    std::string description;
    std::string state;          // "DRAFT" | "ACTIVE" | "DEPRECATED"
    std::vector<float> embedding;
    int revision{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// buildModelStore — synthesise N process model records with pre-computed embeds
// ─────────────────────────────────────────────────────────────────────────────

static std::vector<SimProcessModel> buildModelStore(int n) {
    static const char* const kDomainWords[] = {
        "Bauantrag", "Beschaffung", "Personal", "Haushalt", "Genehmigung",
        "Pruefung", "Antrag", "Dokumentenfreigabe", "Einspruch", "Ausschreibung"
    };
    static const int kDomainCount = 10;

    static const char* const kStates[] = { "DRAFT", "ACTIVE", "DEPRECATED" };

    std::vector<SimProcessModel> store;
    store.reserve(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i) {
        SimProcessModel m;
        m.id = "proc_model_" + std::to_string(i);
        m.name = std::string(kDomainWords[i % kDomainCount]) +
                 " v" + std::to_string(i / kDomainCount + 1);
        m.description =
            "Verwaltungsvorgang fuer " + m.name +
            ". Dieser Prozess umfasst die Bearbeitung von Antraegen und Genehmigungen"
            " im Bereich " +
            std::string(kDomainWords[(i + 3) % kDomainCount]) +
            ". Version " + std::to_string(i) +
            " mit erweitertem Compliance-Profil und DSGVO-Konformitaet.";
        m.state    = kStates[i % 3];
        m.revision = i % 5 + 1;
        m.embedding = generateEmbedding(m.description);
        store.push_back(std::move(m));
    }

    return store;
}

// ─────────────────────────────────────────────────────────────────────────────
// makeBpmnXml — minimal BPMN 2.0 fragment for import simulation
// ─────────────────────────────────────────────────────────────────────────────

static std::string makeBpmnXml(int model_idx, int node_count) {
    std::ostringstream oss = {};
    oss << R"(<?xml version="1.0" encoding="UTF-8"?>)"
        << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">)"
        << "<process id=\"proc_" << model_idx << "\" name=\"Verwaltungsvorgang "
        << model_idx << "\">"
        << "<startEvent id=\"start_" << model_idx << "\" name=\"Eingang\"/>";

    for (int i = 0; i < node_count; ++i) {
        oss << "<userTask id=\"task_" << model_idx << "_" << i
            << "\" name=\"Schritt " << i << "\"/>"
            << "<sequenceFlow id=\"sf_" << model_idx << "_" << i
            << "\" sourceRef=\""
            << (i == 0 ? "start_" + std::to_string(model_idx)
                       : "task_" + std::to_string(model_idx) + "_" + std::to_string(i - 1))
            << "\" targetRef=\"task_" << model_idx << "_" << i << "\"/>";
    }

    oss << "<endEvent id=\"end_" << model_idx << "\" name=\"Bescheid\"/>"
        << "<sequenceFlow id=\"sf_end_" << model_idx
        << "\" sourceRef=\"task_" << model_idx << "_" << (node_count - 1)
        << "\" targetRef=\"end_" << model_idx << "\"/>"
        << "</process></definitions>";

    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// bm25Score — simplified BM25 term-frequency score (PROC-OPEN-03)
//
// Simulates the keyword scoring used by ProcessModelManager::search().
// ─────────────────────────────────────────────────────────────────────────────

static float bm25Score(const std::string& doc,
                       const std::string& query,
                       float              avg_dl,
                       float              k1 = 1.5f,
                       float              b  = 0.75f)
{
    size_t pos = 0;
    int tf = 0;
    while ((pos = doc.find(query, pos)) != std::string::npos) {
        ++tf;
        ++pos;
    }
    if (tf == 0) {
      return 0.0f;
    }

    const float dl  = static_cast<float>(doc.size());
    const float idf = std::log(1.0f + 100.0f / (1.0f + static_cast<float>(tf)));
    return idf * (static_cast<float>(tf) * (k1 + 1.0f)) /
           (static_cast<float>(tf) + k1 * (1.0f - b + b * dl / avg_dl));
}

// ─────────────────────────────────────────────────────────────────────────────
// hnswScan — brute-force cosine top-k scan (HNSW baseline path)
//
// Simulates ProcessModelManager::findSimilar() on a corpus of pre-embedded
// models.  Vectors are already L2-normalised so cosine = dot product.
// ─────────────────────────────────────────────────────────────────────────────

static std::vector<std::pair<float, std::string>>
hnswScan(const std::vector<SimProcessModel>& store,
         const std::vector<float>&           query_emb,
         int                                 k)
{
    std::vector<std::pair<float, std::string>> scores;
    scores.reserve(store.size());

    for (const auto& m : store) {
        float dot = 0.0f;
        for (int d = 0; d < kEmbeddingDim; ++d) {
            dot += query_emb[static_cast<size_t>(d)] *
                   m.embedding[static_cast<size_t>(d)];
        }
        scores.emplace_back(dot, m.id);
    }

    const int top = std::min(k, static_cast<int>(scores.size()));
    std::partial_sort(scores.begin(),
                      scores.begin() + top,
                      scores.end(),
                      [](const auto& a, const auto& b) {
                          return a.first > b.first;
                      });
    scores.resize(static_cast<size_t>(top));
    return scores;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// BM_ProcessModelImport
//
// Simulates ProcessModelManager::importBpmn() — BPMN XML tokenisation and
// graph normalization.  Arg: number of BPMN task nodes per model (10/50/200).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ProcessModelImport(benchmark::State& state) {
    const int node_count = static_cast<int>(state.range(0));
    int model_idx = 0;

    for (auto _ : state) {
        const std::string xml = makeBpmnXml(model_idx++, node_count);

        // Simulate token counts (analogous to BpmnSerializer::importXml)
        size_t nodes = 0;
        size_t edges = 0;
        size_t pos   = 0;
        while ((pos = xml.find("Task id=", pos)) != std::string::npos) { ++nodes; ++pos; }
        pos = 0;
        while ((pos = xml.find("Event id=", pos)) != std::string::npos) { ++nodes; ++pos; }
        pos = 0;
        while ((pos = xml.find("sequenceFlow id=", pos)) != std::string::npos) { ++edges; ++pos; }

        benchmark::DoNotOptimize(nodes);
        benchmark::DoNotOptimize(edges);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("nodes=" + std::to_string(node_count));
}

BENCHMARK(BM_ProcessModelImport)
    ->Arg(10)
    ->Arg(50)
    ->Arg(200)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_ProcessEmbeddingGenerate
//
// Deterministic 256-dim embedding generation from description text.
// Simulates the auto-embed hook invoked by ProcessModelManager::save() on
// import and state transitions (PROC-OPEN-01).
// Arg: description length in bytes (256 / 1024 / 4096).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ProcessEmbeddingGenerate(benchmark::State& state) {
    const int text_len = static_cast<int>(state.range(0));

    // Fill description buffer with a realistic cycling ASCII phrase
    const std::string pattern =
        "Verwaltungsvorgang Bauantrag Genehmigung Pruefung Antrag Bescheid ";
    std::string description(static_cast<size_t>(text_len), ' ');
    const size_t plen = pattern.size();
    for (int i = 0; i < text_len; ++i) {
        description[static_cast<size_t>(i)] = pattern[static_cast<size_t>(i) % plen];
    }

    for (auto _ : state) {
        auto emb = generateEmbedding(description);
        benchmark::DoNotOptimize(emb.data());
    }

    state.SetBytesProcessed(state.iterations() * text_len);
    state.SetLabel("text_len=" + std::to_string(text_len));
}

BENCHMARK(BM_ProcessEmbeddingGenerate)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_ProcessEmbeddingPersist
//
// Serialises a ProcessModelRecord including the embedding vector to a JSON
// byte representation.  Simulates ProcessModelManager::save() with the
// _embedding field populated (toDocument() path).
// Arg: embedding dimensions (64 / 256 / 1024).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ProcessEmbeddingPersist(benchmark::State& state) {
    const int dims = static_cast<int>(state.range(0));

    SimProcessModel model;
    model.id          = "proc_model_persist_bench";
    model.name        = "Bauantragsverfahren Standard";
    model.description = "Genehmigungsverfahren fuer Bauantraege gemaess Bauordnung.";
    model.state       = "ACTIVE";
    model.revision    = 3;
    model.embedding.resize(static_cast<size_t>(dims));

    std::mt19937 prng(0xABCD1234u);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto& v : model.embedding) { v = dist(prng); }

    for (auto _ : state) {
        std::ostringstream oss = {};
        oss << "{\"id\":\"" << model.id
            << "\",\"name\":\"" << model.name
            << "\",\"state\":\"" << model.state
            << "\",\"revision\":" << model.revision
            << ",\"_embedding\":[";

        for (int i = 0; i < dims; ++i) {
            if (i > 0) {
              oss << ',';
            }
            oss << model.embedding[static_cast<size_t>(i)];
        }
        oss << "]}";

        auto result = oss.str();
        benchmark::DoNotOptimize(result.data());
    }

    state.SetBytesProcessed(state.iterations() * dims * static_cast<int>(sizeof(float)));
    state.SetLabel("dims=" + std::to_string(dims));
}

BENCHMARK(BM_ProcessEmbeddingPersist)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_ProcessFullTextSearch
//
// BM25 keyword search over a corpus of model descriptions.
// Simulates ProcessModelManager::search() (PROC-OPEN-03).
// Arg: store size (10 / 100 / 1000 models).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ProcessFullTextSearch(benchmark::State& state) {
    const int store_size = static_cast<int>(state.range(0));
    const auto store = buildModelStore(store_size);

    float avg_dl = 0.0f;
    for (const auto& m : store) {
        avg_dl += static_cast<float>(m.description.size());
    }
    avg_dl /= static_cast<float>(store.size());

    const std::string query = "Bauantrag";

    for (auto _ : state) {
        std::vector<std::pair<float, std::string>> results;
        results.reserve(static_cast<size_t>(store_size));

        for (const auto& m : store) {
            float score = bm25Score(m.description, query, avg_dl);
            if (score > 0.0f) {
                results.emplace_back(score, m.id);
            }
        }

        std::sort(results.begin(), results.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        benchmark::DoNotOptimize(results.data());
    }

    state.SetItemsProcessed(state.iterations() * store_size);
    state.SetLabel("store=" + std::to_string(store_size));
}

BENCHMARK(BM_ProcessFullTextSearch)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_ProcessHnswRetrieve
//
// Top-k cosine similarity scan over pre-embedded process models.
// Simulates ProcessModelManager::findSimilar() (HNSW baseline path).
// Arg: store size (10 / 100 / 1000 models).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ProcessHnswRetrieve(benchmark::State& state) {
    const int store_size = static_cast<int>(state.range(0));
    const auto store     = buildModelStore(store_size);
    const auto query_emb = generateEmbedding("Bauantragsverfahren Genehmigung DSGVO");

    for (auto _ : state) {
        auto top = hnswScan(store, query_emb, kDefaultTopK);
        benchmark::DoNotOptimize(top.data());
    }

    state.SetItemsProcessed(state.iterations() * store_size);
    state.SetLabel("store=" + std::to_string(store_size) +
                   " k=" + std::to_string(kDefaultTopK));
}

BENCHMARK(BM_ProcessHnswRetrieve)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// BM_ProcessStateChangeEmbed
//
// Re-generates embedding for each process instance state transition.
// Simulates the auto-embed hook triggered by ProcessGraphManager state changes
// (PROC-OPEN-02: "Auto-generate process instance embeddings after state change").
// Arg: number of state changes to process in batch (10 / 100 / 500).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_ProcessStateChangeEmbed(benchmark::State& state) {
    const int change_count = static_cast<int>(state.range(0));

    static const char* const kTargetStates[] = {
        "RUNNING", "SUSPENDED", "COMPLETED", "TERMINATED", "FAILED"
    };

    for (auto _ : state) {
        for (int i = 0; i < change_count; ++i) {
            const std::string text =
                "Instance proc_inst_" + std::to_string(i) +
                " transitioned to state " +
                std::string(kTargetStates[i % 5]) +
                ". Active nodes: [node_" + std::to_string(i) +
                "]. Timestamp: " + std::to_string(INT64_C(1700000000) + i);

            auto emb = generateEmbedding(text);
            benchmark::DoNotOptimize(emb.data());
        }
    }

    state.SetItemsProcessed(state.iterations() * change_count);
    state.SetLabel("changes=" + std::to_string(change_count));
}

BENCHMARK(BM_ProcessStateChangeEmbed)
    ->Arg(10)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMicrosecond);
