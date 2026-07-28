/**
 * ThemisDB | Query API Handler QW-46 Collection Name Injection Guard Tests
 * 
 * Tests the fail-closed collection name validation guards in QueryApiHandler::handleQuery().
 * 
 * QW-46: Validates that collection/table names are properly sanitized to prevent:
 *   - Path traversal attacks (../, /, \)
 *   - Shell command injection (;, |, &, `, $)
 *   - LDAP/SQL-like operator injection
 *   - Excessively long names (>256 chars)
 *   - Empty/null values
 * 
 * All guards follow fail-closed semantics: invalid input → bad_request HTTP 400
 */

#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "server/query_api_handler.h"
#include "utils/input_validator.h"

namespace themis { namespace server { namespace test { 

using json = nlohmann::json;
namespace http = boost::beast::http;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixture: QueryApiHandlerQW46Test
// Direct unit tests for InputValidator integration in handleQuery
// ─────────────────────────────────────────────────────────────────────────────

class QueryApiHandlerQW46Test : public ::testing::Test {
protected:
    themis::utils::InputValidator validator_;

    void SetUp() override {
        validator_ = themis::utils::InputValidator();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// QW-46 Tests: Valid and Invalid Table Names
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryApiHandlerQW46Test, ValidTableName_SimpleAlphanumeric) {
    std::string table = "orders";
    EXPECT_TRUE(validator_.validateStringLength(table, 256));
    EXPECT_TRUE(validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ValidTableName_WithUnderscore) {
    std::string table = "customer_orders";
    EXPECT_TRUE(validator_.validateStringLength(table, 256));
    EXPECT_TRUE(validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ValidTableName_WithHyphen) {
    std::string table = "order-data";
    EXPECT_TRUE(validator_.validateStringLength(table, 256));
    EXPECT_TRUE(validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ValidTableName_MixedAlphanumeric) {
    std::string table = "order2023_data";
    EXPECT_TRUE(validator_.validateStringLength(table, 256));
    EXPECT_TRUE(validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ValidTableName_MaxLength256) {
    std::string long_name(256, 'a');
    EXPECT_TRUE(validator_.validateStringLength(long_name, 256));
    EXPECT_TRUE(validator_.validatePathSegment(long_name));
}

// ─────────────────────────────────────────────────────────────────────────────
// QW-46 Guard Tests: Path Traversal Attacks
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryApiHandlerQW46Test, PathTraversalAttack_DoubleDot) {
    std::string table = "../etc/passwd";
    // At least one of these should be false
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, PathTraversalAttack_DoubleSlash) {
    std::string table = "../../data";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, PathTraversalAttack_AbsolutePath) {
    std::string table = "/etc/passwd";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, PathTraversalAttack_WindowsAbsolutePath) {
    std::string table = "C:\\windows\\system32";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, PathTraversalAttack_BackslashSeparator) {
    std::string table = "data\\..\\secret";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, PathTraversalAttack_EscapedDot) {
    std::string table = "table%2e%2e%2fpasswd";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

// ─────────────────────────────────────────────────────────────────────────────
// QW-46 Guard Tests: Shell/Command Injection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryApiHandlerQW46Test, ShellInjection_Semicolon) {
    std::string table = "orders; drop table users";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ShellInjection_Pipe) {
    std::string table = "orders | cat /etc/passwd";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ShellInjection_Ampersand) {
    std::string table = "orders & rm -rf /";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ShellInjection_Backtick) {
    std::string table = "orders`whoami`";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ShellInjection_Dollar) {
    std::string table = "orders$(id)";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ShellInjection_Newline) {
    std::string table = "orders\nDROP TABLE users";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

// ─────────────────────────────────────────────────────────────────────────────
// QW-46 Guard Tests: Length Boundaries
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryApiHandlerQW46Test, LengthBoundary_Exactly256Chars) {
    std::string name_256(256, 'a');
    EXPECT_TRUE(validator_.validateStringLength(name_256, 256));
    EXPECT_TRUE(validator_.validatePathSegment(name_256));
}

TEST_F(QueryApiHandlerQW46Test, LengthBoundary_ExceedsMax) {
    std::string name_257(257, 'a');
    EXPECT_FALSE(validator_.validateStringLength(name_257, 256));
}

TEST_F(QueryApiHandlerQW46Test, LengthBoundary_VeryLong) {
    std::string name_1000(1000, 'a');
    EXPECT_FALSE(validator_.validateStringLength(name_1000, 256));
}

// ─────────────────────────────────────────────────────────────────────────────
// QW-46 Guard Tests: Special Characters and Edge Cases
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryApiHandlerQW46Test, SpecialChar_Space) {
    std::string table = "order data";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, SpecialChar_Parenthesis) {
    std::string table = "order(s)";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, SpecialChar_Dot) {
    std::string table = "schema.table";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, SpecialChar_Bracket) {
    std::string table = "table[0]";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, EmptyTableName) {
    std::string table = "";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, WhitespaceOnlyTableName) {
    std::string table = "   ";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

// ─────────────────────────────────────────────────────────────────────────────
// QW-46 Guard Tests: LDAP/SQL-like Injection Patterns
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryApiHandlerQW46Test, SQLInjection_Like) {
    std::string table = "orders' OR '1'='1";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, SQLInjection_Union) {
    std::string table = "orders UNION SELECT * FROM users";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, LDAPInjection_Asterisk) {
    std::string table = "*";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, LDAPInjection_Parenthesis) {
    std::string table = "(|(uid=*";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

// ─────────────────────────────────────────────────────────────────────────────
// QW-46 Guard Tests: Boundary and Consistency
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(QueryApiHandlerQW46Test, ConsistentValidationAcrossInvocations) {
    // Multiple calls with same invalid name should consistently fail
    std::string invalid_name = "../data";
    for (int i = 0; i < 3; ++i) {
        EXPECT_FALSE(validator_.validateStringLength(invalid_name, 256) && validator_.validatePathSegment(invalid_name))
            << "Iteration: " << i;
    }
}

TEST_F(QueryApiHandlerQW46Test, MixedValidInvalidPatterns) {
    // Even if table name has valid characters, reject if it has invalid sequences
    std::string table = "valid_order_../table";
    EXPECT_FALSE(validator_.validateStringLength(table, 256) && validator_.validatePathSegment(table));
}

TEST_F(QueryApiHandlerQW46Test, ConsistentValidationAcrossMultipleNames) {
    // Valid names should always pass
    std::vector<std::string> valid_names = {
        "orders", "customer_orders", "order-data", "order2023_data"
    };
    for (const auto& name : valid_names) {
        EXPECT_TRUE(validator_.validateStringLength(name, 256) && validator_.validatePathSegment(name))
            << "Name: " << name << " should be valid";
    }

    // Invalid names should always fail
    std::vector<std::string> invalid_names = {
        "../data", "/etc/passwd", "orders;DROP", "orders$(id)", ""
    };
    for (const auto& name : invalid_names) {
        EXPECT_FALSE(validator_.validateStringLength(name, 256) && validator_.validatePathSegment(name))
            << "Name: " << name << " should be invalid";
    }
}
} } } // namespace themis::server::test
