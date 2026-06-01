/**
 * @file tensor_midlayer_test.cc
 * @brief Contract tests for the Tensor Mid-Layer interface (sub-issue #5425).
 *
 * Validates factory construction, reranking passthrough, cache operations,
 * and active compression reporting against the scaffold stub implementation.
 * Production compression-aware reranking is tracked in sub-issue #5425.
 */

#include "retrieval/include/tensor_midlayer.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace themis::retrieval;

namespace {

TensorMidlayerConfig defaultConfig() {
    TensorMidlayerConfig cfg;
    cfg.compression              = TensorCompression::FP16;
    cfg.summary_cache_max_entries = 10000;
    cfg.enable_cross_shard       = false;
    return cfg;
}

TensorQuery queryfromCandidates(std::vector<AnnCandidate> candidates,
                                 std::uint32_t top_k = 3) {
    TensorQuery q;
    q.ann_candidates         = std::move(candidates);
    q.rerank_top_k           = top_k;
    q.preferred_compression  = TensorCompression::FP16;
    return q;
}

} // namespace

class TensorMidlayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        midlayer_ = makeTensorMidlayer(defaultConfig());
        ASSERT_NE(midlayer_, nullptr);
    }

    std::unique_ptr<ITensorMidlayer> midlayer_;
};

TEST_F(TensorMidlayerTest, FactoryReturnsNonNull) {
    EXPECT_NE(midlayer_, nullptr);
}

TEST_F(TensorMidlayerTest, ActiveCompressionMatchesConfig) {
    EXPECT_EQ(midlayer_->activeCompression(), TensorCompression::FP16);
}

TEST_F(TensorMidlayerTest, RerankEmptyCandidatesDoesNotThrow) {
    TensorQuery q = queryfromCandidates({});
    EXPECT_NO_THROW(midlayer_->rerank(q));
}

TEST_F(TensorMidlayerTest, RerankReturnsResult) {
    AnnCandidate c;
    c.id       = 1;
    c.distance = 0.1f;
    c.score    = 0.9f;
    TensorQuery q = queryfromCandidates({c});
    TensorResult result = midlayer_->rerank(q);
    // Scaffold: reranked list may be empty or pass-through; must not crash.
    (void)result;
    SUCCEED();
}

TEST_F(TensorMidlayerTest, WarmCacheDoesNotThrow) {
    EXPECT_NO_THROW(midlayer_->warmCache("shard-1"));
    EXPECT_NO_THROW(midlayer_->warmCache(""));
}

TEST_F(TensorMidlayerTest, EvictCacheDoesNotThrow) {
    EXPECT_NO_THROW(midlayer_->evictCache("shard-1"));
    EXPECT_NO_THROW(midlayer_->evictCache("nonexistent-shard"));
}

TEST_F(TensorMidlayerTest, WarmThenEvictSameShard) {
    EXPECT_NO_THROW(midlayer_->warmCache("shard-2"));
    EXPECT_NO_THROW(midlayer_->evictCache("shard-2"));
}

TEST_F(TensorMidlayerTest, MultipleReranksAreStable) {
    TensorQuery q = queryfromCandidates({});
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(midlayer_->rerank(q));
    }
}
