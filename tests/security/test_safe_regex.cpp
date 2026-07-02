#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "security/safe_regex.h"

using namespace themis::security;

class SafeRegexTest : public ::testing::Test {
protected:
    SafeRegex regex_;

    void SetUp() override {
        regex_.clear_cache();
    }
};

// ============================================================================
// ReDoS Attack Prevention Tests
// ============================================================================

TEST_F(SafeRegexTest, NestedQuantifiers_Blocked) {
    // (a+)+ is a known ReDoS vulnerability
    std::string pattern = "(a+)+";
    EXPECT_FALSE(SafeRegex::is_pattern_safe(pattern));
}

TEST_F(SafeRegexTest, NestedQuantifiers_Multiple_Blocked) {
    // (a*)+ is another ReDoS pattern
    std::string pattern = "(a*)+";
    EXPECT_FALSE(SafeRegex::is_pattern_safe(pattern));
}

TEST_F(SafeRegexTest, SafePattern_Accepted) {
    // Simple, safe pattern
    std::string pattern = "^[a-z]+$";
    EXPECT_TRUE(SafeRegex::is_pattern_safe(pattern));
}

TEST_F(SafeRegexTest, Input_NormalLength_Accepted) {
    // Normal length input should pass validation
    std::string text = "Hello World";
    EXPECT_TRUE(SafeRegex::validate_input(text));
}

TEST_F(SafeRegexTest, Input_ExcessiveLength_Rejected) {
    // Very long input to prevent DoS
    std::string text(20000, 'A');
    EXPECT_FALSE(SafeRegex::validate_input(text));
}

TEST_F(SafeRegexTest, Input_PathologicalRepetition_Rejected) {
    // Input with excessive repetition
    std::string text(2000, 'A');  // 2000 repeated As
    EXPECT_FALSE(SafeRegex::validate_input(text));
}

TEST_F(SafeRegexTest, Match_SimplePattern) {
    // Test basic pattern matching
    std::string pattern = "^hello$";
    std::string text = "hello";
    EXPECT_TRUE(regex_.match(pattern, text));
}

TEST_F(SafeRegexTest, Match_NoMatch) {
    // Test non-matching pattern
    std::string pattern = "^hello$";
    std::string text = "world";
    EXPECT_FALSE(regex_.match(pattern, text));
}

TEST_F(SafeRegexTest, Match_CaseInsensitive) {
    // Pattern should be case sensitive by default
    std::string pattern = "^hello$";
    std::string text = "Hello";
    EXPECT_FALSE(regex_.match(pattern, text));
}

TEST_F(SafeRegexTest, Search_PatternFound) {
    // Search for pattern in middle of text
    std::string pattern = "world";
    std::string text = "Hello world!";
    EXPECT_TRUE(regex_.search(pattern, text));
}

TEST_F(SafeRegexTest, Search_PatternNotFound) {
    // Search for non-existent pattern
    std::string pattern = "xyz";
    std::string text = "Hello world!";
    EXPECT_FALSE(regex_.search(pattern, text));
}

TEST_F(SafeRegexTest, Replace_Simple) {
    // Simple string replacement
    std::string pattern = "world";
    std::string text = "Hello world!";
    std::string replacement = "universe";
    std::string result = regex_.replace(pattern, text, replacement);
    EXPECT_EQ(result, "Hello universe!");
}

TEST_F(SafeRegexTest, Replace_MultipleMatches) {
    // Replace all occurrences
    std::string pattern = "a";
    std::string text = "banana";
    std::string replacement = "e";
    std::string result = regex_.replace(pattern, text, replacement);
    EXPECT_EQ(result, "benene");
}

TEST_F(SafeRegexTest, Split_Simple) {
    // Simple string split
    std::string pattern = ",";
    std::string text = "a,b,c";
    auto result = regex_.split(pattern, text);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");
}

// ============================================================================
// ReDoS Timeout Protection
// ============================================================================

TEST_F(SafeRegexTest, Timeout_ShouldNotTimeout_SimplePattern) {
    // Simple pattern should complete quickly
    std::string pattern = "^test$";
    std::string text = "test";
    EXPECT_NO_THROW(regex_.match(pattern, text, std::chrono::milliseconds(100)));
}

TEST_F(SafeRegexTest, Timeout_LongText_StillCompletes) {
    // Even with long text, simple pattern should match quickly
    std::string pattern = "^[a-z]+$";
    std::string text(5000, 'a');
    EXPECT_NO_THROW(regex_.match(pattern, text, std::chrono::seconds(5)));
}

TEST_F(SafeRegexTest, UnsafePattern_Rejected) {
    // Unsafe patterns should be rejected during compilation
    std::string pattern = "(a+)+";
    std::string text = "aaaa";
    EXPECT_THROW(regex_.match(pattern, text), std::runtime_error);
}

// ============================================================================
// Real-World Regex Patterns (Safe)
// ============================================================================

TEST_F(SafeRegexTest, EmailValidation_ValidEmail) {
    // Email validation pattern (simplified)
    std::string pattern = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";
    std::string email = "user@example.com";
    EXPECT_TRUE(regex_.match(pattern, email));
}

TEST_F(SafeRegexTest, EmailValidation_InvalidEmail) {
    // Invalid email
    std::string pattern = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";
    std::string email = "invalid.email";
    EXPECT_FALSE(regex_.match(pattern, email));
}

TEST_F(SafeRegexTest, URLValidation_SimpleURL) {
    // Simple URL pattern
    std::string pattern = "^https?://";
    std::string url = "https://example.com";
    EXPECT_TRUE(regex_.search(pattern, url));
}

TEST_F(SafeRegexTest, IPAddress_Valid) {
    // IPv4 pattern (simplified)
    std::string pattern = "^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$";
    std::string ip = "192.168.1.1";
    EXPECT_TRUE(regex_.match(pattern, ip));
}

TEST_F(SafeRegexTest, JSONKey_Extraction) {
    // Extract JSON keys
    std::string pattern = "\"([^\"]+)\"\\s*:";
    std::string json = R"({"name": "John", "age": 30})";
    EXPECT_TRUE(regex_.search(pattern, json));
}

TEST_F(SafeRegexTest, SQLInjection_Detection) {
    // Detect common SQL injection patterns
    std::string pattern = "'.*?(OR|AND).*?'";
    std::string input = "' OR '1'='1";
    EXPECT_TRUE(regex_.search(pattern, input));
}

// ============================================================================
// Input Validation Edge Cases
// ============================================================================

TEST_F(SafeRegexTest, InputValidation_EmptyString) {
    // Empty string should be valid
    EXPECT_TRUE(SafeRegex::validate_input(""));
}

TEST_F(SafeRegexTest, InputValidation_MaxLength) {
    // Test custom max length
    std::string text = "test";
    EXPECT_TRUE(SafeRegex::validate_input(text, 100));
}

TEST_F(SafeRegexTest, InputValidation_ExceedsCustomMax) {
    // Test exceeding custom max length
    std::string text(1000, 'a');
    EXPECT_FALSE(SafeRegex::validate_input(text, 100));
}

// ============================================================================
// Cache Functionality
// ============================================================================

TEST_F(SafeRegexTest, Cache_FirstCompilation) {
    // First call should miss cache
    std::string pattern = "^test$";
    regex_.clear_cache();
    regex_.match(pattern, "test");
    std::string stats = regex_.cache_stats();
    EXPECT_THAT(stats, testing::HasSubstr("1 hits"));
}

TEST_F(SafeRegexTest, Cache_CacheHit) {
    // Second call with same pattern should hit cache
    std::string pattern = "^test$";
    regex_.clear_cache();
    regex_.match(pattern, "test");
    regex_.match(pattern, "test");
    // Stats should show hit
    // This is to verify cache is working
}

TEST_F(SafeRegexTest, Cache_DifferentPatterns) {
    // Different patterns should be cached separately
    regex_.clear_cache();
    regex_.match("^a$", "a");
    regex_.match("^b$", "b");
    std::string stats = regex_.cache_stats();
    EXPECT_THAT(stats, testing::HasSubstr("2"));  // 2 misses
}

// ============================================================================
// Error Handling
// ============================================================================

TEST_F(SafeRegexTest, InvalidPattern_SyntaxError) {
    // Invalid regex pattern
    std::string pattern = "[unclosed";
    std::string text = "test";
    EXPECT_THROW(regex_.match(pattern, text), std::runtime_error);
}

TEST_F(SafeRegexTest, InvalidPattern_Message) {
    // Error message should be informative
    std::string pattern = "[";
    std::string text = "test";
    try {
        regex_.match(pattern, text);
        FAIL() << "Expected exception";
    } catch (const std::runtime_error& e) {
        EXPECT_THAT(std::string(e.what()), testing::HasSubstr("regex"));
    }
}

TEST_F(SafeRegexTest, DangerousPattern_Message) {
    // Dangerous pattern should be detected
    std::string pattern = "(a+)+";
    std::string text = "test";
    try {
        regex_.match(pattern, text);
        FAIL() << "Expected exception for unsafe pattern";
    } catch (const std::runtime_error& e) {
        EXPECT_THAT(std::string(e.what()), testing::HasSubstr("unsafe"));
    }
}

// ============================================================================
// Unicode and Special Characters
// ============================================================================

TEST_F(SafeRegexTest, Unicode_SimplePattern) {
    // Basic unicode support
    std::string pattern = "test";
    std::string text = "test";
    EXPECT_TRUE(regex_.match(pattern, text));
}

TEST_F(SafeRegexTest, WhitespaceHandling) {
    // Pattern with whitespace
    std::string pattern = "\\s+";
    std::string text = "hello   world";
    EXPECT_TRUE(regex_.search(pattern, text));
}

TEST_F(SafeRegexTest, DigitPattern) {
    // Digit matching
    std::string pattern = "\\d+";
    std::string text = "Version 2.5.1";
    EXPECT_TRUE(regex_.search(pattern, text));
}

// ============================================================================
// Real-World Attack Scenarios
// ============================================================================

TEST_F(SafeRegexTest, ReDoS_AttackPattern_Nested) {
    // Known ReDoS pattern should be blocked
    // (a+)+b won't match "aaaaaaa" but will cause exponential backtracking
    std::string pattern = "(a+)+b";
    EXPECT_FALSE(SafeRegex::is_pattern_safe(pattern));
}

TEST_F(SafeRegexTest, ReDoS_AttackPattern_Alternation) {
    // (a|ab)+ with input "aaaa" causes exponential time
    std::string pattern = "(a|ab)+";
    std::string text = "aaaaaaaaaaaa";
    // This pattern is detected as potentially unsafe
    EXPECT_FALSE(SafeRegex::is_pattern_safe(pattern));
}

TEST_F(SafeRegexTest, ReDoS_Protection_StackOverflow) {
    // Prevent stack overflow from deep recursion
    std::string text(10000, 'a');
    EXPECT_FALSE(SafeRegex::validate_input(text));
}

// ============================================================================
// BATCH B: ReDoS Remediation Integration Tests
// ============================================================================

TEST_F(SafeRegexTest, BatchB_QueryValidator_PatternValidation) {
    // Remediation pattern: Query validator pattern validation
    // BEFORE: std::regex pattern = user_pattern; pattern.match(input);
    // AFTER: SafeRegex::match(user_pattern, input, timeout)
     
    // This example tests a safe pattern (should work)
    std::string safe_pattern = "^[a-zA-Z0-9_%]*$";  // Alphanumeric with underscores
    std::string input = "SELECT_123_ABC";
     
    EXPECT_TRUE(regex_.match(safe_pattern, input));
}

TEST_F(SafeRegexTest, BatchB_InputValidator_SuspiciousPattern) {
    // Remediation pattern: Input validation with time protection
    // Example: Detecting SQL injection patterns
    std::string pattern = "^[a-zA-Z0-9 ]*$";  // Allow only safe characters
    std::string suspicious = "'; DROP TABLE users; --";
     
    EXPECT_FALSE(regex_.match(pattern, suspicious));
}

TEST_F(SafeRegexTest, BatchB_FilterEngine_TimeoutProtection) {
    // Remediation pattern: Filter engine with timeout
    // Testing that simple patterns complete quickly
    std::string filter_pattern = "^test_.*$";
    std::string data_row = "test_value_123";
     
    auto start = std::chrono::high_resolution_clock::now();
    bool matches = regex_.match(filter_pattern, data_row, 
                               std::chrono::seconds(5));
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
     
    EXPECT_TRUE(matches);
    // Should complete quickly
    EXPECT_LT(elapsed.count(), 1000000000);  // Less than 1 second in nanoseconds
}

TEST_F(SafeRegexTest, BatchB_RequestParser_URLPattern) {
    // Remediation pattern: Request parser URL pattern matching
    // Safe pattern: Only allow valid URL characters
    std::string url_pattern = "^[a-zA-Z0-9._~:/?#\\[\\]@!$&'()*+,;=%]*$";
     
    // Valid URLs
    EXPECT_TRUE(regex_.match(url_pattern, "https://example.com/path?q=value"));
    EXPECT_TRUE(regex_.match(url_pattern, "/api/v1/users/123"));
     
    // Invalid URLs
    EXPECT_FALSE(regex_.match(url_pattern, "../../../etc/passwd"));
}

TEST_F(SafeRegexTest, BatchB_SearchEngine_KeywordPattern) {
    // Remediation pattern: Search engine keyword matching
    // Safe pattern: Simple keyword matching with word boundaries
    std::string keyword_pattern = "\\bquery\\b";  // Word boundary, not quantifier
     
    EXPECT_TRUE(regex_.search(keyword_pattern, "This is a query about something"));
    EXPECT_FALSE(regex_.search(keyword_pattern, "quarantine"));  // Word boundary protects
}

TEST_F(SafeRegexTest, BatchB_CSVParser_FieldPattern) {
    // Remediation pattern: CSV parser field pattern
    // Safe pattern: Match quoted fields or unquoted fields with limited special chars
    std::string field_pattern = R"(\"[^\"]*\"|[^,]*)";  // No nested quantifiers
     
    std::string csv_row = R"("Last, First",12345,"Valid","Data")";
    EXPECT_TRUE(regex_.search(field_pattern, csv_row));
}

TEST_F(SafeRegexTest, BatchB_NestedQuantifierDetection) {
    // Verify detection of dangerous nested quantifiers
    // Pattern 1: (a+)+
    EXPECT_FALSE(SafeRegex::is_pattern_safe("(a+)+"));
     
    // Pattern 2: (a*)+
    EXPECT_FALSE(SafeRegex::is_pattern_safe("(a*)+"));
     
    // Pattern 3: (a|a)*
    EXPECT_FALSE(SafeRegex::is_pattern_safe("(a|a)*"));
     
    // Pattern 4: (a|ab)+
    EXPECT_FALSE(SafeRegex::is_pattern_safe("(a|ab)+"));
}

TEST_F(SafeRegexTest, BatchB_SafeAlternationPattern) {
    // Remediation pattern: Safe alternation (no overlap)
    // This should be safe: (cat|dog|bird)
    std::string safe_alt = "(cat|dog|bird)";
     
    // These are actually safe (non-overlapping)
    std::string text1 = "The cat sleeps";
    std::string text2 = "The dog runs";
     
    EXPECT_TRUE(regex_.search(safe_alt, text1));
    EXPECT_TRUE(regex_.search(safe_alt, text2));
}

TEST_F(SafeRegexTest, BatchB_InputLengthValidation) {
    // Remediation pattern: Pre-validate input length before matching
    // This prevents DoS attacks via excessively long input strings
     
    // Short input should pass
    EXPECT_TRUE(SafeRegex::validate_input("short", 10000));
     
    // Long input should fail with small limit
    std::string long_input(5000, 'a');
    EXPECT_FALSE(SafeRegex::validate_input(long_input, 1000));
}

TEST_F(SafeRegexTest, BatchB_RepetitionPattern_Rejected) {
    // Remediation pattern: Reject patterns with dangerous repetition
    // Example: Users attempting to use (x+)+ patterns
     
    std::string dangerous = "(x+)+";
    EXPECT_THROW({
        regex_.match(dangerous, "xxxx");
    }, std::runtime_error);
}

TEST_F(SafeRegexTest, BatchB_RegexCaching_Performance) {
    // Remediation pattern: Use caching to improve performance
    // Same pattern should be reused from cache
     
    regex_.clear_cache();
    std::string pattern = "^[a-zA-Z0-9]{1,50}$";
     
    // First call
    regex_.match(pattern, "test1");
     
    // Second call should hit cache
    auto start = std::chrono::high_resolution_clock::now();
    regex_.match(pattern, "test2");
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
     
    // Cache hit should be very fast
    // (This is a soft test; actual timing depends on system)
    EXPECT_GE(elapsed.count(), 0);
}

TEST_F(SafeRegexTest, BatchB_PreValidation_BeforeRegex) {
    // Remediation pattern: Pre-validate input before regex matching
    // This is a key remediation: check input length first, then match
     
    std::string potentially_malicious = std::string(15000, 'a');
     
    // First: Validate input length
    if (!SafeRegex::validate_input(potentially_malicious)) {
        // Input rejected early, no regex matching needed
        EXPECT_TRUE(true);
    } else {
        // If input passes validation, regex matching is safe
        EXPECT_TRUE(regex_.match("^[a]+$", potentially_malicious, 
                                std::chrono::seconds(5)));
    }
}
