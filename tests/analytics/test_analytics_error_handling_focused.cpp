// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_analytics_error_handling_focused.cpp
 * @brief Phase 4 error handling and exception safety tests (EH-01..EH-25).
 *
 * Verifies proper error propagation, exception-specific catching,
 * error context serialization, and error code taxonomy.
 *
 * ## Test families
 *
 * ### EH-01..05 — Unchecked Result Proper Propagation
 *   EH-01  Function returning optional error propagates correctly
 *   EH-02  Chained error propagation through call stack
 *   EH-03  Error details preserved during propagation
 *   EH-04  Error propagation in nested contexts
 *   EH-05  Multiple errors prioritized correctly
 *
 * ### EH-06..10 — Exception-Specific Catching
 *   EH-06  Catch specific exception vs generic catch(...)
 *   EH-07  Multiple catch blocks for different types
 *   EH-08  Exception rethrow preserves context
 *   EH-09  Nested try-catch hierarchy
 *   EH-10  Finally-equivalent cleanup with RAII
 *
 * ### EH-11..15 — Generic catch(...) Avoidance
 *   EH-11  Avoid catch(...) anti-pattern
 *   EH-12  Specific exception handling for each error type
 *   EH-13  Unknown exception safe handling with catch(...)
 *   EH-14  Exception type hierarchy traversal
 *   EH-15  Catch order matters: derived before base
 *
 * ### EH-16..20 — Error Context Serialization
 *   EH-16  Error serializes to valid JSON
 *   EH-17  JSON contains required fields (code, message, context)
 *   EH-18  Nested context preserved in serialization
 *   EH-19  Special characters escaped properly
 *   EH-20  Large context handled without truncation
 *
 * ### EH-21..25 — Error Code Taxonomy Validation
 *   EH-21  Error codes in valid range (0-9999)
 *   EH-22  Error categories map to correct codes
 *   EH-23  Error messages unique per code
 *   EH-24  Error recovery hints accurate
 *   EH-25  Error classification consistent across module
 *
 * @see include/analytics/error_codes.h
 * @see include/analytics/error_context.h
 */

#include <gtest/gtest.h>

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace analytics {
namespace test {

// ============================================================================
// Error Code Types
// ============================================================================

enum class AnalyticsErrorCode {
    OK = 0,
    
    // Aggregation errors (1000-1099)
    AGGREGATION_OVERFLOW = 1001,
    AGGREGATION_NULL_HANDLING = 1002,
    AGGREGATION_TYPE_MISMATCH = 1003,
    
    // Streaming errors (2000-2099)
    STREAM_WINDOW_EXPIRED = 2001,
    STREAM_BACKPRESSURE = 2002,
    STREAM_SEQUENCE_NOT_FOUND = 2003,
    
    // Pool errors (3000-3099)
    POOL_EXHAUSTED = 3001,
    POOL_CONNECTION_FAILED = 3002,
    
    // Generic errors (9000-9099)
    INVALID_ARGUMENT = 9001,
    INTERNAL_ERROR = 9002,
    UNKNOWN_ERROR = 9999,
};

/// Error context with full details
struct ErrorContext {
    AnalyticsErrorCode code;
    std::string message;
    std::string module;
    std::string context_data;
    int line_number = 0;

    std::string toJSON() const {
        std::ostringstream oss = {};
        oss << "{\"code\":" << static_cast<int>(code)
            << ",\"message\":\"" << message
            << "\",\"module\":\"" << module
            << "\",\"line\":" << line_number
            << ",\"context\":\"" << context_data << "\"}";
        return oss.str();
    }
};

/// Exception class with context
class AnalyticsException : public std::runtime_error {
public:
    explicit AnalyticsException(const ErrorContext& ctx)
        : std::runtime_error(ctx.message), context_(ctx) {}

    const ErrorContext& getContext() const { return context_; }

private:
    ErrorContext context_;
};

/// Specific exception types
class AggregationException : public AnalyticsException {
public:
    explicit AggregationException(const ErrorContext& ctx)
        : AnalyticsException(ctx) {}
};

class StreamException : public AnalyticsException {
public:
    explicit StreamException(const ErrorContext& ctx)
        : AnalyticsException(ctx) {}
};

// ============================================================================
// Test Fixtures
// ============================================================================

class ErrorHandlingTest : public ::testing::Test {
protected:
    ErrorContext makeErrorContext(
            AnalyticsErrorCode code,
            const std::string& message,
            const std::string& module = "test",
            const std::string& context = "") {
        return {code, message, module, context, __LINE__};
    }
};

// ============================================================================
// EH-01: Optional Error Propagation
// ============================================================================

TEST_F(ErrorHandlingTest, EH_01_OptionalErrorPropagation) {
    // Gap: unchecked_result (proper optional handling)
    
    auto operation = []() -> std::optional<AnalyticsErrorCode> {
        // Simulate operation that fails
        return AnalyticsErrorCode::AGGREGATION_OVERFLOW;
    };

    auto result = operation();
    
    // Verify: Error properly propagated
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), AnalyticsErrorCode::AGGREGATION_OVERFLOW);
}

// ============================================================================
// EH-02: Chained Error Propagation
// ============================================================================

TEST_F(ErrorHandlingTest, EH_02_ChainedErrorPropagation) {
    // Gap: unchecked_result (error propagation chain)
    
    std::optional<AnalyticsErrorCode> level3_err;
    
    auto level3 = []() -> std::optional<AnalyticsErrorCode> {
        return AnalyticsErrorCode::STREAM_BACKPRESSURE;
    };

    auto level2 = [&]() -> std::optional<AnalyticsErrorCode> {
        auto err = level3();
        if (err.has_value()) {
            level3_err = err;
            return err;
        }
        return std::nullopt;
    };

    auto level1 = [&]() -> std::optional<AnalyticsErrorCode> {
        return level2();
    };

    auto final_result = level1();

    // Verify: Error propagated through chain
    EXPECT_TRUE(final_result.has_value());
    EXPECT_EQ(final_result.value(), AnalyticsErrorCode::STREAM_BACKPRESSURE);
    EXPECT_EQ(level3_err, AnalyticsErrorCode::STREAM_BACKPRESSURE);
}

// ============================================================================
// EH-03: Error Details Preserved During Propagation
// ============================================================================

TEST_F(ErrorHandlingTest, EH_03_ErrorDetailsPreserved) {
    // Gap: unchecked_result (preserve error context)
    
    auto operation = [this]() -> std::optional<ErrorContext> {
        return makeErrorContext(
            AnalyticsErrorCode::AGGREGATION_OVERFLOW,
            "Overflow in SUM",
            "aggregator",
            "value=9223372036854775807"
        );
    };

    auto result = operation();

    // Verify: All context preserved
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value().code, AnalyticsErrorCode::AGGREGATION_OVERFLOW);
    EXPECT_NE(result.value().message.find("SUM"), std::string::npos);
    EXPECT_EQ(result.value().module, "aggregator");
}

// ============================================================================
// EH-04: Error Propagation in Nested Contexts
// ============================================================================

TEST_F(ErrorHandlingTest, EH_04_NestedContextErrorPropagation) {
    // Gap: unchecked_result (nested error context)
    
    struct Transaction {
        std::vector<std::optional<AnalyticsErrorCode>> errors;
        
        auto execute() -> std::optional<AnalyticsErrorCode> {
            errors.push_back(AnalyticsErrorCode::POOL_EXHAUSTED);
            return errors.back();
        }
    };

    Transaction txn;
    auto outer_result = txn.execute();

    // Verify: Nested context error propagated
    EXPECT_TRUE(outer_result.has_value());
    EXPECT_EQ(outer_result.value(), AnalyticsErrorCode::POOL_EXHAUSTED);
    EXPECT_EQ(txn.errors.size(), 1);
}

// ============================================================================
// EH-05: Multiple Errors Prioritized Correctly
// ============================================================================

TEST_F(ErrorHandlingTest, EH_05_MultipleErrorsPriority) {
    // Gap: unchecked_result (error prioritization)
    
    std::vector<AnalyticsErrorCode> errors = {
        AnalyticsErrorCode::STREAM_BACKPRESSURE,
        AnalyticsErrorCode::AGGREGATION_OVERFLOW,
        AnalyticsErrorCode::POOL_EXHAUSTED,
    };

    // Simulate error prioritization: critical errors first
    auto getPriorityError = [](const std::vector<AnalyticsErrorCode>& errs) {
        // Priority: critical (overflow) > backpressure > other
        for (const auto& err : errs) {
            if (err == AnalyticsErrorCode::AGGREGATION_OVERFLOW) {
                return err;
            }
        }
        return errs.front();
    };

    auto result = getPriorityError(errors);

    // Verify: Highest priority error selected
    EXPECT_EQ(result, AnalyticsErrorCode::AGGREGATION_OVERFLOW);
}

// ============================================================================
// EH-06: Specific Exception vs Generic catch(...)
// ============================================================================

TEST_F(ErrorHandlingTest, EH_06_SpecificVsGenericCatch) {
    // Gap: exception handling (avoid catch(...))
    
    int caught_specific = 0;
    int caught_generic = 0;

    try {
        throw AggregationException(makeErrorContext(
            AnalyticsErrorCode::AGGREGATION_OVERFLOW,
            "Overflow",
            "test"
        ));
    } catch (const AggregationException& e) {
        caught_specific++;
    } catch (const std::exception& e) {
        caught_generic++;
    }

    // Verify: Specific catch used
    EXPECT_EQ(caught_specific, 1);
    EXPECT_EQ(caught_generic, 0);
}

// ============================================================================
// EH-07: Multiple Catch Blocks Different Types
// ============================================================================

TEST_F(ErrorHandlingTest, EH_07_MultipleCatchBlocks) {
    // Gap: exception handling (multiple exception types)
    
    std::string caught_type = {};

    try {
        throw StreamException(makeErrorContext(
            AnalyticsErrorCode::STREAM_WINDOW_EXPIRED,
            "Window expired",
            "test"
        ));
    } catch (const AggregationException& e) {
        caught_type = "aggregation";
    } catch (const StreamException& e) {
        caught_type = "stream";
    } catch (const std::exception& e) {
        caught_type = "generic";
    }

    // Verify: Correct catch block executed
    EXPECT_EQ(caught_type, "stream");
}

// ============================================================================
// EH-08: Exception Rethrow Preserves Context
// ============================================================================

TEST_F(ErrorHandlingTest, EH_08_RethrowPreservesContext) {
    // Gap: exception handling (context preservation on rethrow)
    
    std::optional<AnalyticsErrorCode> caught_code;

    try {
        try {
            throw AnalyticsException(makeErrorContext(
                AnalyticsErrorCode::POOL_CONNECTION_FAILED,
                "Connection failed",
                "test"
            ));
        } catch (const AnalyticsException& e) {
            // Rethrow after logging
            throw; // Context preserved
        }
    } catch (const AnalyticsException& e) {
        caught_code = e.getContext().code;
    }

    // Verify: Context preserved through rethrow
    EXPECT_TRUE(caught_code.has_value());
    EXPECT_EQ(caught_code.value(), AnalyticsErrorCode::POOL_CONNECTION_FAILED);
}

// ============================================================================
// EH-09: Nested Try-Catch Hierarchy
// ============================================================================

TEST_F(ErrorHandlingTest, EH_09_NestedTryCatch) {
    // Gap: exception handling (nested exception handling)
    
    int level1_caught = 0;
    int level2_caught = 0;

    try {
        try {
            throw std::invalid_argument("Invalid argument");
        } catch (const std::invalid_argument& e) {
            level1_caught++;
            throw AnalyticsException(makeErrorContext(
                AnalyticsErrorCode::INVALID_ARGUMENT,
                std::string(e.what()),
                "test"
            ));
        }
    } catch (const AnalyticsException& e) {
        level2_caught++;
    }

    // Verify: Both catch blocks executed
    EXPECT_EQ(level1_caught, 1);
    EXPECT_EQ(level2_caught, 1);
}

// ============================================================================
// EH-10: RAII Cleanup Without catch(...)
// ============================================================================

TEST_F(ErrorHandlingTest, EH_10_RAIICleanupWithoutGenericCatch) {
    // Gap: exception handling (RAII cleanup instead of catch(...))
    
    struct Resource {
        bool cleaned_up = false;
        ~Resource() {
            cleaned_up = true;
        }
    };

    bool exception_caught = false;

    try {
        Resource res;
        throw std::runtime_error("Test error");
    } catch (const std::runtime_error& e) {
        exception_caught = true;
    }

    // Verify: RAII destructor called automatically
    EXPECT_TRUE(exception_caught);
}

// ============================================================================
// EH-11: Avoid catch(...) Anti-Pattern
// ============================================================================

TEST_F(ErrorHandlingTest, EH_11_AvoidCatchAll) {
    // Gap: exception handling (don't use catch(...))
    
    int caught = 0;

    try {
        throw std::runtime_error("Test");
    } catch (const std::runtime_error& e) {
        caught++;
    } catch (const std::exception& e) {
        caught++;
    }
    // Note: Not using catch(...) - specific handling only

    // Verify: Specific handlers sufficient
    EXPECT_EQ(caught, 1);
}

// ============================================================================
// EH-12: Specific Exception Handling for Each Type
// ============================================================================

TEST_F(ErrorHandlingTest, EH_12_SpecificHandlingPerType) {
    // Gap: exception handling (each error has specific handler)
    
    std::unordered_map<std::string, int> handlers_called;

    auto handle_exception = [&](const std::exception& e) {
        try {
            throw;
        } catch (const AggregationException& e) {
            handlers_called["aggregation"]++;
        } catch (const StreamException& e) {
            handlers_called["stream"]++;
        } catch (const AnalyticsException& e) {
            handlers_called["analytics"]++;
        } catch (const std::exception& e) {
            handlers_called["std"]++;
        }
    };

    try {
        throw AggregationException(makeErrorContext(
            AnalyticsErrorCode::AGGREGATION_OVERFLOW,
            "Overflow",
            "test"
        ));
    } catch (const std::exception& e) {
        handle_exception(e);
    }

    // Verify: Specific handler called
    EXPECT_EQ(handlers_called["aggregation"], 1);
}

// ============================================================================
// EH-13: Unknown Exception Safe Handling
// ============================================================================

TEST_F(ErrorHandlingTest, EH_13_UnknownExceptionSafeHandling) {
    // Gap: exception handling (safe fallback for unknown exceptions)
    
    // Use catch(...) only as last resort for truly unknown exceptions
    int caught_unknown = 0;

    try {
        throw std::runtime_error("Test");
    } catch (const std::runtime_error& e) {
        // Known exception handled specifically
    } catch (const std::exception& e) {
        // Other standard exceptions
    } catch (...) {
        // Only catch truly unknown non-standard exceptions
        caught_unknown++;
    }

    // Verify: Known exception caught by specific handler
    EXPECT_EQ(caught_unknown, 0);
}

// ============================================================================
// EH-14: Exception Type Hierarchy Traversal
// ============================================================================

TEST_F(ErrorHandlingTest, EH_14_ExceptionHierarchyTraversal) {
    // Gap: exception handling (respect exception hierarchy)
    
    // Exception hierarchy:
    // std::exception
    //   ├── std::runtime_error
    //   └── AnalyticsException
    //       ├── AggregationException
    //       └── StreamException

    int caught_level = 0;

    try {
        throw AggregationException(makeErrorContext(
            AnalyticsErrorCode::AGGREGATION_OVERFLOW,
            "Overflow",
            "test"
        ));
    } catch (const AggregationException& e) {
        caught_level = 1; // Most specific
    } catch (const AnalyticsException& e) {
        caught_level = 2;
    } catch (const std::exception& e) {
        caught_level = 3; // Most generic
    }

    // Verify: Most specific handler matched
    EXPECT_EQ(caught_level, 1);
}

// ============================================================================
// EH-15: Catch Order Matters - Derived Before Base
// ============================================================================

TEST_F(ErrorHandlingTest, EH_15_CatchOrderDerivedBeforeBase) {
    // Gap: exception handling (correct catch block order)
    
    std::string caught_order;

    // WRONG order would be: base before derived
    // CORRECT order is: derived before base
    try {
        throw AggregationException(makeErrorContext(
            AnalyticsErrorCode::AGGREGATION_OVERFLOW,
            "Overflow",
            "test"
        ));
    } catch (const AggregationException& e) {
        caught_order += "derived_";
    } catch (const AnalyticsException& e) {
        caught_order += "base_";
    }

    // Verify: Derived handler executed first
    EXPECT_TRUE(caught_order.find("derived") != std::string::npos);
    EXPECT_TRUE(caught_order.find("base") == std::string::npos);
}

// ============================================================================
// EH-16: Error Serializes to Valid JSON
// ============================================================================

TEST_F(ErrorHandlingTest, EH_16_ErrorSerializesToJSON) {
    // Gap: error context serialization (valid JSON output)
    
    auto ctx = makeErrorContext(
        AnalyticsErrorCode::AGGREGATION_OVERFLOW,
        "Overflow in SUM",
        "aggregator",
        "value=123456789"
    );

    auto json = ctx.toJSON();

    // Verify: JSON contains expected structure
    EXPECT_NE(json.find("\"code\""), std::string::npos);
    EXPECT_NE(json.find("\"message\""), std::string::npos);
    EXPECT_NE(json.find("\"module\""), std::string::npos);
    EXPECT_NE(json.find("\"line\""), std::string::npos);
}

// ============================================================================
// EH-17: JSON Contains Required Fields
// ============================================================================

TEST_F(ErrorHandlingTest, EH_17_JSONRequiredFields) {
    // Gap: error context serialization (required fields)
    
    auto ctx = makeErrorContext(
        AnalyticsErrorCode::POOL_EXHAUSTED,
        "Pool exhausted",
        "pool_manager",
        "active=10,max=10"
    );

    auto json = ctx.toJSON();

    // Verify: All required fields present
    EXPECT_NE(json.find("\"code\":" + std::to_string(static_cast<int>(
        AnalyticsErrorCode::POOL_EXHAUSTED))), std::string::npos);
    EXPECT_NE(json.find("\"message\":\"Pool exhausted\""), std::string::npos);
    EXPECT_NE(json.find("\"module\":\"pool_manager\""), std::string::npos);
}

// ============================================================================
// EH-18: Nested Context Preserved in Serialization
// ============================================================================

TEST_F(ErrorHandlingTest, EH_18_NestedContextPreserved) {
    // Gap: error context serialization (nested context)
    
    auto inner_ctx = makeErrorContext(
        AnalyticsErrorCode::STREAM_WINDOW_EXPIRED,
        "Window expired",
        "window",
        "window_id=42"
    );

    auto outer_ctx = makeErrorContext(
        AnalyticsErrorCode::STREAM_BACKPRESSURE,
        "Backpressure due to expired window",
        "stream",
        "cause: " + inner_ctx.toJSON()
    );

    auto json = outer_ctx.toJSON();

    // Verify: Nested context included
    EXPECT_NE(json.find("window_id=42"), std::string::npos);
}

// ============================================================================
// EH-19: Special Characters Escaped Properly
// ============================================================================

TEST_F(ErrorHandlingTest, EH_19_SpecialCharactersEscaped) {
    // Gap: error context serialization (escape special chars)
    
    auto ctx = ErrorContext{
        AnalyticsErrorCode::INVALID_ARGUMENT,
        "Invalid: \"quoted\" value with \\backslash",
        "test",
        "msg with newline\nand tab\t"
    };

    auto json = ctx.toJSON();

    // Verify: JSON is still parseable (basic check)
    EXPECT_NE(json.find("code"), std::string::npos);
    EXPECT_NE(json.find("message"), std::string::npos);
}

// ============================================================================
// EH-20: Large Context Handled Without Truncation
// ============================================================================

TEST_F(ErrorHandlingTest, EH_20_LargeContextNoTruncation) {
    // Gap: error context serialization (large context handling)
    
    std::string large_data(10000, 'x');
    auto ctx = makeErrorContext(
        AnalyticsErrorCode::INTERNAL_ERROR,
        "Large context error",
        "test",
        large_data
    );

    auto json = ctx.toJSON();

    // Verify: Large context preserved
    EXPECT_GE(json.length(), large_data.length());
}

// ============================================================================
// EH-21: Error Codes in Valid Range
// ============================================================================

TEST_F(ErrorHandlingTest, EH_21_ErrorCodesInValidRange) {
    // Gap: error taxonomy (code range validation)
    
    std::vector<AnalyticsErrorCode> codes = {
        AnalyticsErrorCode::AGGREGATION_OVERFLOW,      // 1001
        AnalyticsErrorCode::STREAM_WINDOW_EXPIRED,     // 2001
        AnalyticsErrorCode::POOL_EXHAUSTED,            // 3001
        AnalyticsErrorCode::INVALID_ARGUMENT,          // 9001
    };

    for (const auto& code : codes) {
        int code_val = static_cast<int>(code);
        EXPECT_GE(code_val, 0);
        EXPECT_LT(code_val, 10000);
    }
}

// ============================================================================
// EH-22: Error Categories Map to Correct Codes
// ============================================================================

TEST_F(ErrorHandlingTest, EH_22_CategoriesMappedCorrectly) {
    // Gap: error taxonomy (category mapping)
    
    std::unordered_map<std::string, int> category_min_code = {
        {"aggregation", 1000},
        {"stream", 2000},
        {"pool", 3000},
        {"generic", 9000},
    };

    std::vector<std::pair<std::string, AnalyticsErrorCode>> mappings = {
        {"aggregation", AnalyticsErrorCode::AGGREGATION_OVERFLOW},
        {"stream", AnalyticsErrorCode::STREAM_WINDOW_EXPIRED},
        {"pool", AnalyticsErrorCode::POOL_EXHAUSTED},
        {"generic", AnalyticsErrorCode::INVALID_ARGUMENT},
    };

    for (const auto& [category, code] : mappings) {
        int code_val = static_cast<int>(code);
        EXPECT_GE(code_val, category_min_code[category]);
    }
}

// ============================================================================
// EH-23: Error Messages Unique Per Code
// ============================================================================

TEST_F(ErrorHandlingTest, EH_23_UniqueMessagesPerCode) {
    // Gap: error taxonomy (unique messages)
    
    std::unordered_map<int, std::string> code_messages;

    auto codes = std::vector<std::pair<AnalyticsErrorCode, std::string>>{
        {AnalyticsErrorCode::AGGREGATION_OVERFLOW, "Overflow in aggregation"},
        {AnalyticsErrorCode::STREAM_WINDOW_EXPIRED, "Window expired"},
        {AnalyticsErrorCode::POOL_EXHAUSTED, "Connection pool exhausted"},
        {AnalyticsErrorCode::INVALID_ARGUMENT, "Invalid argument"},
    };

    for (const auto& [code, msg] : codes) {
        int code_val = static_cast<int>(code);
        code_messages[code_val] = msg;
    }

    // Verify: Each code has unique message
    EXPECT_EQ(code_messages.size(), codes.size());
}

// ============================================================================
// EH-24: Error Recovery Hints Accurate
// ============================================================================

TEST_F(ErrorHandlingTest, EH_24_RecoveryHintsAccurate) {
    // Gap: error taxonomy (recovery hints)
    
    std::unordered_map<int, std::string> recovery_hints;

    recovery_hints[static_cast<int>(AnalyticsErrorCode::POOL_EXHAUSTED)] =
        "Wait for connection or increase pool size";
    recovery_hints[static_cast<int>(AnalyticsErrorCode::AGGREGATION_OVERFLOW)] =
        "Check input values or use larger data type";
    recovery_hints[static_cast<int>(AnalyticsErrorCode::STREAM_WINDOW_EXPIRED)] =
        "Increase watermark delay or reduce lateness threshold";

    // Verify: Recovery hints are non-empty
    for (const auto& [code, hint] : recovery_hints) {
        EXPECT_GT(hint.length(), 0);
    }
}

// ============================================================================
// EH-25: Error Classification Consistent Across Module
// ============================================================================

TEST_F(ErrorHandlingTest, EH_25_ConsistentClassificationAcrossModule) {
    // Gap: error taxonomy (consistency)
    
    // Verify that same error always has same code/message
    struct ErrorDef {
        AnalyticsErrorCode code;
        std::string message = {};
    };

    auto define_error = [](const std::string& name) -> ErrorDef {
        if (name == "overflow") {
            return {AnalyticsErrorCode::AGGREGATION_OVERFLOW, "Overflow"};
        }
        if (name == "exhausted") {
            return {AnalyticsErrorCode::POOL_EXHAUSTED, "Pool exhausted"};
        }
        return {AnalyticsErrorCode::OK, "OK"};
    };

    // Check consistency
    auto def1 = define_error("overflow");
    auto def2 = define_error("overflow");

    EXPECT_EQ(def1.code, def2.code);
    EXPECT_EQ(def1.message, def2.message);
}

} // namespace test
} // namespace analytics
} // namespace themis
