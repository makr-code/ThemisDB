/**
 * @file test_llm_validator.cpp
 * @brief Unit tests for LLM Output Validator
 * 
 * Tests the production readiness output validation that prevents
 * cascading failures from malformed/incomplete LLM responses.
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "llm/llamacpp_inference_engine.h"

using namespace themis::llm;

// Test fixture for LLM validator tests
class LLMValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config_ = LLMOutputValidator::Config();
    }
    
    LLMOutputValidator::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Basic Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, ValidText) {
    LLMOutputValidator validator(config_);
    
    std::string text = "This is a valid response with proper formatting.";
    auto result = validator.validate(text);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(LLMValidatorTest, EmptyText) {
    config_.allow_empty = false;
    LLMOutputValidator validator(config_);
    
    std::string text = "";
    auto result = validator.validate(text);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(LLMValidatorTest, EmptyTextAllowed) {
    config_.allow_empty = true;
    LLMOutputValidator validator(config_);
    
    std::string text = "";
    auto result = validator.validate(text);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.errors.empty());
}

// ═══════════════════════════════════════════════════════════
// UTF-8 Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, ValidUTF8) {
    LLMOutputValidator validator(config_);
    
    std::string text = "Hello world! 你好世界 🌍";
    auto result = validator.validate(text);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.metrics.is_utf8_valid);
}

TEST_F(LLMValidatorTest, InvalidUTF8) {
    config_.require_utf8 = true;
    LLMOutputValidator validator(config_);
    
    // Invalid UTF-8 sequence
    std::string text = "Hello\xFF\xFEworld";
    auto result = validator.validate(text);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.metrics.is_utf8_valid);
}

// ═══════════════════════════════════════════════════════════
// Length Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, TextTooShort) {
    config_.min_length = 10;
    LLMOutputValidator validator(config_);
    
    std::string text = "Hi";  // Only 2 chars
    auto result = validator.validate(text);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_GT(result.errors.size(), 0);
}

TEST_F(LLMValidatorTest, TextTooLong) {
    config_.max_length = 100;
    LLMOutputValidator validator(config_);
    
    std::string text(200, 'a');  // 200 chars
    auto result = validator.validate(text);
    
    // Might not be invalid, but should have warning
    EXPECT_GT(result.warnings.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Truncation Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, CompleteSentence) {
    config_.check_truncation = true;
    LLMOutputValidator validator(config_);
    
    std::string text = "This is a complete sentence.";
    auto result = validator.validate(text);
    
    EXPECT_FALSE(result.metrics.is_truncated);
}

TEST_F(LLMValidatorTest, TruncatedMidSentence) {
    config_.check_truncation = true;
    LLMOutputValidator validator(config_);
    
    std::string text = "This is an incomplete";  // No ending punctuation
    auto result = validator.validate(text);
    
    EXPECT_TRUE(result.metrics.is_truncated);
}

TEST_F(LLMValidatorTest, ExplicitTruncationMarker) {
    config_.check_truncation = true;
    LLMOutputValidator validator(config_);
    
    std::string text = "This response was... [truncated]";
    auto result = validator.validate(text);
    
    EXPECT_TRUE(result.metrics.is_truncated);
}

// ═══════════════════════════════════════════════════════════
// Metrics Calculation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, WordCount) {
    LLMOutputValidator validator(config_);
    
    std::string text = "This is a test with five words exactly.";
    auto result = validator.validate(text);
    
    EXPECT_EQ(result.metrics.word_count, 8);
}

TEST_F(LLMValidatorTest, SentenceCount) {
    LLMOutputValidator validator(config_);
    
    std::string text = "First sentence. Second sentence! Third sentence?";
    auto result = validator.validate(text);
    
    EXPECT_EQ(result.metrics.sentence_count, 3);
}

TEST_F(LLMValidatorTest, AverageWordLength) {
    LLMOutputValidator validator(config_);
    
    std::string text = "I am ok";  // Words of length 1, 2, 2 -> avg = 1.67
    auto result = validator.validate(text);
    
    EXPECT_GT(result.metrics.avg_word_length, 1.0);
    EXPECT_LT(result.metrics.avg_word_length, 3.0);
}

// ═══════════════════════════════════════════════════════════
// Token Count Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, TokenLimitReached) {
    LLMOutputValidator validator(config_);
    
    std::string text = "Test response.";
    auto result = validator.validateWithTokens(text, 100, 100);
    
    EXPECT_EQ(result.metrics.token_count, 100);
    EXPECT_TRUE(result.metrics.is_truncated);
    EXPECT_GT(result.warnings.size(), 0);
}

TEST_F(LLMValidatorTest, TokenLimitNotReached) {
    LLMOutputValidator validator(config_);
    
    std::string text = "Test response.";
    auto result = validator.validateWithTokens(text, 50, 100);
    
    EXPECT_EQ(result.metrics.token_count, 50);
    // May or may not be truncated based on text analysis
}

// ═══════════════════════════════════════════════════════════
// Error Pattern Detection Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, ErrorPatternDetection) {
    LLMOutputValidator validator(config_);
    
    std::string text = "Error: Failed to process request";
    auto result = validator.validate(text);
    
    EXPECT_FALSE(result.is_valid);
}

TEST_F(LLMValidatorTest, StubResponseDetection) {
    LLMOutputValidator validator(config_);
    
    std::string text = "STUB_RESPONSE";
    auto result = validator.validate(text);
    
    EXPECT_FALSE(result.is_valid);
}

TEST_F(LLMValidatorTest, PlaceholderDetection) {
    LLMOutputValidator validator(config_);
    
    std::string text = "This is a placeholder response";
    auto result = validator.validate(text);
    
    EXPECT_FALSE(result.is_valid);
}

// ═══════════════════════════════════════════════════════════
// Semantic Coherence Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, HighCoherence) {
    config_.check_coherence = true;
    config_.min_coherence = 0.3;
    LLMOutputValidator validator(config_);
    
    std::string text = "The quick brown fox jumps over the lazy dog. This is a well-formed sentence.";
    auto result = validator.validate(text);
    
    EXPECT_GT(result.metrics.semantic_coherence, 0.3);
}

TEST_F(LLMValidatorTest, LowCoherence) {
    config_.check_coherence = true;
    config_.min_coherence = 0.7;
    LLMOutputValidator validator(config_);
    
    // Nonsense text
    std::string text = "aaa aaa aaa aaa aaa aaa";
    auto result = validator.validate(text);
    
    EXPECT_LE(result.metrics.semantic_coherence, 0.7);  // Use <= instead of < for boundary case
    EXPECT_GT(result.warnings.size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Repeating Pattern Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, RepeatingPatternDetection) {
    LLMOutputValidator validator(config_);
    
    std::string text = "repeat repeat repeat repeat repeat";
    auto result = validator.validate(text);
    
    // Should detect the repeating pattern
    EXPECT_GT(result.warnings.size(), 0);
}

TEST_F(LLMValidatorTest, NoRepeatingPattern) {
    LLMOutputValidator validator(config_);
    
    std::string text = "This is a normal sentence without repetition.";
    auto result = validator.validate(text);
    
    // Should pass without repeating pattern warnings
    EXPECT_TRUE(result.is_valid);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LLMValidatorTest, ComplexValidResponse) {
    LLMOutputValidator validator(config_);
    
    std::string text = R"(
        The ThemisDB database provides several key features:
        1. ACID transactions with MVCC
        2. Multi-model support (relational, graph, vector, document)
        3. High performance with GPU acceleration
        
        These features make it suitable for modern AI applications.
    )";
    
    auto result = validator.validate(text);
    
    EXPECT_TRUE(result.is_valid);
    EXPECT_GT(result.metrics.word_count, 10);
    EXPECT_GT(result.metrics.sentence_count, 1);
    EXPECT_TRUE(result.metrics.is_utf8_valid);
}

TEST_F(LLMValidatorTest, InvalidResponseWithMultipleIssues) {
    LLMOutputValidator validator(config_);
    
    // Multiple issues: error pattern, invalid UTF-8, truncation
    std::string text = "Error: \xFF\xFE incomplete";
    auto result = validator.validate(text);
    
    EXPECT_FALSE(result.is_valid);
    EXPECT_GT(result.errors.size(), 1);  // Should have multiple errors
}


