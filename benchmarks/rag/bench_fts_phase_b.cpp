// SIMULATION NOTE:
// Purpose: Synthetic FTS benchmark for phrase and proximity query acceleration work.
// Activation: Always active in benchmark builds; not used in production query execution.
// Production Delta: The corpus and match logic are deterministic synthetic tokens, not production FTS index state.
// Removal Plan: Replace with module-level benchmarks against the production FTS indexer/query engine when those hooks are available.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct Document {
    std::vector<std::string> tokens;
};

std::vector<Document> makeCorpus(std::size_t count) {
    std::vector<Document> corpus;
    corpus.reserve(count);

    for (std::size_t index = 0; index < count; ++index) {
        Document document;
        document.tokens = {
            "wikipedia", "search", "benchmark", "phase", "b",
            "article", std::to_string(index), "phrase", "proximity", "query"};
        corpus.push_back(std::move(document));
    }

    return corpus;
}

bool containsPhrase(const Document& document, std::string_view first, std::string_view second) {
    for (std::size_t index = 0; index + 1U < document.tokens.size(); ++index) {
        if (document.tokens[index] == first && document.tokens[index + 1U] == second) {
            return true;
        }
    }
    return false;
}

bool containsNear(const Document& document, std::string_view first, std::string_view second, std::size_t distance) {
    for (std::size_t first_index = 0; first_index < document.tokens.size(); ++first_index) {
        if (document.tokens[first_index] != first) {
            continue;
        }
        const std::size_t begin = first_index + 1U;
        const std::size_t end = std::min(document.tokens.size(), first_index + distance + 1U);
        for (std::size_t index = begin; index < end; ++index) {
            if (document.tokens[index] == second) {
                return true;
            }
        }
    }
    return false;
}

void BM_FTSPhraseQuery(benchmark::State& state) {
    const std::size_t document_count = static_cast<std::size_t>(state.range(0));
    const auto corpus = makeCorpus(document_count);

    for (auto _ : state) {
        std::size_t matches = 0;
        for (const auto& document : corpus) {
            matches += containsPhrase(document, "phrase", "proximity") ? 1U : 0U;
        }
        benchmark::DoNotOptimize(matches);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(document_count));
}

void BM_FTSProximityQuery(benchmark::State& state) {
    const std::size_t document_count = static_cast<std::size_t>(state.range(0));
    const std::size_t distance = static_cast<std::size_t>(state.range(1));
    const auto corpus = makeCorpus(document_count);

    for (auto _ : state) {
        std::size_t matches = 0;
        for (const auto& document : corpus) {
            matches += containsNear(document, "phase", "query", distance) ? 1U : 0U;
        }
        benchmark::DoNotOptimize(matches);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(document_count));
}

BENCHMARK(BM_FTSPhraseQuery)->Arg(1000)->Arg(5000)->Arg(10000)->UseRealTime();
BENCHMARK(BM_FTSProximityQuery)->Args({1000, 2})->Args({5000, 4})->Args({10000, 6})->UseRealTime();

}  // namespace