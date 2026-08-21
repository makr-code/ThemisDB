// SIMULATION NOTE:
// Purpose: Synthetic vector-search benchmark for L2 and cosine batch queries.
// Activation: Benchmark-only synthetic vector dataset and score path.
// Production Delta: This does not call the production ANN/vector index implementation; it computes an in-memory distance model over generated vectors.
// Removal Plan: Replace with real ANN/index benchmarks once the vector-search runtime exposes a stable benchmark harness.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

namespace {

using Vector = std::vector<float>;

Vector makeVector(std::size_t dimension, std::mt19937& rng) {
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    Vector vector(dimension);
    for (auto& value : vector) {
        value = distribution(rng);
    }
    return vector;
}

float l2Distance(const Vector& lhs, const Vector& rhs) {
    float total = 0.0f;
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        const float delta = lhs[index] - rhs[index];
        total += delta * delta;
    }
    return std::sqrt(total);
}

float cosineSimilarity(const Vector& lhs, const Vector& rhs) {
    float dot = 0.0f;
    float lhs_norm = 0.0f;
    float rhs_norm = 0.0f;
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        dot += lhs[index] * rhs[index];
        lhs_norm += lhs[index] * lhs[index];
        rhs_norm += rhs[index] * rhs[index];
    }
    return dot / (std::sqrt(lhs_norm) * std::sqrt(rhs_norm) + 1e-6f);
}

void BM_VectorSearchL2(benchmark::State& state) {
    const std::size_t vector_count = static_cast<std::size_t>(state.range(0));
    const std::size_t dimension = static_cast<std::size_t>(state.range(1));
    std::mt19937 rng(42);

    std::vector<Vector> dataset;
    dataset.reserve(vector_count);
    for (std::size_t index = 0; index < vector_count; ++index) {
        dataset.push_back(makeVector(dimension, rng));
    }
    const Vector query = makeVector(dimension, rng);

    for (auto _ : state) {
        float best = std::numeric_limits<float>::max();
        for (const auto& candidate : dataset) {
            best = std::min(best, l2Distance(query, candidate));
        }
        benchmark::DoNotOptimize(best);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(vector_count));
}

void BM_VectorSearchCosineBatch(benchmark::State& state) {
    const std::size_t vector_count = static_cast<std::size_t>(state.range(0));
    const std::size_t dimension = static_cast<std::size_t>(state.range(1));
    std::mt19937 rng(42);

    std::vector<Vector> dataset;
    dataset.reserve(vector_count);
    for (std::size_t index = 0; index < vector_count; ++index) {
        dataset.push_back(makeVector(dimension, rng));
    }

    std::vector<Vector> queries;
    queries.reserve(64);
    for (std::size_t index = 0; index < 64; ++index) {
        queries.push_back(makeVector(dimension, rng));
    }

    for (auto _ : state) {
        float aggregate = 0.0f;
        for (const auto& query : queries) {
            float best = -1.0f;
            for (const auto& candidate : dataset) {
                best = std::max(best, cosineSimilarity(query, candidate));
            }
            aggregate += best;
        }
        benchmark::DoNotOptimize(aggregate);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(vector_count * queries.size()));
}

BENCHMARK(BM_VectorSearchL2)->Args({1000, 128})->Args({5000, 128})->Args({10000, 256})->UseRealTime();
BENCHMARK(BM_VectorSearchCosineBatch)->Args({1000, 128})->Args({5000, 128})->UseRealTime();

}  // namespace