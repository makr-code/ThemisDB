#include <gtest/gtest.h>

#include "themis/base/interfaces/index_interface.h"

#include <atomic>
#include <chrono>
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
