/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_query_validator.cpp                       ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-22 08:56:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     323                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_aql_query_validator.cpp
 * @brief Unit tests for AQLQueryValidator and scoreQueryConfidence
 */

#include <gtest/gtest.h>
#include "aql/aql_query_validator.h"
#include "aql/aql_query_builder.h"
#include "aql/llm_aql_handler.h"

using namespace themis::aql;

// ============================================================================
// ValidationResult helper tests
// ============================================================================

TEST(ValidationResultTest, EmptyHasNoErrorsOrWarnings) {
    ValidationResult r;
    r.is_valid = true;
    EXPECT_FALSE(r.hasErrors());
    EXPECT_FALSE(r.hasWarnings());
    EXPECT_EQ(r.summary(), "OK");
}

TEST(ValidationResultTest, SummaryOneError) {
    ValidationResult r;
    r.is_valid = false;
    r.issues.push_back({ValidationIssue::Severity::ERROR, "Missing RETURN", "RETURN"});
    EXPECT_TRUE(r.hasErrors());
    EXPECT_FALSE(r.hasWarnings());
    EXPECT_EQ(r.summary(), "1 error");
}

TEST(ValidationResultTest, SummaryMultipleIssues) {
    ValidationResult r;
    r.is_valid = false;
    r.issues.push_back({ValidationIssue::Severity::ERROR,   "err",  "FOR"});
    r.issues.push_back({ValidationIssue::Severity::WARNING, "warn", "FILTER"});
    r.issues.push_back({ValidationIssue::Severity::INFO,    "hint", "LIMIT"});
    EXPECT_TRUE(r.hasErrors());
    EXPECT_TRUE(r.hasWarnings());
    std::string s = r.summary();
    EXPECT_NE(s.find("1 error"),   std::string::npos);
    EXPECT_NE(s.find("1 warning"), std::string::npos);
    EXPECT_NE(s.find("1 hint"),    std::string::npos);
}

// ============================================================================
// String validation tests
// ============================================================================

class AQLQueryValidatorStringTest : public ::testing::Test {
protected:
    AQLQueryValidator validator;
};

TEST_F(AQLQueryValidatorStringTest, EmptyQueryIsError) {
    auto result = validator.validate("");
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(AQLQueryValidatorStringTest, ValidSimpleQuery) {
    auto result = validator.validate("FOR doc IN users RETURN doc");
    EXPECT_TRUE(result.is_valid);
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLQueryValidatorStringTest, MissingReturnIsError) {
    auto result = validator.validate("FOR doc IN users FILTER doc.age > 18");
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.hasErrors());
    bool found = false;
    for (const auto& issue : result.issues) {
        if (issue.clause == "RETURN") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(AQLQueryValidatorStringTest, MissingForIsError) {
    auto result = validator.validate("RETURN doc");
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(AQLQueryValidatorStringTest, LimitZeroIsWarning) {
    auto result = validator.validate("FOR doc IN users LIMIT 0 RETURN doc");
    // is_valid may still be true (no errors), but should have a warning
    bool has_limit_warning = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::WARNING &&
            issue.clause == "LIMIT") {
            has_limit_warning = true;
            break;
        }
    }
    EXPECT_TRUE(has_limit_warning);
}

TEST_F(AQLQueryValidatorStringTest, CollectAfterSortIsWarning) {
    // SORT appears before COLLECT in the string
    auto result = validator.validate(
        "FOR o IN orders SORT o.amount ASC COLLECT city = o.city RETURN city"
    );
    bool has_collect_warning = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::WARNING &&
            issue.clause == "COLLECT") {
            has_collect_warning = true;
            break;
        }
    }
    EXPECT_TRUE(has_collect_warning);
}

TEST_F(AQLQueryValidatorStringTest, NoFilterAndNoLimitIsInfoHint) {
    auto result = validator.validate("FOR doc IN users RETURN doc");
    bool has_limit_hint = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::INFO &&
            issue.clause == "LIMIT") {
            has_limit_hint = true;
            break;
        }
    }
    EXPECT_TRUE(has_limit_hint);
}

TEST_F(AQLQueryValidatorStringTest, QueryWithFilterSuppressesLimitHint) {
    auto result = validator.validate(
        "FOR u IN users FILTER u.age > 18 RETURN u"
    );
    bool has_limit_hint = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::INFO &&
            issue.clause == "LIMIT") {
            has_limit_hint = true;
            break;
        }
    }
    EXPECT_FALSE(has_limit_hint);
}

TEST_F(AQLQueryValidatorStringTest, QueryWithLimitSuppressesLimitHint) {
    auto result = validator.validate(
        "FOR u IN users LIMIT 100 RETURN u"
    );
    bool has_limit_hint = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::INFO &&
            issue.clause == "LIMIT") {
            has_limit_hint = true;
            break;
        }
    }
    EXPECT_FALSE(has_limit_hint);
}

TEST_F(AQLQueryValidatorStringTest, AssignmentInFilterIsWarning) {
    // Single = in FILTER position (should be ==)
    auto result = validator.validate(
        "FOR u IN users FILTER u.name = \"Alice\" RETURN u"
    );
    bool has_filter_warning = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::WARNING &&
            issue.clause == "FILTER") {
            has_filter_warning = true;
            break;
        }
    }
    EXPECT_TRUE(has_filter_warning);
}

TEST_F(AQLQueryValidatorStringTest, CorrectEqualityOperatorNoWarning) {
    auto result = validator.validate(
        "FOR u IN users FILTER u.name == \"Alice\" RETURN u"
    );
    bool has_filter_warning = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::WARNING &&
            issue.clause == "FILTER") {
            has_filter_warning = true;
            break;
        }
    }
    EXPECT_FALSE(has_filter_warning);
}

// ============================================================================
// Builder validation tests
// ============================================================================

class AQLQueryValidatorBuilderTest : public ::testing::Test {
protected:
    AQLQueryValidator validator;
    AQLQueryBuilder   builder;
};

TEST_F(AQLQueryValidatorBuilderTest, EmptyBuilderIsValid) {
    auto result = validator.validate(builder);
    // Empty builder has no clauses, so nothing to complain about
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLQueryValidatorBuilderTest, CompleteBuilderNoIssues) {
    builder.forIn("u", "users").filter("u.age > 18").limit(10).ret("u");
    auto result = validator.validate(builder);
    EXPECT_TRUE(result.is_valid);
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLQueryValidatorBuilderTest, CompleteBuilderWithoutLimitGetsHint) {
    builder.forIn("u", "users").ret("u");
    auto result = validator.validate(builder);
    EXPECT_TRUE(result.is_valid);  // no errors
    bool has_limit_hint = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::INFO &&
            issue.clause == "LIMIT") {
            has_limit_hint = true;
            break;
        }
    }
    EXPECT_TRUE(has_limit_hint);
}

TEST_F(AQLQueryValidatorBuilderTest, LimitZeroWarningFromBuilder) {
    builder.forIn("u", "users").limit(0).ret("u");
    auto result = validator.validate(builder);
    bool has_warn = false;
    for (const auto& issue : result.issues) {
        if (issue.severity == ValidationIssue::Severity::WARNING &&
            issue.clause == "LIMIT") {
            has_warn = true;
            break;
        }
    }
    EXPECT_TRUE(has_warn);
}

// ============================================================================
// AQLQueryBuilder::validate() integration test
// ============================================================================

TEST(AQLQueryBuilderValidateTest, ValidateMethodReturnsResult) {
    AQLQueryBuilder builder;
    builder.forIn("doc", "documents").limit(50).ret("doc");

    ValidationResult result = builder.validate();
    // No errors expected
    EXPECT_FALSE(result.hasErrors());
    EXPECT_EQ(result.is_valid, true);
}

TEST(AQLQueryBuilderValidateTest, IncompleteBuilderValidateNoError) {
    AQLQueryBuilder builder;
    builder.forIn("doc", "documents");

    ValidationResult result = builder.validate();
    // Builder is not yet complete, but structurally valid so far
    EXPECT_FALSE(result.hasErrors());
}

// ============================================================================
// scoreQueryConfidence tests (graceful LLM absence)
// ============================================================================

class ScoreQueryConfidenceTest : public ::testing::Test {
protected:
    LLMAQLHandler handler;
};

TEST_F(ScoreQueryConfidenceTest, EmptyQueryReturnsZeroScore) {
    auto score = handler.scoreQueryConfidence("");
    EXPECT_NEAR(score.score, 0.0f, 1e-6f);
    EXPECT_FALSE(score.explanation.empty());
}

TEST_F(ScoreQueryConfidenceTest, ValidQueryWithNoModelReturnsUnavailable) {
    // Without a loaded model the score should be -1 (unavailable)
    auto score = handler.scoreQueryConfidence(
        "FOR u IN users FILTER u.age > 18 RETURN u"
    );
    // Either -1 (model not loaded) or a valid [0,1] score
    EXPECT_TRUE(score.score >= -1.0f);
    EXPECT_TRUE(score.score <= 1.0f || score.score == -1.0f);
}

TEST_F(ScoreQueryConfidenceTest, ScoreWithIntentAndSchema) {
    std::string query  = "FOR u IN users FILTER u.city == \"Berlin\" RETURN u";
    std::string intent = "find users in Berlin";
    std::string schema = "Collections:\n- users: {name, email, city}\n";
    auto score = handler.scoreQueryConfidence(query, intent, schema);
    // Accept both unavailable (-1) and valid scores
    EXPECT_TRUE(score.score >= -1.0f && score.score <= 1.0f);
    EXPECT_FALSE(score.explanation.empty());
}

TEST_F(ScoreQueryConfidenceTest, ScoreDoesNotThrow) {
    EXPECT_NO_THROW({
        handler.scoreQueryConfidence("FOR x IN col RETURN x");
    });
}
