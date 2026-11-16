#include <gtest/gtest.h>

#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "themis/base/interfaces/index_interface.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

class LegacyVectorIndex : public themis::IVectorIndex {
public:
    [[nodiscard]] bool insert(std::string_view, const std::vector<float>&) override {
        return true;
    }

    [[nodiscard]] bool remove(std::string_view) override { return true; }

    [[nodiscard]] std::vector<themis::VectorSearchResult> search(
        const std::vector<float>&,
        uint32_t,
        const themis::IExpressionEvaluator*) const override {
        ++legacy_search_calls;
        return {themis::VectorSearchResult("legacy-result", 0.25f)};
    }

    [[nodiscard]] std::vector<themis::VectorSearchResult> rangeSearch(
        const std::vector<float>&,
        float,
        const themis::IExpressionEvaluator*) const override {
        return {};
    }

    [[nodiscard]] std::string getName() const override { return "legacy"; }
    [[nodiscard]] uint32_t getDimension() const override { return 1; }
    [[nodiscard]] std::string getStatistics() const override { return "{}"; }

    mutable std::atomic<unsigned int> legacy_search_calls{0};
};

class CooperativeVectorIndex final : public LegacyVectorIndex {
public:
    [[nodiscard]] themis::VectorSearchOutcome search(
        const std::vector<float>&,
        uint32_t,
        const themis::IExpressionEvaluator*,
        const themis::query::RequestContext& request) const override {
        entered.store(true, std::memory_order_release);
        while (!request.shouldStop()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        return {{}, request.stopReason()};
    }

    mutable std::atomic<bool> entered{false};
};

struct PollCancellationState {
    mutable std::atomic<unsigned int> polls{0};
    unsigned int stop_after = 0;
};

bool cancelAfterPolls(const void* opaque) noexcept {
    const auto* state = static_cast<const PollCancellationState*>(opaque);
    return state->polls.fetch_add(1, std::memory_order_relaxed) + 1 >=
           state->stop_after;
}

class CountingEvaluator final : public themis::IExpressionEvaluator {
public:
    [[nodiscard]] bool evaluate(const std::string&, const void*) const override {
        evaluations.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    [[nodiscard]] std::string get_expression_type() const override {
        return "themis_json_context_v1";
    }

    mutable std::atomic<size_t> evaluations{0};
};

bool cancelAfterCandidateBatch(const void* opaque) noexcept {
    const auto* evaluator = static_cast<const CountingEvaluator*>(opaque);
    return evaluator->evaluations.load(std::memory_order_relaxed) >= 32;
}

class ScopedDbDirectory {
public:
    explicit ScopedDbDirectory(std::filesystem::path path) : path_(std::move(path)) {
        std::filesystem::create_directories(path_.parent_path());
    }

    ~ScopedDbDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
        std::filesystem::remove(path_.parent_path(), error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

private:
    std::filesystem::path path_;
};

ScopedDbDirectory makeDbDirectory(const std::string& prefix) {
    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    return ScopedDbDirectory(std::filesystem::current_path() / ".qry07-test-data" /
                             (prefix + std::to_string(suffix)));
}

::testing::AssertionResult populateVectorIndex(themis::VectorIndexManager& index,
                                                size_t count) {
    const auto init_status =
        index.init("vectors", 1, themis::VectorIndexManager::Metric::L2);
    if (!init_status.ok) {
        return ::testing::AssertionFailure() << init_status.message;
    }
    for (size_t i = 0; i < count; ++i) {
        themis::BaseEntity entity("pk-" + std::to_string(i));
        entity.setField("embedding", std::vector<float>{static_cast<float>(i)});
        const auto status = index.addEntity(entity);
        if (!status.ok) {
            return ::testing::AssertionFailure() << status.message;
        }
    }
    return ::testing::AssertionSuccess();
}

} // namespace

TEST(VectorIndexRequestContext, LegacyIndexReportsPreCancellationWithoutSearching) {
    LegacyVectorIndex index;
    auto token = std::make_shared<themis::query::QueryCancellationToken>();
    token->cancel();
    const themis::query::RequestContext request(token);

    const auto outcome = index.search({0.0f}, 1, nullptr, request);

    EXPECT_EQ(outcome.stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_TRUE(outcome.results.empty());
    EXPECT_EQ(index.legacy_search_calls.load(), 0u);
}

TEST(VectorIndexRequestContext, LegacyIndexReportsExpiredDeadlineDistinctly) {
    LegacyVectorIndex index;
    const auto deadline = themis::query::RequestContext::Clock::now() -
                          std::chrono::milliseconds{1};
    const themis::query::RequestContext request(nullptr, deadline);

    const auto outcome = index.search({0.0f}, 1, nullptr, request);

    EXPECT_EQ(outcome.stop_reason, themis::query::StopReason::DeadlineExceeded);
    EXPECT_TRUE(outcome.results.empty());
    EXPECT_EQ(index.legacy_search_calls.load(), 0u);
}

TEST(VectorIndexRequestContext, CooperativeIndexStopsDuringInFlightSearch) {
    CooperativeVectorIndex index;
    auto token = std::make_shared<themis::query::QueryCancellationToken>();
    const themis::query::RequestContext request(token);
    themis::VectorSearchOutcome outcome;

    std::thread search_thread([&] {
        outcome = index.search({0.0f}, 1, nullptr, request);
    });

    const auto wait_deadline = std::chrono::steady_clock::now() +
                               std::chrono::seconds{2};
    while (!index.entered.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < wait_deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    const bool search_entered = index.entered.load(std::memory_order_acquire);
    token->cancel();
    search_thread.join();

    EXPECT_TRUE(search_entered);
    EXPECT_EQ(outcome.stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_TRUE(outcome.results.empty());
}

TEST(VectorIndexRequestContext, ManagerStopsDuringLargeCacheScanWithoutPartialResults) {
    auto db_directory = makeDbDirectory("cache_scan_");
    themis::RocksDBWrapper::Config config;
    config.db_path = db_directory.path().string();
    themis::RocksDBWrapper db(config);
    ASSERT_TRUE(db.open());

    themis::VectorIndexManager index(db);
    ASSERT_TRUE(populateVectorIndex(index, 512));
    index.setVectorEncryptionEnabled(true);

    PollCancellationState cancellation;
    cancellation.stop_after = 12;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &cancelAfterPolls, &cancellation);
    themis::query::StopReason stop_reason = themis::query::StopReason::None;

    const auto [status, results] = index.searchKnn(
        {0.0f}, 10, nullptr, request, &stop_reason);

    EXPECT_TRUE(status.ok);
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_TRUE(results.empty());
    EXPECT_GE(cancellation.polls.load(std::memory_order_relaxed), 12u);
}

TEST(VectorIndexRequestContext, ManagerStopsDuringEvaluatorFilteringWithoutPartialResults) {
    auto db_directory = makeDbDirectory("evaluator_filter_");
    themis::RocksDBWrapper::Config config;
    config.db_path = db_directory.path().string();
    themis::RocksDBWrapper db(config);
    ASSERT_TRUE(db.open());

    themis::VectorIndexManager index(db);
    ASSERT_TRUE(populateVectorIndex(index, 257));
    index.setVectorEncryptionEnabled(true);

    CountingEvaluator evaluator;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &cancelAfterCandidateBatch, &evaluator);
    themis::query::StopReason stop_reason = themis::query::StopReason::None;

    const auto [status, results] = index.searchKnnEvaluated(
        {0.0f}, 64, &evaluator, 4, nullptr, request, &stop_reason);

    EXPECT_TRUE(status.ok);
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_TRUE(results.empty());
    EXPECT_EQ(evaluator.evaluations.load(std::memory_order_relaxed), 32u);
}
