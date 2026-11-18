#include <gtest/gtest.h>

#include "index/rotary_embeddings.h"
#include "index/vector_index.h"
#include "query/request_context.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

struct PollState {
    std::atomic<unsigned int> polls{0};
    unsigned int stop_after = 0;
};

bool stopAfterPolls(const void* opaque) noexcept {
    auto* state = const_cast<PollState*>(static_cast<const PollState*>(opaque));
    const auto poll = state->polls.fetch_add(1, std::memory_order_relaxed) + 1;
    return state->stop_after != 0 && poll >= state->stop_after;
}

class VectorIndexRequestCancellationTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto suffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = std::filesystem::temp_directory_path() /
                   ("themis_vector_index_cancel_" + std::to_string(suffix));

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        index_ = std::make_unique<themis::VectorIndexManager>(*db_);
        ASSERT_TRUE(index_->init(
            "rope_cancel", 128, themis::VectorIndexManager::Metric::COSINE).ok);

        themis::RotationConfig rotation;
        rotation.hidden_dim = 128;
        rotation.num_rotation_pairs = 64;
        rotation.computeThetaCache();
        ASSERT_TRUE(index_->setRotaryEmbeddingConfig(rotation).ok);
    }

    void TearDown() override {
        index_.reset();
        db_.reset();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);
    }

    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::VectorIndexManager> index_;
};

TEST_F(VectorIndexRequestCancellationTest,
       RotationSearchPropagatesStopToKnnAndDiscardsResults) {
    PollState state;
    state.stop_after = 4;
    const themis::query::RequestContext request(
        nullptr, std::nullopt, &stopAfterPolls, &state);
    auto stop_reason = themis::query::StopReason::None;
    std::vector<float> query(128, 1.0F);

    auto [status, results] = index_->searchWithRotation(
        query, 10, 0, nullptr, request, &stop_reason);

    EXPECT_TRUE(status.ok) << status.message;
    EXPECT_TRUE(results.empty());
    EXPECT_EQ(stop_reason, themis::query::StopReason::Cancelled);
    EXPECT_GE(state.polls.load(std::memory_order_relaxed), state.stop_after);
}

TEST_F(VectorIndexRequestCancellationTest,
       RotationSearchDistinguishesExpiredDeadlineFromEmptyResults) {
    const auto request = themis::query::RequestContext::withTimeout(
        std::chrono::milliseconds{0});
    auto stop_reason = themis::query::StopReason::None;
    std::vector<float> query(128, 1.0F);

    auto [status, results] = index_->searchWithRotation(
        query, 10, 0, nullptr, request, &stop_reason);

    EXPECT_TRUE(status.ok) << status.message;
    EXPECT_TRUE(results.empty());
    EXPECT_EQ(stop_reason, themis::query::StopReason::DeadlineExceeded);
}

}  // namespace
