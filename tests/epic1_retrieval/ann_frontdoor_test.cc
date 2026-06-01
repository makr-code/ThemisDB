/**
 * @file ann_frontdoor_test.cc
 * @brief Contract tests for the ANN Frontdoor interface (sub-issue #5424).
 *
 * Validates factory construction, route reporting, backend switching, and
 * callback registration against the scaffold stub implementation.
 * Production HNSW / DiskANN integration is tracked in sub-issue #5424.
 */

#include "retrieval/include/ann_frontdoor.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace themis::retrieval;

namespace {

AnnFrontdoorConfig defaultConfig() {
    AnnFrontdoorConfig cfg;
    cfg.preferred_backend       = AnnBackend::Auto;
    cfg.hot_index_size_limit_bytes = 4ULL * 1024 * 1024 * 1024;
    cfg.min_score_threshold     = 0.0f;
    cfg.enable_cache            = true;
    cfg.index_directory         = "/tmp/test_index";
    return cfg;
}

AnnQuery simpleQuery(std::uint32_t top_k = 5) {
    AnnQuery q;
    q.embedding = std::vector<float>(128, 0.1f);
    q.top_k     = top_k;
    q.backend   = AnnBackend::Auto;
    return q;
}

} // namespace

class AnnFrontdoorTest : public ::testing::Test {
protected:
    void SetUp() override {
        frontdoor_ = makeAnnFrontdoor(defaultConfig());
        ASSERT_NE(frontdoor_, nullptr);
    }

    std::unique_ptr<IAnnFrontdoor> frontdoor_;
};

TEST_F(AnnFrontdoorTest, FactoryReturnsNonNull) {
    EXPECT_NE(frontdoor_, nullptr);
}

TEST_F(AnnFrontdoorTest, InitialRouteIsHot) {
    // Scaffold stub defaults to Hot.
    EXPECT_EQ(frontdoor_->getRoute(), AnnRoute::Hot);
}

TEST_F(AnnFrontdoorTest, SearchReturnsResultWithRoute) {
    AnnResult result = frontdoor_->search(simpleQuery());
    // Scaffold: route is propagated; candidates list may be empty.
    EXPECT_EQ(result.route, AnnRoute::Hot);
}

TEST_F(AnnFrontdoorTest, SearchWithTopKZero) {
    AnnQuery q = simpleQuery(0);
    AnnResult result = frontdoor_->search(q);
    // Should not throw; route must be valid.
    EXPECT_EQ(result.route, AnnRoute::Hot);
}

TEST_F(AnnFrontdoorTest, SetBackendDoesNotThrow) {
    EXPECT_NO_THROW(frontdoor_->setBackend(AnnBackend::HNSW));
    EXPECT_NO_THROW(frontdoor_->setBackend(AnnBackend::DiskANN));
    EXPECT_NO_THROW(frontdoor_->setBackend(AnnBackend::Auto));
}

TEST_F(AnnFrontdoorTest, CallbackRegistrationDoesNotThrow) {
    bool called = false;
    EXPECT_NO_THROW(frontdoor_->onSearch([&](const AnnResult&) {
        called = true;
    }));
    // Scaffold stub does not invoke the callback automatically.
    (void)called;
}

TEST_F(AnnFrontdoorTest, MultipleSearchCallsAreStable) {
    for (int i = 0; i < 10; ++i) {
        AnnResult r = frontdoor_->search(simpleQuery());
        EXPECT_EQ(r.route, AnnRoute::Hot);
    }
}

TEST_F(AnnFrontdoorTest, EmptyEmbeddingSearchDoesNotThrow) {
    AnnQuery q;
    q.top_k = 10;
    // Empty embedding — scaffold must not crash.
    EXPECT_NO_THROW(frontdoor_->search(q));
}
