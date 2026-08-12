/**
 * @file test_prompt_engineering_phase6.cpp
 * @brief Unit tests for prompt_engineering Phase 6 components (v1.8.0):
 *   - StructuredOutputEnforcer  (SOE-01 .. SOE-08)
 *   - SimplePromptCompressor    (PCM-01 .. PCM-08)
 *   - SimpleAdversarialTester   (APT-01 .. APT-06)
 *
 * Acceptance criteria (all must pass):
 *   SOE-01  NONE constraint: enforce() always returns is_valid=true, 0 attempts.
 *   SOE-02  JSON_SCHEMA: valid JSON with required fields passes.
 *   SOE-03  JSON_SCHEMA: missing required field produces validation_errors.
 *   SOE-04  JSON_SCHEMA: Markdown fences are stripped before validation.
 *   SOE-05  JSON_SCHEMA: trailing comma is repaired and then passes.
 *   SOE-06  JSON_SCHEMA: strict_mode rejects unknown keys.
 *   SOE-07  REGEX constraint: matching output passes; non-matching fails.
 *   SOE-08  REGEX: invalid regex pattern populates validation_errors gracefully.
 *
 *   PCM-01  No compression needed if prompt is within budget.
 *   PCM-02  TRUNCATE_TAIL removes trailing words until within budget.
 *   PCM-03  TRUNCATE_HEAD removes leading words until within budget.
 *   PCM-04  SELECTIVE_TRIM keeps system prompt and last N turns.
 *   PCM-05  SUMMARY strategy inserts placeholder summary text.
 *   PCM-06  Custom summary function is called via setSummaryFn().
 *   PCM-07  EMBEDDING_PRUNE falls back to SELECTIVE_TRIM.
 *   PCM-08  supportedStrategies() includes all 5 strategies.
 *
 *   APT-01  loadDefaultTestSuite() populates ≥ 10 test cases.
 *   APT-02  runAll() returns correct total / blocked_count / passed_count.
 *   APT-03  Known attack payload is blocked by the default detector.
 *   APT-04  Benign payload is NOT blocked by the default detector.
 *   APT-05  addTestCase() rejects duplicate ids with std::invalid_argument.
 *   APT-06  Custom detector function is invoked via setDetectorFn().
 */

#include "prompt_engineering/structured_output.h"
#include "prompt_engineering/prompt_compressor.h"
#include "prompt_engineering/adversarial_prompt_tester.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <string>

using namespace themis::prompt_engineering;

// ─────────────────────────────────────────────────────────────────────────────
// StructuredOutputEnforcer tests
// ─────────────────────────────────────────────────────────────────────────────

// SOE-01: NONE constraint always passes.
TEST(StructuredOutputEnforcerTest, SOE01_NoneConstraintAlwaysPasses) {
    StructuredOutputEnforcer enforcer;
    StructuredOutputConfig cfg;
    cfg.type = OutputConstraintType::NONE;

    auto result = enforcer.enforce("any raw text here", cfg);
    EXPECT_TRUE(result.is_valid);
    EXPECT_EQ(result.attempts_used, 0);
    EXPECT_TRUE(result.validation_errors.empty());
}

// SOE-02: Valid JSON with required fields passes.
TEST(StructuredOutputEnforcerTest, SOE02_ValidJsonWithRequiredFields) {
    StructuredOutputEnforcer enforcer;
    StructuredOutputConfig cfg;
    cfg.type = OutputConstraintType::JSON_SCHEMA;
    cfg.json_schema.schema_json = R"({"required":["name","age"],"properties":{"name":{},"age":{}}})";
    cfg.json_schema.strict_mode = true;

    auto result = enforcer.enforce(R"({"name":"Alice","age":30})", cfg);
    EXPECT_TRUE(result.is_valid) << "Errors: "
        << (result.validation_errors.empty() ? "" : result.validation_errors[0]);
    EXPECT_TRUE(result.validation_errors.empty());
}

// SOE-03: Missing required field produces validation_errors.
TEST(StructuredOutputEnforcerTest, SOE03_MissingRequiredField) {
    StructuredOutputEnforcer enforcer;
    StructuredOutputConfig cfg;
    cfg.type = OutputConstraintType::JSON_SCHEMA;
    cfg.json_schema.schema_json = R"({"required":["name","email"]})";
    cfg.json_schema.strict_mode = false;

    auto result = enforcer.enforce(R"({"name":"Bob"})", cfg);
    EXPECT_FALSE(result.is_valid);
    ASSERT_FALSE(result.validation_errors.empty());
    const bool has_email_error = std::any_of(
        result.validation_errors.begin(), result.validation_errors.end(),
        [](const std::string& e) { return e.find("email") != std::string::npos; });
    EXPECT_TRUE(has_email_error) << "Expected 'email' in errors";
}

// SOE-04: Markdown fences are stripped before validation.
TEST(StructuredOutputEnforcerTest, SOE04_MarkdownFencesStripped) {
    StructuredOutputEnforcer enforcer;
    StructuredOutputConfig cfg;
    cfg.type = OutputConstraintType::JSON_SCHEMA;
    cfg.json_schema.schema_json = R"({"required":["status"]})";
    cfg.strip_markdown = true;
    cfg.repair_json    = true;

    const std::string fenced = "```json\n{\"status\":\"ok\"}\n```";
    auto result = enforcer.enforce(fenced, cfg);
    EXPECT_TRUE(result.is_valid) << "Errors: "
        << (result.validation_errors.empty() ? "" : result.validation_errors[0]);
}

// SOE-05: Trailing comma is repaired.
TEST(StructuredOutputEnforcerTest, SOE05_TrailingCommaRepaired) {
    StructuredOutputEnforcer enforcer;
    StructuredOutputConfig cfg;
    cfg.type = OutputConstraintType::JSON_SCHEMA;
    cfg.json_schema.schema_json = R"({"required":["key"]})";
    cfg.repair_json = true;

    const std::string bad_json = R"({"key":"value",})";
    auto result = enforcer.enforce(bad_json, cfg);
    EXPECT_TRUE(result.is_valid) << "Errors: "
        << (result.validation_errors.empty() ? "" : result.validation_errors[0]);
}

// SOE-06: strict_mode rejects unknown keys.
TEST(StructuredOutputEnforcerTest, SOE06_StrictModeRejectsUnknownKeys) {
    StructuredOutputEnforcer enforcer;
    StructuredOutputConfig cfg;
    cfg.type = OutputConstraintType::JSON_SCHEMA;
    cfg.json_schema.schema_json =
        R"({"required":["id"],"properties":{"id":{}}})";
    cfg.json_schema.strict_mode = true;
    cfg.repair_json = false;

    // "extra" is not in properties → should fail strict mode
    auto result = enforcer.enforce(R"({"id":1,"extra":"unexpected"})", cfg);
    EXPECT_FALSE(result.is_valid);
    const bool has_unknown = std::any_of(
        result.validation_errors.begin(), result.validation_errors.end(),
        [](const std::string& e) {
            return e.find("extra") != std::string::npos ||
                   e.find("Unknown") != std::string::npos;
        });
    EXPECT_TRUE(has_unknown);
}

// SOE-07: REGEX — matching passes; non-matching fails.
TEST(StructuredOutputEnforcerTest, SOE07_RegexConstraint) {
    StructuredOutputEnforcer enforcer;
    StructuredOutputConfig cfg;
    cfg.type = OutputConstraintType::REGEX;
    cfg.regex_grammar.pattern    = R"(^\d{4}-\d{2}-\d{2}$)";
    cfg.regex_grammar.full_match = true;

    auto pass_result = enforcer.enforce("2026-04-19", cfg);
    EXPECT_TRUE(pass_result.is_valid);

    auto fail_result = enforcer.enforce("not-a-date", cfg);
    EXPECT_FALSE(fail_result.is_valid);
    EXPECT_FALSE(fail_result.validation_errors.empty());
}

// SOE-08: Invalid regex pattern → validation_errors, not exception.
TEST(StructuredOutputEnforcerTest, SOE08_InvalidRegexHandledGracefully) {
    StructuredOutputEnforcer enforcer;
    StructuredOutputConfig cfg;
    cfg.type = OutputConstraintType::REGEX;
    cfg.regex_grammar.pattern = R"([invalid(regex)";  // intentionally broken

    EXPECT_NO_THROW({
        auto result = enforcer.enforce("anything", cfg);
        EXPECT_FALSE(result.is_valid);
        EXPECT_FALSE(result.validation_errors.empty());
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// SimplePromptCompressor tests
// ─────────────────────────────────────────────────────────────────────────────

namespace {
// Helper: build a prompt with approximately N words.
std::string makeWords(int n, const std::string& word = "token") {
    std::string result;
    for (int i = 0; i < n; ++i) {
        if (i > 0) result += ' ';
        result += word;
    }
    return result;
}
}  // namespace

// PCM-01: No compression when already within budget.
TEST(SimplePromptCompressorTest, PCM01_NoBudgetExceeded) {
    SimplePromptCompressor compressor;
    PromptCompressionConfig cfg;
    cfg.strategy            = CompressionStrategy::TRUNCATE_TAIL;
    cfg.target_token_budget = 10000;

    const std::string prompt = "Short prompt.";
    auto result = compressor.compress(prompt, cfg);

    EXPECT_EQ(result.compressed_prompt, prompt);
    EXPECT_FLOAT_EQ(result.compression_ratio, 0.0f);
}

// PCM-02: TRUNCATE_TAIL produces a shorter output.
TEST(SimplePromptCompressorTest, PCM02_TruncateTail) {
    SimplePromptCompressor compressor;

    // Build a prompt with many words so it definitely exceeds a small budget.
    const std::string prompt = makeWords(2000, "wordtoken");  // ~18000 chars
    PromptCompressionConfig cfg;
    cfg.strategy            = CompressionStrategy::TRUNCATE_TAIL;
    cfg.target_token_budget = 50;   // very small to force compression

    auto result = compressor.compress(prompt, cfg);

    EXPECT_LT(result.compressed_token_count, result.original_token_count);
    EXPECT_GT(result.compression_ratio, 0.0f);
    // Compressed prompt should not start with the same words as the end.
    EXPECT_FALSE(result.compressed_prompt.empty());
}

// PCM-03: TRUNCATE_HEAD removes from the front.
TEST(SimplePromptCompressorTest, PCM03_TruncateHead) {
    SimplePromptCompressor compressor;

    const std::string prompt = "FIRST " + makeWords(2000, "mid") + " LAST";
    PromptCompressionConfig cfg;
    cfg.strategy            = CompressionStrategy::TRUNCATE_HEAD;
    cfg.target_token_budget = 50;

    auto result = compressor.compress(prompt, cfg);

    EXPECT_LT(result.compressed_token_count, result.original_token_count);
    // Result should contain "LAST" (end of prompt) but not "FIRST".
    EXPECT_EQ(result.compressed_prompt.find("FIRST"), std::string::npos);
}

// PCM-04: SELECTIVE_TRIM keeps system prompt and last N turns.
TEST(SimplePromptCompressorTest, PCM04_SelectiveTrimPreservesSystemAndTail) {
    SimplePromptCompressor compressor;

    const std::string system_block = "SYSTEM_PROMPT: you are a helpful assistant.";
    const std::string middle       = makeWords(2000, "mid");
    const std::string tail_turn    = "USER_LAST_TURN: what is 2+2?";

    const std::string prompt =
        system_block + "\n\n" + middle + "\n\n" + tail_turn;

    PromptCompressionConfig cfg;
    cfg.strategy              = CompressionStrategy::SELECTIVE_TRIM;
    cfg.target_token_budget   = 50;
    cfg.preserve_system_prompt = true;
    cfg.preserve_last_n_turns  = 1;

    auto result = compressor.compress(prompt, cfg);

    EXPECT_NE(result.compressed_prompt.find("SYSTEM_PROMPT"), std::string::npos)
        << "System prompt should be preserved";
    EXPECT_NE(result.compressed_prompt.find("USER_LAST_TURN"), std::string::npos)
        << "Last turn should be preserved";
    EXPECT_LT(result.compressed_token_count, result.original_token_count);
}

// PCM-05: SUMMARY inserts placeholder text.
TEST(SimplePromptCompressorTest, PCM05_SummaryPlaceholderInserted) {
    SimplePromptCompressor compressor;

    const std::string prompt =
        "SYSTEM: assistant.\n\n" +
        makeWords(1000, "bodyword") +
        "\n\nUSER: final question.";

    PromptCompressionConfig cfg;
    cfg.strategy              = CompressionStrategy::SUMMARY;
    cfg.target_token_budget   = 50;
    cfg.preserve_system_prompt = true;
    cfg.preserve_last_n_turns  = 1;

    auto result = compressor.compress(prompt, cfg);

    // Default summary fn inserts "…summary of … omitted tokens…"
    EXPECT_NE(result.compressed_prompt.find("summary"), std::string::npos)
        << "Expected summary placeholder in output";
}

// PCM-06: Custom setSummaryFn() is called.
TEST(SimplePromptCompressorTest, PCM06_CustomSummaryFn) {
    SimplePromptCompressor compressor;

    bool called = false;
    compressor.setSummaryFn([&called](const std::string& /*text*/,
                                       const std::string& /*model*/) {
        called = true;
        return "[CUSTOM_SUMMARY]";
    });

    const std::string prompt =
        "SYS: ok.\n\n" + makeWords(1000, "w") + "\n\nUSER: hi.";

    PromptCompressionConfig cfg;
    cfg.strategy              = CompressionStrategy::SUMMARY;
    cfg.target_token_budget   = 20;
    cfg.preserve_system_prompt = true;
    cfg.preserve_last_n_turns  = 1;

    auto result = compressor.compress(prompt, cfg);

    EXPECT_TRUE(called) << "Custom summary function was not called";
    EXPECT_NE(result.compressed_prompt.find("[CUSTOM_SUMMARY]"), std::string::npos);
}

// PCM-07: EMBEDDING_PRUNE falls back to SELECTIVE_TRIM.
TEST(SimplePromptCompressorTest, PCM07_EmbeddingPruneFallback) {
    SimplePromptCompressor compressor;

    const std::string system_block = "SYSTEM: helper.";
    const std::string middle       = makeWords(2000, "word");
    const std::string last_turn    = "USER_LAST: question?";
    const std::string prompt =
        system_block + "\n\n" + middle + "\n\n" + last_turn;

    PromptCompressionConfig cfg;
    cfg.strategy              = CompressionStrategy::EMBEDDING_PRUNE;
    cfg.target_token_budget   = 50;
    cfg.preserve_system_prompt = true;
    cfg.preserve_last_n_turns  = 1;

    auto result = compressor.compress(prompt, cfg);

    // Strategy reported should be SELECTIVE_TRIM (the fallback).
    EXPECT_EQ(result.strategy_used, CompressionStrategy::SELECTIVE_TRIM);
    EXPECT_LT(result.compressed_token_count, result.original_token_count);
}

// PCM-08: supportedStrategies() includes all 5 strategies.
TEST(SimplePromptCompressorTest, PCM08_SupportedStrategiesComplete) {
    SimplePromptCompressor compressor;
    auto strategies = compressor.supportedStrategies();
    EXPECT_EQ(strategies.size(), 5u);

    const auto has = [&](CompressionStrategy s) {
        return std::find(strategies.begin(), strategies.end(), s) != strategies.end();
    };
    EXPECT_TRUE(has(CompressionStrategy::TRUNCATE_HEAD));
    EXPECT_TRUE(has(CompressionStrategy::TRUNCATE_TAIL));
    EXPECT_TRUE(has(CompressionStrategy::SELECTIVE_TRIM));
    EXPECT_TRUE(has(CompressionStrategy::SUMMARY));
    EXPECT_TRUE(has(CompressionStrategy::EMBEDDING_PRUNE));
}

// ─────────────────────────────────────────────────────────────────────────────
// SimpleAdversarialTester tests
// ─────────────────────────────────────────────────────────────────────────────

// APT-01: loadDefaultTestSuite() populates ≥ 10 test cases.
TEST(SimpleAdversarialTesterTest, APT01_DefaultSuiteHasTenCases) {
    SimpleAdversarialTester tester;
    tester.loadDefaultTestSuite();
    EXPECT_GE(tester.testCases().size(), 10u);
}

// APT-02: runAll() totals, blocked_count, and passed_count are consistent.
TEST(SimpleAdversarialTesterTest, APT02_RunAllCountsConsistent) {
    SimpleAdversarialTester tester;
    tester.loadDefaultTestSuite();

    auto report = tester.runAll();

    EXPECT_EQ(report.total, tester.testCases().size());
    EXPECT_EQ(report.results.size(), report.total);

    // Recount manually.
    size_t manual_blocked = 0, manual_passed = 0;
    for (const auto& r : report.results) {
        if (r.blocked)  ++manual_blocked;
        if (r.passed()) ++manual_passed;
    }
    EXPECT_EQ(report.blocked_count, manual_blocked);
    EXPECT_EQ(report.passed_count,  manual_passed);
}

// APT-03: Known attack payload is blocked.
TEST(SimpleAdversarialTesterTest, APT03_AttackPayloadBlocked) {
    SimpleAdversarialTester tester;
    tester.loadDefaultTestSuite();

    // APT-01 is a known-blocked case.
    auto result = tester.runOne("APT-01");
    EXPECT_TRUE(result.blocked);
    EXPECT_TRUE(result.passed());
}

// APT-04: Benign payload is NOT blocked.
TEST(SimpleAdversarialTesterTest, APT04_BenignPayloadNotBlocked) {
    SimpleAdversarialTester tester;
    tester.loadDefaultTestSuite();

    // APT-02 is a benign case.
    auto result = tester.runOne("APT-02");
    EXPECT_FALSE(result.blocked);
    EXPECT_TRUE(result.passed());
}

// APT-05: Duplicate id throws std::invalid_argument.
TEST(SimpleAdversarialTesterTest, APT05_DuplicateIdThrows) {
    SimpleAdversarialTester tester;
    tester.addTestCase({"CUSTOM-01", AttackCategory::JAILBREAK, "payload", true});
    EXPECT_THROW(
        tester.addTestCase({"CUSTOM-01", AttackCategory::JAILBREAK, "other", false}),
        std::invalid_argument);
}

// APT-06: Custom detector function is invoked.
TEST(SimpleAdversarialTesterTest, APT06_CustomDetectorInvoked) {
    SimpleAdversarialTester tester;

    int call_count = 0;
    tester.setDetectorFn([&call_count](const std::string& /*payload*/) {
        ++call_count;
        return true;  // always blocks
    });

    tester.addTestCase({"T-01", AttackCategory::JAILBREAK, "test payload", true});
    tester.addTestCase({"T-02", AttackCategory::JAILBREAK, "another",      true});

    auto report = tester.runAll();

    EXPECT_EQ(call_count, 2);
    EXPECT_EQ(report.blocked_count, 2u);
    EXPECT_EQ(report.passed_count,  2u);  // expected_blocked=true AND blocked=true
}

// ---------------------------------------------------------------------------
// attackCategoryName — minimum coverage tests (UNUSED_FUNCTIONS_REPORT KEEP)
// ---------------------------------------------------------------------------

// ACN-01: Each AttackCategory value maps to a non-null, non-empty string.
TEST(AttackCategoryNameTest, ACN01_AllValuesNonEmpty) {
    const AttackCategory values[] = {
        AttackCategory::JAILBREAK,
        AttackCategory::ROLE_OVERRIDE,
        AttackCategory::INDIRECT_INJECTION,
        AttackCategory::PROMPT_LEAKING,
        AttackCategory::DATA_EXTRACTION,
    };
    for (auto cat : values) {
        const char* name = attackCategoryName(cat);
        ASSERT_NE(name, nullptr) << "attackCategoryName returned nullptr";
        EXPECT_GT(std::strlen(name), 0u) << "attackCategoryName returned empty string";
    }
}

// ACN-02: Known enum values produce the expected string literals.
TEST(AttackCategoryNameTest, ACN02_KnownMappings) {
    EXPECT_STREQ(attackCategoryName(AttackCategory::JAILBREAK),          "JAILBREAK");
    EXPECT_STREQ(attackCategoryName(AttackCategory::ROLE_OVERRIDE),      "ROLE_OVERRIDE");
    EXPECT_STREQ(attackCategoryName(AttackCategory::INDIRECT_INJECTION), "INDIRECT_INJECTION");
    EXPECT_STREQ(attackCategoryName(AttackCategory::PROMPT_LEAKING),     "PROMPT_LEAKING");
    EXPECT_STREQ(attackCategoryName(AttackCategory::DATA_EXTRACTION),    "DATA_EXTRACTION");
}

// ACN-03: Return value is a compile-time string literal (pointer stability).
TEST(AttackCategoryNameTest, ACN03_PointerStability) {
    const char* a = attackCategoryName(AttackCategory::JAILBREAK);
    const char* b = attackCategoryName(AttackCategory::JAILBREAK);
    EXPECT_EQ(a, b) << "attackCategoryName must return a stable literal pointer";
}
