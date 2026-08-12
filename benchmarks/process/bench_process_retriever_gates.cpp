#include <benchmark/benchmark.h>
#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include <algorithm>
#include <random>
#include <thread>
#include <cmath>

namespace themis::process::benchmark {

// ============================================================================
// Constants
// ============================================================================

constexpr uint64_t kCanonicalRngSeed = 42;
constexpr int kSmallDatasetSize = 100;
constexpr int kMediumDatasetSize = 1000;
constexpr int kLargeDatasetSize = 10000;
constexpr int kEmbeddingDim = 128;

// ============================================================================
// Data Types
// ============================================================================

/**
 * @brief Simulated process model record
 */
struct ProcessModelRecord {
    std::string id;
    std::string name;
    std::string description;
    std::string state;  // DRAFT, ACTIVE, DEPRECATED
    std::vector<float> embedding;
    int revision{0};
    int64_t created_ms{0};
    double relevance_score{0.0};
};

/**
 * @brief Query result
 */
struct QueryResult {
    std::vector<ProcessModelRecord> matches;
    int total_count{0};
    int64_t query_time_ms{0};
};

/**
 * @brief In-memory process model store with retrieval capabilities
 */
class ProcessModelRetriever {
private:
    std::vector<ProcessModelRecord> models_;
    int64_t last_modified_ms_{0};

public:
    ProcessModelRetriever() = default;

    /**
     * @brief Add a process model to the store
     */
    void addModel(const ProcessModelRecord& model) {
        models_.push_back(model);
        last_modified_ms_ = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    }

    /**
     * @brief Simple query by name/state filter
     */
    QueryResult simpleQuery(const std::string& state_filter, int limit = 10) {
        QueryResult result;
        result.total_count = 0;

        for (const auto& model : models_) {
            if (model.state == state_filter) {
                result.total_count++;
                if (static_cast<int>(result.matches.size()) < limit) {
                    result.matches.push_back(model);
                }
            }
        }

        return result;
    }

    /**
     * @brief Complex query with multiple filters
     */
    QueryResult complexQuery(const std::string& state, const std::string& name_pattern,
                            int min_revision, int limit = 10) {
        QueryResult result;
        result.total_count = 0;

        for (const auto& model : models_) {
            bool matches = (model.state == state) &&
                          (model.name.find(name_pattern) != std::string::npos) &&
                          (model.revision >= min_revision);

            if (matches) {
                result.total_count++;
                if (static_cast<int>(result.matches.size()) < limit) {
                    result.matches.push_back(model);
                }
            }
        }

        return result;
    }

    /**
     * @brief Full-text search on description
     */
    QueryResult fullTextSearch(const std::string& query_term, int limit = 10) {
        QueryResult result;
        result.total_count = 0;

        for (const auto& model : models_) {
            if (model.description.find(query_term) != std::string::npos) {
                result.total_count++;
                if (static_cast<int>(result.matches.size()) < limit) {
                    result.matches.push_back(model);
                }
            }
        }

        return result;
    }

    /**
     * @brief Embedding similarity search (cosine distance)
     */
    QueryResult embeddingSimilaritySearch(const std::vector<float>& query_embedding,
                                         float min_similarity = 0.7f, int limit = 10) {
        QueryResult result;

        std::vector<std::pair<ProcessModelRecord, float>> scored;

        for (auto model : models_) {
            float similarity = cosineSimilarity(query_embedding, model.embedding);
            if (similarity >= min_similarity) {
                model.relevance_score = similarity;
                scored.push_back({model, similarity});
                result.total_count++;
            }
        }

        // Sort by similarity descending
        std::sort(scored.begin(), scored.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        // Return top-k
        for (int i = 0; i < limit && i < static_cast<int>(scored.size()); ++i) {
            result.matches.push_back(scored[i].first);
        }

        return result;
    }

    /**
     * @brief Paginated query
     */
    QueryResult paginatedQuery(const std::string& state, int page, int page_size = 20) {
        QueryResult result;

        std::vector<ProcessModelRecord> filtered;
        for (const auto& model : models_) {
            if (model.state == state) {
                filtered.push_back(model);
            }
        }

        result.total_count = static_cast<int>(filtered.size());

        int start_idx = page * page_size;
        int end_idx = std::min(start_idx + page_size, static_cast<int>(filtered.size()));

        if (start_idx < static_cast<int>(filtered.size())) {
            result.matches.insert(result.matches.end(), 
                                filtered.begin() + start_idx,
                                filtered.begin() + end_idx);
        }

        return result;
    }

    /**
     * @brief Retrieve and rank models by relevance
     */
    QueryResult rankAndSort(const std::vector<ProcessModelRecord>& candidates,
                           int limit = 10) {
        QueryResult result;

        std::vector<ProcessModelRecord> ranked = candidates;
        std::sort(ranked.begin(), ranked.end(),
                  [](const auto& a, const auto& b) {
                      return a.relevance_score > b.relevance_score;
                  });

        result.total_count = static_cast<int>(ranked.size());
        for (int i = 0; i < limit && i < static_cast<int>(ranked.size()); ++i) {
            result.matches.push_back(ranked[i]);
        }

        return result;
    }

    size_t modelCount() const { return models_.size(); }
    int64_t lastModifiedMs() const { return last_modified_ms_; }

private:
    /**
     * @brief Calculate cosine similarity between two embedding vectors
     */
    static float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.empty() || b.empty() || a.size() != b.size()) {
            return 0.0f;
        }

        float dot_product = 0.0f;
        float norm_a = 0.0f;
        float norm_b = 0.0f;

        for (size_t i = 0; i < a.size(); ++i) {
            dot_product += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }

        norm_a = std::sqrt(norm_a);
        norm_b = std::sqrt(norm_b);

        if (norm_a > 0.0f && norm_b > 0.0f) {
            return dot_product / (norm_a * norm_b);
        }

        return 0.0f;
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate synthetic embedding vector
 */
static std::vector<float> generateEmbedding(const std::string& text) {
    std::vector<float> embedding(kEmbeddingDim);

    uint64_t hash = 42;
    for (char c : text) {
        hash = ((hash << 5) + hash) + c;
    }

    std::mt19937 gen(hash);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (auto& v : embedding) {
        v = dist(gen);
    }

    // Normalize
    float norm = 0.0f;
    for (float v : embedding) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 0.0f) {
        for (auto& v : embedding) {
            v /= norm;
        }
    }

    return embedding;
}

/**
 * @brief Generate synthetic process model collection
 */
static std::vector<ProcessModelRecord> generateModelCollection(int count) {
    std::vector<ProcessModelRecord> models;

    static const char* states[] = {"DRAFT", "ACTIVE", "DEPRECATED"};
    static const char* descriptions[] = {
        "Customer order processing",
        "Invoice generation and validation",
        "Payment reconciliation",
        "Inventory management workflow",
        "Procurement approval process",
        "HR onboarding procedure",
        "IT support ticket resolution",
        "Quality assurance review",
        "Compliance audit trail",
        "Financial reporting cycle"
    };

    int64_t base_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

    for (int i = 0; i < count; ++i) {
        ProcessModelRecord model;
        model.id = "model_" + std::to_string(i);
        model.name = "ProcessModel_" + std::to_string(i / 10);
        model.description = descriptions[i % 10];
        model.state = states[i % 3];
        model.embedding = generateEmbedding(model.name);
        model.revision = 1 + (i % 5);
        model.created_ms = base_ms - (i * 1000);
        model.relevance_score = 0.5f + (i % 50) * 0.01f;

        models.push_back(model);
    }

    return models;
}

// ============================================================================
// Retriever Performance Benchmarks
// ============================================================================

/**
 * RP-01: Simple Query (1k models)
 */
static void BM_RP01_SimpleQuery(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;
    auto models = generateModelCollection(num_models);

    auto retriever = std::make_unique<ProcessModelRetriever>();
    for (const auto& model : models) {
        retriever->addModel(model);
    }

    std::vector<double> latencies;
    latencies.reserve(100);

    for (auto _ : state) {
        state.PauseTiming();
        latencies.clear();
        state.ResumeTiming();

        for (int i = 0; i < 50; ++i) {
            std::string state_filter = (i % 3 == 0) ? "ACTIVE" : "DRAFT";

            auto start = std::chrono::high_resolution_clock::now();
            auto result = retriever->simpleQuery(state_filter, 10);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));

            benchmark::DoNotOptimize(result);
        }
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        double p99 = latencies[std::min(size_t(99), latencies.size() - 1)];
        state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(50 * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_RP01_SimpleQuery)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * RP-02: Complex Query (1k models)
 */
static void BM_RP02_ComplexQuery(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;
    auto models = generateModelCollection(num_models);

    auto retriever = std::make_unique<ProcessModelRetriever>();
    for (const auto& model : models) {
        retriever->addModel(model);
    }

    int64_t queries_done = 0;

    for (auto _ : state) {
        for (int i = 0; i < 50; ++i) {
            std::string state_filter = (i % 3 == 0) ? "ACTIVE" : "DRAFT";
            auto result = retriever->complexQuery(state_filter, "Model", i % 5, 10);
            benchmark::DoNotOptimize(result);
            queries_done++;
        }
    }

    state.SetItemsProcessed(queries_done);
    state.counters["queries_per_sec"] = benchmark::Counter(queries_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RP02_ComplexQuery)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * RP-03: Full-Text Search (1k models)
 */
static void BM_RP03_FullTextSearch(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;
    auto models = generateModelCollection(num_models);

    auto retriever = std::make_unique<ProcessModelRetriever>();
    for (const auto& model : models) {
        retriever->addModel(model);
    }

    int64_t searches_done = 0;

    for (auto _ : state) {
        static const char* search_terms[] = {
            "order", "payment", "inventory", "procurement", "approval"
        };

        for (int i = 0; i < 50; ++i) {
            std::string term = search_terms[i % 5];
            auto result = retriever->fullTextSearch(term, 10);
            benchmark::DoNotOptimize(result);
            searches_done++;
        }
    }

    state.SetItemsProcessed(searches_done);
}

BENCHMARK(BM_RP03_FullTextSearch)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * RP-04: Embedding Similarity Search (1k models)
 */
static void BM_RP04_EmbeddingSimilarity(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;
    auto models = generateModelCollection(num_models);

    auto retriever = std::make_unique<ProcessModelRetriever>();
    for (const auto& model : models) {
        retriever->addModel(model);
    }

    auto query_embedding = generateEmbedding("test_query");

    int64_t searches_done = 0;

    for (auto _ : state) {
        for (int i = 0; i < 50; ++i) {
            float min_similarity = 0.5f + (i % 5) * 0.1f;
            auto result = retriever->embeddingSimilaritySearch(query_embedding, min_similarity, 10);
            benchmark::DoNotOptimize(result);
            searches_done++;
        }
    }

    state.SetItemsProcessed(searches_done);
}

BENCHMARK(BM_RP04_EmbeddingSimilarity)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * RP-05: Pagination Query (10k models)
 */
static void BM_RP05_PaginationQuery(benchmark::State& state) {
    const int num_models = kLargeDatasetSize;
    auto models = generateModelCollection(num_models);

    auto retriever = std::make_unique<ProcessModelRetriever>();
    for (const auto& model : models) {
        retriever->addModel(model);
    }

    int64_t page_accesses = 0;

    for (auto _ : state) {
        for (int page = 0; page < 10; ++page) {
            auto result = retriever->paginatedQuery("ACTIVE", page, 20);
            benchmark::DoNotOptimize(result);
            page_accesses++;
        }
    }

    state.SetItemsProcessed(page_accesses);
}

BENCHMARK(BM_RP05_PaginationQuery)->Iterations(5)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * RP-06: Concurrent Query (1k models, 4 threads)
 */
static void BM_RP06_ConcurrentQuery(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;
    const int num_threads = 4;

    auto models = generateModelCollection(num_models);

    int64_t total_queries = 0;

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        std::vector<int64_t> thread_queries(num_threads, 0);
        state.ResumeTiming();

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&models, &thread_queries, t]() {
                auto retriever = std::make_unique<ProcessModelRetriever>();
                for (const auto& model : models) {
                    retriever->addModel(model);
                }

                for (int q = 0; q < 100; ++q) {
                    std::string state_filter = (q % 3 == 0) ? "ACTIVE" : "DRAFT";
                    auto result = retriever->simpleQuery(state_filter, 10);
                    benchmark::DoNotOptimize(result);
                    thread_queries[t]++;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        for (int64_t q : thread_queries) {
            total_queries += q;
        }
    }

    state.SetItemsProcessed(total_queries);
    state.counters["queries_per_sec"] = benchmark::Counter(total_queries, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RP06_ConcurrentQuery)->Iterations(5)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * RP-07: Query Under Churn (1k -> 10k models)
 */
static void BM_RP07_QueryUnderChurn(benchmark::State& state) {
    const int small_model_count = kMediumDatasetSize;
    const int large_model_count = kLargeDatasetSize;

    auto retriever = std::make_unique<ProcessModelRetriever>();

    std::vector<double> latencies;
    latencies.reserve(100);

    for (auto _ : state) {
        state.PauseTiming();
        retriever = std::make_unique<ProcessModelRetriever>();

        // Start with 1k models
        auto models = generateModelCollection(small_model_count);
        for (const auto& model : models) {
            retriever->addModel(model);
        }

        latencies.clear();
        state.ResumeTiming();

        // Run queries while adding models
        int queries_run = 0;
        int models_added = small_model_count;

        while (models_added < large_model_count) {
            auto start = std::chrono::high_resolution_clock::now();
            auto result = retriever->simpleQuery("ACTIVE", 10);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));

            benchmark::DoNotOptimize(result);

            // Add 10 new models every 10 queries
            if (queries_run++ % 10 == 0 && models_added < large_model_count) {
                auto new_models = generateModelCollection(std::min(10, large_model_count - models_added));
                for (const auto& model : new_models) {
                    retriever->addModel(model);
                }
                models_added += static_cast<int>(new_models.size());
            }
        }
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        double p99 = latencies[std::min(size_t(99), latencies.size() - 1)];
        state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 50);
}

BENCHMARK(BM_RP07_QueryUnderChurn)->Iterations(5)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * RP-08: Ranking/Sorting (1k results)
 */
static void BM_RP08_RankingAndSorting(benchmark::State& state) {
    const int num_results = 1000;

    auto retriever = std::make_unique<ProcessModelRetriever>();
    auto candidates = generateModelCollection(num_results);

    int64_t sorts_done = 0;

    for (auto _ : state) {
        for (int i = 0; i < 20; ++i) {
            auto result = retriever->rankAndSort(candidates, 100);
            benchmark::DoNotOptimize(result);
            sorts_done++;
        }
    }

    state.SetItemsProcessed(sorts_done);
    state.counters["sorts_per_sec"] = benchmark::Counter(sorts_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_RP08_RankingAndSorting)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

}  // namespace themis::process::benchmark

BENCHMARK_MAIN();
