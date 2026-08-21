#include <benchmark/benchmark.h>

#include <algorithm>
#include <functional>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Article {
    std::string title;
    std::string body;
};

std::vector<Article> makeCorpus(std::size_t count) {
    std::vector<Article> corpus;
    corpus.reserve(count);

    for (std::size_t index = 0; index < count; ++index) {
        corpus.push_back(Article{
            "Wikipedia article " + std::to_string(index),
            "Wikipedia content block " + std::to_string(index) +
                " about search indexing, references, and revision history."});
    }

    return corpus;
}

std::size_t ingestArticle(const Article& article) {
    std::size_t token_count = 0;
    const std::string_view title{article.title};
    const std::string_view body{article.body};

    token_count += static_cast<std::size_t>(std::count(title.begin(), title.end(), ' ')) + 1U;
    token_count += static_cast<std::size_t>(std::count(body.begin(), body.end(), ' ')) + 1U;
    token_count += static_cast<std::size_t>(std::hash<std::string_view>{}(title) & 0xFFU);
    token_count += static_cast<std::size_t>(std::hash<std::string_view>{}(body) & 0xFFU);
    return token_count;
}

void BM_WikipediaThroughput(benchmark::State& state) {
    const std::size_t article_count = static_cast<std::size_t>(state.range(0));
    const auto corpus = makeCorpus(article_count);

    for (auto _ : state) {
        std::size_t processed = 0;
        for (const auto& article : corpus) {
            processed += ingestArticle(article);
        }
        benchmark::DoNotOptimize(processed);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(article_count));
    state.counters["articles_per_sec"] = benchmark::Counter(
        static_cast<double>(article_count), benchmark::Counter::kIsRate);
}

void BM_WikipediaCheckpointResume(benchmark::State& state) {
    const std::size_t article_count = static_cast<std::size_t>(state.range(0));
    const auto corpus = makeCorpus(article_count);

    std::string checkpoint;
    checkpoint.reserve(article_count * 16U);

    for (auto _ : state) {
        checkpoint.clear();
        for (const auto& article : corpus) {
            checkpoint.append(article.title);
            checkpoint.push_back('|');
            checkpoint.append(article.body);
            checkpoint.push_back('\n');
        }
        benchmark::DoNotOptimize(checkpoint);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(article_count));
}

BENCHMARK(BM_WikipediaThroughput)->Arg(1000)->Arg(5000)->Arg(10000)->UseRealTime();
BENCHMARK(BM_WikipediaCheckpointResume)->Arg(1000)->Arg(5000)->UseRealTime();

}  // namespace