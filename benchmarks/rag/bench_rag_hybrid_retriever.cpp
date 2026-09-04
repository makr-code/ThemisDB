/**
 * @file bench_rag_hybrid_retriever.cpp
 * @brief Google Benchmark suite for HybridRetriever (RAG Phase 3)
 *
 * Benchmarks the algorithmic cost of RRF and linear-combination fusion
 * at varying candidate pool sizes.  All benchmarks are in-process (no
 * database I/O) to isolate fusion overhead.
 *
 * Performance targets:
 *   - RRF mode:    < 1 ms for 100 candidates
 *   - Linear mode: < 1 ms for 100 candidates
 *
 * Build:
 *   cmake --build . --target bench_rag_hybrid_retriever --config Release
 * Run:
 *   ./benchmarks/bench_rag_hybrid_retriever [--benchmark_filter=...]
 */

#include <benchmark/benchmark.h>
#include "rag/hybrid_retriever.h"
#include "rag/dpr_vectorizer.h"
#include "rag/fairness_detector.h"
#include "rag/vectorizer_interface.h"
#include <exception>
#include <random>
#include <string>
#include <vector>

using namespace themis::rag;
using namespace themis::rag::judge;

// ============================================================================
// Helper: build a candidate list of `n` documents
// ============================================================================

static std::vector<RetrievedDocument>
makeCandidates(int n, double base_score, std::mt19937& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::vector<RetrievedDocument> list;
    list.reserve(n);
    for (int i = 0; i < n; ++i) {
        RetrievedDocument d;
        d.id               = "doc" + std::to_string(i);
        d.content          = "Candidate document number " + std::to_string(i);
        d.similarity_score = base_score - static_cast<double>(i) * 0.001 + dist(rng) * 0.0001;
        list.push_back(d);
    }
    return list;
}

class DeterministicBenchmarkVectorizer final : public IVectorizer {
public:
    void initialize() override { initialized_ = true; }

    bool isInitialized() const override { return initialized_; }

    std::vector<float> encodeQuery(const std::string& query) override {
        return encodeText(query);
    }

    std::vector<float> encodePassage(const std::string& passage) override {
        return encodeText(passage);
    }

    size_t getEmbeddingDimension() const override { return kDim; }

private:
    static constexpr size_t kDim = 64;
    bool initialized_ = false;

    static std::vector<float> encodeText(const std::string& text) {
        std::vector<float> embedding(kDim, 0.0f);
        if (text.empty()) {
            return embedding;
        }

        for (size_t i = 0; i < text.size(); ++i) {
            const size_t idx = i % kDim;
            embedding[idx] += static_cast<float>(static_cast<unsigned char>(text[i])) / 255.0f;
        }
        return embedding;
    }
};

// ============================================================================
// RRF benchmarks: varying candidate pool size
// ============================================================================

static void BM_RRF_Balanced(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, 0.9, rng);
    auto vec  = makeCandidates(n, 0.8, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 0.5;
    cfg.vector_weight = 0.5;
    cfg.use_rrf       = true;
    cfg.top_k         = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, vec);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
    state.SetLabel(std::to_string(n) + " candidates each");
}
BENCHMARK(BM_RRF_Balanced)->Arg(10)->Arg(50)->Arg(100)->Arg(500)->Arg(1000);

static void BM_RRF_BM25Only(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, 0.9, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 1.0;
    cfg.vector_weight = 0.0;
    cfg.use_rrf       = true;
    cfg.top_k         = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, {});
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BM25-only, " + std::to_string(n) + " candidates");
}
BENCHMARK(BM_RRF_BM25Only)->Arg(10)->Arg(100)->Arg(1000);

static void BM_RRF_VectorOnly(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto vec = makeCandidates(n, 0.9, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 0.0;
    cfg.vector_weight = 1.0;
    cfg.use_rrf       = true;
    cfg.top_k         = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse({}, vec);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("vector-only, " + std::to_string(n) + " candidates");
}
BENCHMARK(BM_RRF_VectorOnly)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// RRF – disjoint candidate lists (worst case: all unique IDs, max map size)
// ============================================================================

static void BM_RRF_Disjoint(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    // Disjoint: BM25 uses doc0..doc(n-1), vector uses docB0..docB(n-1)
    auto bm25 = makeCandidates(n, 0.9, rng);
    std::vector<RetrievedDocument> vec = makeCandidates(n, 0.8, rng);
    for (auto& d : vec) { d.id = "b_" + d.id; }

    HybridRetrieverConfig cfg;
    cfg.use_rrf = true;
    cfg.top_k   = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, vec);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
    state.SetLabel("disjoint, " + std::to_string(n) + " each");
}
BENCHMARK(BM_RRF_Disjoint)->Arg(10)->Arg(100)->Arg(500);

// ============================================================================
// Linear combination benchmarks
// ============================================================================

static void BM_Linear_Balanced(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, 0.9, rng);
    auto vec  = makeCandidates(n, 0.8, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight      = 0.5;
    cfg.vector_weight    = 0.5;
    cfg.use_rrf          = false;
    cfg.normalize_scores = true;
    cfg.top_k            = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, vec);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
    state.SetLabel(std::to_string(n) + " candidates each");
}
BENCHMARK(BM_Linear_Balanced)->Arg(10)->Arg(50)->Arg(100)->Arg(500);

// ============================================================================
// Config construction overhead
// ============================================================================

static void BM_ConfigConstruction(benchmark::State& state) {
    for (auto _ : state) {
        HybridRetrieverConfig cfg;
        cfg.bm25_weight   = 0.4;
        cfg.vector_weight = 0.6;
        cfg.rrf_k         = 60.0;
        cfg.top_k         = 10;
        HybridRetriever retriever(cfg);
        benchmark::DoNotOptimize(retriever);
    }
}
BENCHMARK(BM_ConfigConstruction);

// ============================================================================
// Factory helpers overhead
// ============================================================================

static void BM_FactoryCreateBalanced(benchmark::State& state) {
    for (auto _ : state) {
        auto r = HybridRetrieverFactory::createBalanced(10);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_FactoryCreateBalanced);

// ============================================================================
// Wave A2: DPR Vectorizer Benchmarks
// ============================================================================

static void BM_DPRVectorizer_QueryEncoding(benchmark::State& state) {
    // Phase 2: Benchmark query encoding latency
    // Target: ≤ 150 ms per query on GPU
    
    themis::rag::DPRVectorizerConfig cfg;
    cfg.query_model_path = "./models/dpr/query_encoder.onnx";
    cfg.passage_model_path = "./models/dpr/passage_encoder.onnx";
    cfg.device = "cpu";  // Use CPU for benchmarking
    cfg.embedding_dimension = 384;
    
    themis::rag::DPRVectorizer vectorizer(cfg);
    
    // Skip initialization if models not available
    try {
        vectorizer.initialize();
    } catch (const std::exception& e) {
        state.SkipWithMessage(std::string("DPR models not available: ") + e.what());
        return;
    }
    
    const std::string query = "What is machine learning?";
    
    for (auto _ : state) {
        auto embedding = vectorizer.encodeQuery(query);
        benchmark::DoNotOptimize(embedding);
    }
    
    state.SetLabel("Query encoding (384-dim)");
}
BENCHMARK(BM_DPRVectorizer_QueryEncoding);

static void BM_DPRVectorizer_PassageBatch(benchmark::State& state) {
    // Phase 2: Benchmark batch passage encoding throughput
    // Target: ≥ 100 docs/sec for batch_size=32
    
    const int batch_size = static_cast<int>(state.range(0));
    
    themis::rag::DPRVectorizerConfig cfg;
    cfg.query_model_path = "./models/dpr/query_encoder.onnx";
    cfg.passage_model_path = "./models/dpr/passage_encoder.onnx";
    cfg.device = "cpu";
    cfg.batch_size = batch_size;
    cfg.embedding_dimension = 384;
    
    themis::rag::DPRVectorizer vectorizer(cfg);
    
    try {
        vectorizer.initialize();
    } catch (const std::exception& e) {
        state.SkipWithMessage(std::string("DPR models not available: ") + e.what());
        return;
    }
    
    // Create batch of passages
    std::vector<std::string> passages = {};

    for (int i = 0; i < batch_size; ++i) {
        passages.push_back("This is a sample passage number " + std::to_string(i) + 
                          " for benchmarking the DPR vectorizer batch encoding.");
    }
    
    for (auto _ : state) {
        auto embeddings = vectorizer.encodePassageBatch(passages);
        benchmark::DoNotOptimize(embeddings);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetLabel("Batch size: " + std::to_string(batch_size));
}
BENCHMARK(BM_DPRVectorizer_PassageBatch)->Arg(8)->Arg(16)->Arg(32)->Arg(64);

static void BM_HybridRetriever_BM25Baseline(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, 0.9, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight = 1.0;
    cfg.vector_weight = 0.0;
    cfg.use_rrf = true;
    cfg.top_k = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, {});
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BM25-only baseline");
}
BENCHMARK(BM_HybridRetriever_BM25Baseline)->Arg(10)->Arg(50)->Arg(100)->Arg(500);

static void BM_HybridRetriever_VectorizerPath(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, 0.9, rng);

    auto vectorizer = std::make_shared<DeterministicBenchmarkVectorizer>();
    vectorizer->initialize();

    HybridRetrieverConfig cfg;
    cfg.bm25_weight = 0.3;
    cfg.vector_weight = 0.7;
    cfg.use_rrf = true;
    cfg.top_k = 10;
    HybridRetriever retriever(cfg);
    retriever.setVectorizer(vectorizer);

    for (auto _ : state) {
        auto result = retriever.retrieveWithVectorizer("hybrid benchmark query", bm25);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("DPR-like vectorizer path vs BM25 baseline");
}
BENCHMARK(BM_HybridRetriever_VectorizerPath)->Arg(10)->Arg(50)->Arg(100)->Arg(500);

// ============================================================================
// Wave A3: Fairness Detector Benchmarks
// ============================================================================

static void BM_FairnessDetector_BiasDetection(benchmark::State& state) {
    // Phase 2: Benchmark bias detection latency
    // Target: ≤ 5 ms per document
    
    themis::rag::FairnessDetectorConfig cfg;
    cfg.embedding_model_path = "./models/embeddings/glove.6B.300d.txt";
    cfg.bias_threshold = 0.6;
    cfg.detect_gender_bias = true;
    cfg.detect_occupational_bias = true;
    cfg.detect_ethnicity_bias = true;
    
    themis::rag::FairnessDetector detector(cfg);
    
    try {
        detector.initialize();
    } catch (const std::exception& e) {
        state.SkipWithMessage(std::string("Embedding models not available: ") + e.what());
        return;
    }
    
    const std::string document = 
        "The nurse and the doctor discussed the patient's condition. "
        "She was very professional and he was experienced. "
        "The team worked together to provide the best care.";
    
    for (auto _ : state) {
        auto bias_score = detector.detectBias(document);
        benchmark::DoNotOptimize(bias_score);
    }
    
    state.SetLabel("Bias detection per document");
}
BENCHMARK(BM_FairnessDetector_BiasDetection);

static void BM_FairnessDetector_BatchDetection(benchmark::State& state) {
    // Phase 2: Benchmark batch bias detection throughput
    
    const int batch_size = static_cast<int>(state.range(0));
    
    themis::rag::FairnessDetectorConfig cfg;
    cfg.embedding_model_path = "./models/embeddings/glove.6B.300d.txt";
    cfg.bias_threshold = 0.6;
    
    themis::rag::FairnessDetector detector(cfg);
    
    try {
        detector.initialize();
    } catch (const std::exception& e) {
        state.SkipWithMessage(std::string("Embedding models not available: ") + e.what());
        return;
    }
    
    // Create batch of documents
    std::vector<std::string> documents = {};

    for (int i = 0; i < batch_size; ++i) {
        documents.push_back(
            "The nurse and the doctor discussed the patient's condition. "
            "She was very professional and he was experienced. "
            "The team worked together to provide the best care. "
            "Document number " + std::to_string(i));
    }
    
    for (auto _ : state) {
        auto bias_scores = detector.detectBiasBatch(documents);
        benchmark::DoNotOptimize(bias_scores);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetLabel("Batch size: " + std::to_string(batch_size));
}
BENCHMARK(BM_FairnessDetector_BatchDetection)->Arg(10)->Arg(50)->Arg(100)->Arg(500);

BENCHMARK_MAIN();
