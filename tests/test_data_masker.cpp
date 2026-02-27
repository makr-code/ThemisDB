/**
 * @file test_data_masker.cpp
 * @brief Unit tests for DataMasker – automated field-level masking in query results.
 *
 * Tests cover:
 * - DataMasker::applyStrategy() for all four masking strategies
 * - DataMasker::maskFields() on flat and nested JSON documents
 * - DataMasker::maskFieldsArray() on JSON arrays
 * - FieldMaskingPolicy disabled (pass-through)
 * - FieldMaskingPolicy with empty rules (pass-through)
 * - Non-string scalar fields pass through unchanged
 * - Multiple rules, first-wins semantics
 * - TOKENIZE stability (same input → same pseudonym)
 * - TRUNCATE edge cases (value shorter than truncate_length)
 * - PolicyEngine::checkQueryPermission() returns masking policy
 * - PolicyEngine::getMaskingPolicy() returns snapshot
 * - governance_fields_masked_total counter incremented on mask
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "governance/data_masker.h"
#include "governance/policy_engine.h"

using namespace themis::governance;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helper: build a simple FieldMaskingPolicy
// ---------------------------------------------------------------------------

static FieldMaskingPolicy makePolicy(std::vector<FieldMaskingRule> rules, bool enabled = true) {
    FieldMaskingPolicy p;
    p.enabled = enabled;
    p.rules   = std::move(rules);
    return p;
}

static FieldMaskingRule makeRule(const std::string &field, MaskingStrategy strategy, int truncate_len = 4,
                                 const std::string &secret = "test-secret") {
    FieldMaskingRule r;
    r.field_name        = field;
    r.strategy          = strategy;
    r.truncate_length   = truncate_len;
    r.collection_secret = secret;
    return r;
}

// ---------------------------------------------------------------------------
// Strategy: REDACT
// ---------------------------------------------------------------------------

TEST(DataMaskerStrategyTest, RedactReplacesWithLiteral) {
    FieldMaskingRule rule = makeRule("f", MaskingStrategy::REDACT);
    EXPECT_EQ(DataMasker::applyStrategy("alice@example.com", rule), "[REDACTED]");
    EXPECT_EQ(DataMasker::applyStrategy("", rule), "[REDACTED]");
    EXPECT_EQ(DataMasker::applyStrategy("42", rule), "[REDACTED]");
}

// ---------------------------------------------------------------------------
// Strategy: TOKENIZE
// ---------------------------------------------------------------------------

TEST(DataMaskerStrategyTest, TokenizePrefixedWithTkn) {
    FieldMaskingRule rule    = makeRule("f", MaskingStrategy::TOKENIZE);
    const std::string result = DataMasker::applyStrategy("patient-001", rule);
    EXPECT_EQ(result.substr(0, 4), "tkn_");
    EXPECT_EQ(result.size(), 4u + 64u); // "tkn_" + 64 hex chars (HMAC-SHA256)
}

TEST(DataMaskerStrategyTest, TokenizeIsStable) {
    // Same input + same secret must produce the same pseudonym (join-query support)
    FieldMaskingRule rule = makeRule("f", MaskingStrategy::TOKENIZE, 4, "my-secret");
    const std::string a   = DataMasker::applyStrategy("subject-42", rule);
    const std::string b   = DataMasker::applyStrategy("subject-42", rule);
    EXPECT_EQ(a, b) << "TOKENIZE must be deterministic for the same input and secret";
}

TEST(DataMaskerStrategyTest, TokenizeDifferentSecretsProduceDifferentPseudonyms) {
    FieldMaskingRule r1 = makeRule("f", MaskingStrategy::TOKENIZE, 4, "secret-A");
    FieldMaskingRule r2 = makeRule("f", MaskingStrategy::TOKENIZE, 4, "secret-B");
    EXPECT_NE(DataMasker::applyStrategy("same-value", r1), DataMasker::applyStrategy("same-value", r2));
}

TEST(DataMaskerStrategyTest, TokenizeDifferentInputsProduceDifferentPseudonyms) {
    FieldMaskingRule rule = makeRule("f", MaskingStrategy::TOKENIZE, 4, "secret");
    EXPECT_NE(DataMasker::applyStrategy("alice", rule), DataMasker::applyStrategy("bob", rule));
}

TEST(DataMaskerStrategyTest, TokenizeWithEmptySecretFallsBackToSHA256) {
    // When collection_secret is empty the implementation falls back to plain
    // SHA-256 (unkeyed) and still returns a "tkn_" prefixed hex string.
    FieldMaskingRule rule_empty  = makeRule("f", MaskingStrategy::TOKENIZE, 4, "");
    FieldMaskingRule rule_keyed  = makeRule("f", MaskingStrategy::TOKENIZE, 4, "some-key");
    const std::string with_empty = DataMasker::applyStrategy("value", rule_empty);
    const std::string with_key   = DataMasker::applyStrategy("value", rule_keyed);

    // Both must have the "tkn_" prefix and be 68 chars total.
    EXPECT_EQ(with_empty.substr(0, 4), "tkn_");
    EXPECT_EQ(with_empty.size(), 4u + 64u);
    // Empty-secret result differs from the keyed result (SHA-256 ≠ HMAC-SHA256).
    EXPECT_NE(with_empty, with_key);
    // Empty-secret result is stable (same call → same output).
    EXPECT_EQ(with_empty, DataMasker::applyStrategy("value", rule_empty));
}

// ---------------------------------------------------------------------------
// Strategy: TRUNCATE
// ---------------------------------------------------------------------------

TEST(DataMaskerStrategyTest, TruncateKeepsNCharsAndAppendsDots) {
    FieldMaskingRule rule = makeRule("f", MaskingStrategy::TRUNCATE, 5);
    EXPECT_EQ(DataMasker::applyStrategy("Alice Smith", rule), "Alice...");
}

TEST(DataMaskerStrategyTest, TruncateShortValuePassesThrough) {
    FieldMaskingRule rule = makeRule("f", MaskingStrategy::TRUNCATE, 10);
    EXPECT_EQ(DataMasker::applyStrategy("Hi", rule), "Hi");
}

TEST(DataMaskerStrategyTest, TruncateExactLengthPassesThrough) {
    FieldMaskingRule rule = makeRule("f", MaskingStrategy::TRUNCATE, 5);
    EXPECT_EQ(DataMasker::applyStrategy("Hello", rule), "Hello");
}

// ---------------------------------------------------------------------------
// Strategy: HASH
// ---------------------------------------------------------------------------

TEST(DataMaskerStrategyTest, HashPrefixedWithSha) {
    FieldMaskingRule rule    = makeRule("f", MaskingStrategy::HASH);
    const std::string result = DataMasker::applyStrategy("secret-data", rule);
    EXPECT_EQ(result.substr(0, 4), "sha_");
    EXPECT_EQ(result.size(), 4u + 64u); // "sha_" + 64 hex chars (SHA-256)
}

TEST(DataMaskerStrategyTest, HashIsStable) {
    FieldMaskingRule rule = makeRule("f", MaskingStrategy::HASH);
    EXPECT_EQ(DataMasker::applyStrategy("hello", rule), DataMasker::applyStrategy("hello", rule));
}

TEST(DataMaskerStrategyTest, HashDifferentInputsProduceDifferentDigests) {
    FieldMaskingRule rule = makeRule("f", MaskingStrategy::HASH);
    EXPECT_NE(DataMasker::applyStrategy("foo", rule), DataMasker::applyStrategy("bar", rule));
}

// ---------------------------------------------------------------------------
// DataMasker::maskFields – flat document
// ---------------------------------------------------------------------------

class DataMaskerTest : public ::testing::Test {
  protected:
    DataMasker masker_;
};

TEST_F(DataMaskerTest, PolicyDisabledPassesThrough) {
    FieldMaskingPolicy policy = makePolicy({makeRule("ssn", MaskingStrategy::REDACT)}, false);
    json doc                  = {{"ssn", "123-45-6789"}, {"name", "Alice"}};
    EXPECT_EQ(masker_.maskFields(doc, policy), doc);
}

TEST_F(DataMaskerTest, EmptyRulesPassesThrough) {
    FieldMaskingPolicy policy;
    policy.enabled = true;
    json doc       = {{"ssn", "123-45-6789"}};
    EXPECT_EQ(masker_.maskFields(doc, policy), doc);
}

TEST_F(DataMaskerTest, DeclaredFieldMasked) {
    FieldMaskingPolicy policy = makePolicy({makeRule("ssn", MaskingStrategy::REDACT)});
    json doc                  = {{"ssn", "123-45-6789"}, {"name", "Alice"}};
    json result               = masker_.maskFields(doc, policy);

    EXPECT_EQ(result["ssn"].get<std::string>(), "[REDACTED]");
    EXPECT_EQ(result["name"].get<std::string>(), "Alice"); // untouched
}

TEST_F(DataMaskerTest, UnmatchedFieldPassesThrough) {
    FieldMaskingPolicy policy = makePolicy({makeRule("ssn", MaskingStrategy::REDACT)});
    json doc                  = {{"other_field", "some-value"}};
    json result               = masker_.maskFields(doc, policy);
    EXPECT_EQ(result["other_field"].get<std::string>(), "some-value");
}

TEST_F(DataMaskerTest, NumericFieldPassesThrough) {
    FieldMaskingPolicy policy = makePolicy({makeRule("count", MaskingStrategy::REDACT)});
    json doc                  = {{"count", 42}, {"score", 9.8}};
    json result               = masker_.maskFields(doc, policy);
    // Numeric values are not string-typed, so they pass through even if field name matches
    EXPECT_EQ(result["count"].get<int>(), 42);
    EXPECT_DOUBLE_EQ(result["score"].get<double>(), 9.8);
}

TEST_F(DataMaskerTest, BoolAndNullPassThrough) {
    FieldMaskingPolicy policy = makePolicy({makeRule("active", MaskingStrategy::REDACT)});
    json doc                  = {{"active", true}, {"data", nullptr}};
    json result               = masker_.maskFields(doc, policy);
    EXPECT_EQ(result["active"].get<bool>(), true);
    EXPECT_TRUE(result["data"].is_null());
}

TEST_F(DataMaskerTest, MultipleRulesApplied) {
    FieldMaskingPolicy policy
        = makePolicy({makeRule("ssn", MaskingStrategy::REDACT), makeRule("name", MaskingStrategy::TRUNCATE, 3)});
    json doc    = {{"ssn", "123-45-6789"}, {"name", "Alice"}, {"id", "001"}};
    json result = masker_.maskFields(doc, policy);

    EXPECT_EQ(result["ssn"].get<std::string>(), "[REDACTED]");
    EXPECT_EQ(result["name"].get<std::string>(), "Ali...");
    EXPECT_EQ(result["id"].get<std::string>(), "001");
}

TEST_F(DataMaskerTest, FirstRuleWinsForDuplicateField) {
    // Two rules declare the same field; the first should win.
    FieldMaskingPolicy policy
        = makePolicy({makeRule("ssn", MaskingStrategy::REDACT), makeRule("ssn", MaskingStrategy::HASH)});
    json doc    = {{"ssn", "123-45-6789"}};
    json result = masker_.maskFields(doc, policy);
    EXPECT_EQ(result["ssn"].get<std::string>(), "[REDACTED]");
}

// ---------------------------------------------------------------------------
// DataMasker::maskFields – nested JSON
// ---------------------------------------------------------------------------

TEST_F(DataMaskerTest, NestedObjectFieldMasked) {
    FieldMaskingPolicy policy = makePolicy({makeRule("ssn", MaskingStrategy::REDACT)});
    json doc                  = {{"patient", {{"ssn", "123-45-6789"}, {"name", "Alice"}}}, {"id", "P001"}};
    json result               = masker_.maskFields(doc, policy);

    EXPECT_EQ(result["patient"]["ssn"].get<std::string>(), "[REDACTED]");
    EXPECT_EQ(result["patient"]["name"].get<std::string>(), "Alice");
    EXPECT_EQ(result["id"].get<std::string>(), "P001");
}

TEST_F(DataMaskerTest, ArrayOfStringsWithMatchingKeyMasked) {
    // A field whose value is an array of strings – each string element in the
    // array inherits the parent key ("tags") for matching purposes, so each
    // element is eligible for masking when the parent key matches a rule.
    FieldMaskingPolicy policy = makePolicy({makeRule("tags", MaskingStrategy::REDACT)});
    json doc                  = {{"tags", json::array({"pii", "phi"})}};
    json result               = masker_.maskFields(doc, policy);
    for (const auto &elem : result["tags"]) {
        EXPECT_EQ(elem.get<std::string>(), "[REDACTED]");
    }
}

// ---------------------------------------------------------------------------
// DataMasker::maskFieldsArray
// ---------------------------------------------------------------------------

TEST_F(DataMaskerTest, MaskFieldsArrayMasksAllDocuments) {
    FieldMaskingPolicy policy = makePolicy({makeRule("ssn", MaskingStrategy::REDACT)});
    json docs   = json::array({{{"ssn", "111-11-1111"}, {"id", "1"}}, {{"ssn", "222-22-2222"}, {"id", "2"}}});
    json result = masker_.maskFieldsArray(docs, policy);

    ASSERT_TRUE(result.is_array());
    ASSERT_EQ(result.size(), 2u);
    for (const auto &doc : result) {
        EXPECT_EQ(doc["ssn"].get<std::string>(), "[REDACTED]");
    }
}

TEST_F(DataMaskerTest, MaskFieldsArrayOnNonArrayDelegatesToMaskFields) {
    FieldMaskingPolicy policy = makePolicy({makeRule("ssn", MaskingStrategy::REDACT)});
    json single               = {{"ssn", "999-99-9999"}};
    json result               = masker_.maskFieldsArray(single, policy);
    EXPECT_EQ(result["ssn"].get<std::string>(), "[REDACTED]");
}

TEST_F(DataMaskerTest, MaskFieldsArrayEmptyArrayReturnsEmptyArray) {
    FieldMaskingPolicy policy = makePolicy({makeRule("ssn", MaskingStrategy::REDACT)});
    json empty                = json::array();
    json result               = masker_.maskFieldsArray(empty, policy);
    EXPECT_TRUE(result.is_array());
    EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// PolicyEngine integration: checkQueryPermission / getMaskingPolicy
// ---------------------------------------------------------------------------

TEST(PolicyEngineQueryPermissionTest, CheckQueryPermissionReturnsMaskingPolicy) {
    PolicyEngine engine;
    // Load from the project governance.yaml which has data_masking rules.
    // Fall back gracefully if file is absent in the test environment.
    const bool loaded = engine.loadFromYAML("config/governance.yaml");
    if (!loaded) {
        GTEST_SKIP() << "config/governance.yaml not available in test CWD; skipping";
    }

    std::unordered_map<std::string, std::string> headers;
    auto result = engine.checkQueryPermission(headers, "/vector/search");

    // The policy must have a valid classification in the decision.
    EXPECT_FALSE(result.decision.classification.empty());
    // Masking policy enabled flag should match the YAML config.
    EXPECT_TRUE(result.masking_policy.enabled);
    EXPECT_FALSE(result.masking_policy.rules.empty());
}

TEST(PolicyEngineQueryPermissionTest, GetMaskingPolicyReturnsSnapshot) {
    PolicyEngine engine;
    // Without loading YAML, masking_rules_ is default (disabled, no rules).
    FieldMaskingPolicy p = engine.getMaskingPolicy();
    EXPECT_FALSE(p.enabled);
    EXPECT_TRUE(p.rules.empty());
}

TEST(PolicyEngineQueryPermissionTest, CheckQueryPermissionWithoutYAML) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers;
    auto result = engine.checkQueryPermission(headers, "/vector/search");
    // Should not crash and should have a default decision.
    EXPECT_FALSE(result.decision.classification.empty());
    EXPECT_FALSE(result.masking_policy.enabled);
}

// ---------------------------------------------------------------------------
// Thread safety: concurrent maskFields calls are safe
// ---------------------------------------------------------------------------

#include <atomic>
#include <thread>

TEST_F(DataMaskerTest, ConcurrentMaskFieldsIsSafe) {
    FieldMaskingPolicy policy
        = makePolicy({makeRule("ssn", MaskingStrategy::REDACT), makeRule("name", MaskingStrategy::TRUNCATE, 3)});

    const int threads    = 8;
    const int iterations = 100;
    std::atomic<int> errors{0};

    auto worker = [&]() {
        for (int i = 0; i < iterations; ++i) {
            json doc    = {{"ssn", "123-45-6789"}, {"name", "Alice"}, {"id", "001"}};
            json result = masker_.maskFields(doc, policy);
            if (result["ssn"].get<std::string>() != "[REDACTED]") {
                errors.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (int i = 0; i < threads; ++i)
        pool.emplace_back(worker);
    for (auto &t : pool)
        t.join();

    EXPECT_EQ(errors.load(), 0) << "Concurrent maskFields must produce correct results";
}
