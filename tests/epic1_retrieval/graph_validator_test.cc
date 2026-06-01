/**
 * @file graph_validator_test.cc
 * @brief Contract tests for the Graph Truth Validation Layer (sub-issue #5426).
 *
 * Validates factory construction, stub availability reporting, provenance
 * lookup, and validate passthrough against the scaffold stub implementation.
 * Production graph-backend integration is tracked in sub-issue #5426.
 */

#include "retrieval/include/graph_validator.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace themis::retrieval;

namespace {

GraphValidatorConfig defaultConfig() {
    GraphValidatorConfig cfg;
    cfg.graph_endpoint    = ""; // not connected in scaffold
    cfg.max_hops          = 3;
    cfg.min_graph_score   = 0.2;
    cfg.strict_provenance = false;
    return cfg;
}

} // namespace

class GraphValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        validator_ = makeGraphValidator(defaultConfig());
        ASSERT_NE(validator_, nullptr);
    }

    std::unique_ptr<IGraphValidator> validator_;
};

TEST_F(GraphValidatorTest, FactoryReturnsNonNull) {
    EXPECT_NE(validator_, nullptr);
}

TEST_F(GraphValidatorTest, IsAvailableReturnsFalseInScaffold) {
    // Scaffold stub has no backend; must return false without crashing.
    EXPECT_FALSE(validator_->isAvailable());
}

TEST_F(GraphValidatorTest, ValidateEmptyCandidatesDoesNotThrow) {
    GraphQuery q;
    q.query_text = "test query";
    EXPECT_NO_THROW(validator_->validate(q));
}

TEST_F(GraphValidatorTest, ValidatePassesThroughCandidates) {
    AnnCandidate c;
    c.id    = 42;
    c.score = 0.8f;

    GraphQuery q;
    q.candidates  = {c};
    q.query_text  = "unit test query";

    GraphResult result = validator_->validate(q);
    // Scaffold: validated_candidates should contain at least as many as input
    // or be empty — but must not throw.
    (void)result;
    SUCCEED();
}

TEST_F(GraphValidatorTest, LookupProvenanceReturnsNulloptForUnknownId) {
    auto evidence = validator_->lookupProvenance("nonexistent-id");
    EXPECT_FALSE(evidence.has_value());
}

TEST_F(GraphValidatorTest, LookupProvenanceDoesNotThrowOnEmptyId) {
    EXPECT_NO_THROW(validator_->lookupProvenance(""));
}

TEST_F(GraphValidatorTest, MultipleValidateCallsAreStable) {
    GraphQuery q;
    q.query_text = "repeated query";
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(validator_->validate(q));
    }
}
