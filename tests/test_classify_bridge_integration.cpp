/**
 * @file test_classify_bridge_integration.cpp
 * @brief Integration tests for the IClassifyFn / AQLFunctionClassifyBridge wiring.
 *
 * Acceptance criteria verified here:
 *   AC-1  IClassifyFn interface and NullClassifyFn compile and behave as no-ops.
 *   AC-2  setClassifier() wires a classifier into DocsAssistantFunctions.
 *   AC-3  detectIntentWithNativeNLP returns "unknown" when no classifier injected.
 *   AC-4  detectIntentWithNativeNLP delegates to an injected classifier.
 *   AC-5  AQLFunctionClassifyBridge returns "configuration" with confidence > 0.7
 *         for the query "how do I create an index?".
 *   AC-6  NullClassifyFn returns an empty ClassifyResult (category = "", confidence = 0).
 *   AC-7  Classifier can be reset to nullptr (reverts to no-op behaviour).
 */

#include <gtest/gtest.h>

#include "aql/classify_bridge.h"
#include "aql/docs_assistant_functions.h"

using namespace themis::aql;

// Tolerance for softmax sum-to-one verification.
static constexpr double kSoftmaxSumTolerance = 0.01;

// ============================================================================
// Helpers / test doubles
// ============================================================================

/**
 * @brief Deterministic stub that always classifies as "configuration" with
 *        confidence 0.99, regardless of the input.
 */
class StubClassifyFn final : public IClassifyFn {
public:
    ClassifyResult classify(const std::string& /*text*/,
                            const std::vector<std::string>& categories) const override
    {
        ClassifyResult r;
        r.category   = "configuration";
        r.confidence = 0.99;
        for (const auto& c : categories) {
            r.scores[c] = (c == "configuration") ? 0.99 : 0.003;
        }
        return r;
    }
};

/**
 * @brief Thin subclass of DocsAssistantFunctions that exposes the protected
 *        detectIntentWithNativeNLP() method for direct testing.
 */
class TestableDocsAssistant : public DocsAssistantFunctions {
public:
    std::string callDetectIntent(const std::string& query) {
        return detectIntentWithNativeNLP(query);
    }
};

// ============================================================================
// AC-1  NullClassifyFn is a well-behaved no-op
// ============================================================================

TEST(ClassifyBridgeIntegrationTest, NullClassifyFnReturnsEmptyResult) {
    NullClassifyFn fn;
    auto result = fn.classify("how do I create an index?",
                              {"configuration", "troubleshooting", "search", "general"});
    EXPECT_TRUE(result.category.empty());
    EXPECT_DOUBLE_EQ(result.confidence, 0.0);
    EXPECT_TRUE(result.scores.empty());
}

// ============================================================================
// AC-2 / AC-3  No classifier → detectIntentWithNativeNLP returns "unknown"
// ============================================================================

TEST(ClassifyBridgeIntegrationTest, NoClassifierReturnsUnknown) {
    TestableDocsAssistant assistant;
    // No classifier set → should return "unknown".
    EXPECT_EQ(assistant.callDetectIntent("how do I create an index?"), "unknown");
}

// ============================================================================
// AC-2 / AC-4  Injecting a stub classifier is honoured
// ============================================================================

TEST(ClassifyBridgeIntegrationTest, InjectedClassifierIsUsed) {
    TestableDocsAssistant assistant;
    StubClassifyFn stub;

    assistant.setClassifier(&stub);
    EXPECT_EQ(assistant.callDetectIntent("any query at all"), "configuration");
}

// ============================================================================
// AC-5  AQLFunctionClassifyBridge: "how do I create an index?" → "configuration"
//        with confidence > 0.7
// ============================================================================

TEST(ClassifyBridgeIntegrationTest, BridgeClassifiesIndexQueryAsConfiguration) {
    AQLFunctionClassifyBridge bridge;
    auto result = bridge.classify(
        "how do I create an index?",
        {"configuration", "troubleshooting", "search", "general"}
    );

    EXPECT_EQ(result.category, "configuration")
        << "Expected category 'configuration', got '" << result.category << "'";
    EXPECT_GT(result.confidence, 0.7)
        << "Expected confidence > 0.7, got " << result.confidence;
}

// ============================================================================
// AC-5 (end-to-end)  Wired bridge → detectIntentWithNativeNLP returns
//       "configuration" for an index-creation query
// ============================================================================

TEST(ClassifyBridgeIntegrationTest, WiredBridgeDetectsConfigurationIntent) {
    TestableDocsAssistant assistant;
    AQLFunctionClassifyBridge bridge;

    assistant.setClassifier(&bridge);

    std::string intent = assistant.callDetectIntent("how do I create an index?");
    EXPECT_EQ(intent, "configuration")
        << "detectIntentWithNativeNLP should return 'configuration' when bridge is wired";
}

// ============================================================================
// AC-6  NullClassifyFn causes "unknown" to be returned
// ============================================================================

TEST(ClassifyBridgeIntegrationTest, NullClassifyFnGivesUnknown) {
    TestableDocsAssistant assistant;
    NullClassifyFn null_fn;

    assistant.setClassifier(&null_fn);
    // NullClassifyFn returns empty category → should produce "unknown".
    EXPECT_EQ(assistant.callDetectIntent("configure sharding"), "unknown");
}

// ============================================================================
// AC-7  Resetting classifier to nullptr reverts to no-op
// ============================================================================

TEST(ClassifyBridgeIntegrationTest, ResettingClassifierReturnsUnknown) {
    TestableDocsAssistant assistant;
    StubClassifyFn stub;

    assistant.setClassifier(&stub);
    EXPECT_EQ(assistant.callDetectIntent("some query"), "configuration");

    assistant.setClassifier(nullptr);
    EXPECT_EQ(assistant.callDetectIntent("some query"), "unknown");
}

// ============================================================================
// Invalid label from classifier is rejected
// ============================================================================

/**
 * @brief Stub that always returns a category label outside the expected set.
 */
class BadLabelClassifyFn final : public IClassifyFn {
public:
    ClassifyResult classify(const std::string& /*text*/,
                            const std::vector<std::string>& /*categories*/) const override
    {
        ClassifyResult r;
        r.category   = "unexpected_label";
        r.confidence = 0.99;
        return r;
    }
};

TEST(ClassifyBridgeIntegrationTest, InvalidLabelFromClassifierIsRejected) {
    TestableDocsAssistant assistant;
    BadLabelClassifyFn bad;

    assistant.setClassifier(&bad);
    // Unknown label must be rejected → "unknown" returned so LLM path is used.
    EXPECT_EQ(assistant.callDetectIntent("how do I create an index?"), "unknown");
}

// ============================================================================
// Additional bridge smoke-tests
// ============================================================================

TEST(ClassifyBridgeIntegrationTest, BridgeClassifiesErrorQueryAsTroubleshooting) {
    AQLFunctionClassifyBridge bridge;
    auto result = bridge.classify(
        "server crashes on startup with segfault error",
        {"configuration", "troubleshooting", "search", "general"}
    );
    EXPECT_EQ(result.category, "troubleshooting");
    EXPECT_GT(result.confidence, 0.5);
}

TEST(ClassifyBridgeIntegrationTest, BridgeClassifiesSearchQuery) {
    AQLFunctionClassifyBridge bridge;
    auto result = bridge.classify(
        "search for RAID documentation",
        {"configuration", "troubleshooting", "search", "general"}
    );
    EXPECT_EQ(result.category, "search");
    EXPECT_GT(result.confidence, 0.5);
}

TEST(ClassifyBridgeIntegrationTest, BridgeHandlesEmptyCategoryList) {
    AQLFunctionClassifyBridge bridge;
    auto result = bridge.classify("any text", {});
    EXPECT_TRUE(result.category.empty());
}

TEST(ClassifyBridgeIntegrationTest, BridgeReturnsEmptyResultWhenNoKeywordsMatch) {
    // A query with no recognisable keywords should return an empty ClassifyResult
    // so that detectIntentWithNativeNLP() falls through to the LLM/regex path.
    AQLFunctionClassifyBridge bridge;
    auto result = bridge.classify(
        "xyzzyx qrstu 12345",
        {"configuration", "troubleshooting", "search", "general"}
    );
    EXPECT_TRUE(result.category.empty())
        << "No-signal query should produce empty category, got '" << result.category << "'";
    EXPECT_DOUBLE_EQ(result.confidence, 0.0);
}

TEST(ClassifyBridgeIntegrationTest, BridgeReturnedScoresSumToApproximatelyOne) {
    AQLFunctionClassifyBridge bridge;
    const std::vector<std::string> cats = {
        "configuration", "troubleshooting", "search", "general"
    };
    auto result = bridge.classify("how do I create an index?", cats);

    double total = 0.0;
    for (const auto& c : cats) {
        auto it = result.scores.find(c);
        if (it != result.scores.end()) total += it->second;
    }
    EXPECT_NEAR(total, 1.0, kSoftmaxSumTolerance) << "Softmax scores should sum to ~1";
}
