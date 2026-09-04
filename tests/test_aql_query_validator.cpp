/**
 * @file test_aql_query_validator.cpp
 * @brief Unit tests for AQLQueryValidator and scoreQueryConfidence
 */

#include <gtest/gtest.h>
#include "aql/aql_query_validator.h"
#include "aql/aql_query_builder.h"
#include "aql/aql_schema_provider.h"
#include "aql/aql_fewshot_example_library.h"
#include "aql/aql_lora_finetuner.h"
#include "aql/docs_assistant_functions.h"
#include "aql/llm_aql_handler.h"
#include "aql/llm_error_codes.h"
#include <cmath>
#include <numeric>

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
// Traversal depth validation tests
// ============================================================================

TEST_F(AQLQueryValidatorStringTest, TraversalValidDepthRange) {
    // min=1, max=3 is valid
    auto result = validator.validate(
        "FOR v, e, p IN 1..3 OUTBOUND \"start/1\" GRAPH \"g\" RETURN v"
    );
    bool has_depth_error = false;
    for (const auto& issue : result.issues) {
        if (issue.clause == "FOR" && issue.severity == ValidationIssue::Severity::ERROR) {
            has_depth_error = true;
            break;
        }
    }
    EXPECT_FALSE(has_depth_error);
}

TEST_F(AQLQueryValidatorStringTest, TraversalInvalidDepthRangeIsError) {
    // min=5, max=2 is invalid (min > max)
    auto result = validator.validate(
        "FOR v, e, p IN 5..2 OUTBOUND \"start/1\" GRAPH \"g\" RETURN v"
    );
    EXPECT_FALSE(result.is_valid);
    EXPECT_TRUE(result.hasErrors());
    bool has_depth_error = false;
    for (const auto& issue : result.issues) {
        if (issue.clause == "FOR" && issue.severity == ValidationIssue::Severity::ERROR) {
            has_depth_error = true;
            break;
        }
    }
    EXPECT_TRUE(has_depth_error);
}

TEST(AQLQueryBuilderValidateTest, TraverseBuilderValidateDepthError) {
    // The builder rejects min > max at construction time
    AQLQueryBuilder builder;
    EXPECT_THROW(
        builder.forTraverse("v", "e", "p", "start", "g", "OUTBOUND", 5, 2),
        std::invalid_argument
    );
}

TEST(AQLQueryBuilderValidateTest, TraverseBuilderValidateValidDepth) {
    AQLQueryBuilder builder;
    builder.forTraverse("v", "e", "p", "\"start/1\"", "myGraph", "OUTBOUND", 1, 4).ret("v");

    ValidationResult result = builder.validate();
    EXPECT_FALSE(result.hasErrors());
    EXPECT_TRUE(result.is_valid);
}

// ============================================================================
// DML string validation tests
// ============================================================================

TEST_F(AQLQueryValidatorStringTest, InsertQueryIsValid) {
    // Standalone INSERT does not need FOR
    auto result = validator.validate("INSERT {name: \"Alice\"} INTO users");
    EXPECT_TRUE(result.is_valid);
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLQueryValidatorStringTest, RemoveQueryWithForIsValid) {
    auto result = validator.validate(
        "FOR u IN users FILTER u.active == false REMOVE u IN users"
    );
    EXPECT_TRUE(result.is_valid);
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLQueryValidatorStringTest, UpsertQueryIsValid) {
    auto result = validator.validate(
        "UPSERT {name: \"Alice\"} INSERT {name: \"Alice\", age: 30} UPDATE {age: 30} IN users"
    );
    EXPECT_TRUE(result.is_valid);
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLQueryValidatorStringTest, StandaloneRemoveByKeyIsValid) {
    // REMOVE "key123" IN users — standalone, no FOR required
    auto result = validator.validate("REMOVE \"key123\" IN users");
    EXPECT_TRUE(result.is_valid);
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLQueryValidatorStringTest, StandaloneUpdateByKeyIsValid) {
    // UPDATE "key123" WITH {age: 31} IN users — standalone, no FOR required
    auto result = validator.validate("UPDATE \"key123\" WITH {age: 31} IN users");
    EXPECT_TRUE(result.is_valid);
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(AQLQueryValidatorStringTest, StandaloneReplaceByKeyIsValid) {
    // REPLACE "key123" WITH {name: \"Bob\"} IN users — standalone, no FOR required
    auto result = validator.validate("REPLACE \"key123\" WITH {name: \"Bob\"} IN users");
    EXPECT_TRUE(result.is_valid);
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

// ============================================================================
// Feature 12: Schema-aware validate(query, schema) overload
// ============================================================================

class SchemaAwareValidatorTest : public ::testing::Test {
protected:
    AQLQueryValidator validator;
};

TEST_F(SchemaAwareValidatorTest, ValidQueryWithKnownCollectionNoWarning) {
    std::vector<CollectionMetadata> schema;
    schema.push_back({"users", "document", {{"name", "string"}, {"age", "integer"}}});

    const std::string query =
        "FOR u IN users FILTER u.name == \"Alice\" RETURN u";
    auto result = validator.validate(query, schema);

    // No unknown-collection warnings expected
    bool has_unknown_collection = false;
    for (const auto& issue : result.issues) {
        if (issue.message.find("not present in the schema") != std::string::npos) {
            has_unknown_collection = true;
        }
    }
    EXPECT_FALSE(has_unknown_collection);
}

TEST_F(SchemaAwareValidatorTest, UnknownCollectionProducesWarning) {
    std::vector<CollectionMetadata> schema;
    schema.push_back({"users", "document", {{"name", "string"}}});

    const std::string query =
        "FOR o IN orders RETURN o";
    auto result = validator.validate(query, schema);

    bool has_unknown_collection = false;
    for (const auto& issue : result.issues) {
        if (issue.message.find("orders") != std::string::npos &&
            issue.message.find("not present") != std::string::npos) {
            has_unknown_collection = true;
        }
    }
    EXPECT_TRUE(has_unknown_collection);
    // The query is still structurally valid
    EXPECT_FALSE(result.hasErrors());
}

TEST_F(SchemaAwareValidatorTest, UnknownFieldProducesWarning) {
    CollectionMetadata meta;
    meta.name = "users";
    meta.type = "document";
    meta.fields.push_back({"name", "string"});
    meta.fields.push_back({"age", "integer"});

    std::vector<CollectionMetadata> schema = {meta};

    const std::string query =
        "FOR u IN users FILTER u.nonexistent == 42 RETURN u";
    auto result = validator.validate(query, schema);

    bool has_unknown_field = false;
    for (const auto& issue : result.issues) {
        if (issue.message.find("nonexistent") != std::string::npos) {
            has_unknown_field = true;
        }
    }
    EXPECT_TRUE(has_unknown_field);
}

TEST_F(SchemaAwareValidatorTest, KnownFieldNoWarning) {
    CollectionMetadata meta;
    meta.name = "users";
    meta.type = "document";
    meta.fields.push_back({"name", "string"});
    meta.fields.push_back({"city", "string"});

    std::vector<CollectionMetadata> schema = {meta};

    const std::string query =
        "FOR u IN users FILTER u.city == \"Seattle\" RETURN u.name";
    auto result = validator.validate(query, schema);

    for (const auto& issue : result.issues) {
        EXPECT_EQ(issue.message.find("not a known field"), std::string::npos)
            << "Unexpected unknown-field warning: " << issue.message;
    }
}

TEST_F(SchemaAwareValidatorTest, EmptySchemaSkipsSchemaChecks) {
    std::vector<CollectionMetadata> schema;
    const std::string query =
        "FOR u IN unknown_collection RETURN u";
    auto result = validator.validate(query, schema);

    for (const auto& issue : result.issues) {
        EXPECT_EQ(issue.message.find("not present in the schema"), std::string::npos)
            << "Should not emit schema warnings when schema is empty";
    }
}

// ============================================================================
// Feature 13: Runtime-overridable ValidationLimitsConfig
// ============================================================================

class ValidationLimitsConfigTest : public ::testing::Test {
protected:
    LLMAQLHandler handler;
};

TEST_F(ValidationLimitsConfigTest, DefaultsMatchCompileTimeConstants) {
    auto cfg = handler.getValidationLimits();
    EXPECT_EQ(cfg.max_nl_query_length,       ValidationLimits::MAX_NL_QUERY_LENGTH);
    EXPECT_EQ(cfg.max_schema_context_length, ValidationLimits::MAX_SCHEMA_CONTEXT_LENGTH);
    EXPECT_EQ(cfg.max_prompt_length,         ValidationLimits::MAX_PROMPT_LENGTH);
    EXPECT_EQ(cfg.max_rag_top_k,             ValidationLimits::MAX_RAG_TOP_K);
    EXPECT_EQ(cfg.min_rag_top_k,             ValidationLimits::MIN_RAG_TOP_K);
    EXPECT_EQ(cfg.default_timeout_seconds,   ValidationLimits::DEFAULT_TIMEOUT_SECONDS);
}

TEST_F(ValidationLimitsConfigTest, SetAndGetRoundTrips) {
    ValidationLimitsConfig cfg;
    cfg.max_nl_query_length       = 512;
    cfg.max_schema_context_length = 4096;
    cfg.max_prompt_length         = 8192;

    handler.setValidationLimits(cfg);
    auto got = handler.getValidationLimits();

    EXPECT_EQ(got.max_nl_query_length,       512u);
    EXPECT_EQ(got.max_schema_context_length, 4096u);
    EXPECT_EQ(got.max_prompt_length,         8192u);
}

TEST_F(ValidationLimitsConfigTest, EnforcedAfterSet) {
    ValidationLimitsConfig cfg;
    cfg.max_nl_query_length = 5;  // Very tight limit for testing

    handler.setValidationLimits(cfg);

    // A query longer than 5 chars should be rejected
    EXPECT_THROW(
        handler.translateNLToAQL("Find all users older than 30"),
        LLMException
    );
}

// ============================================================================
// Feature 14: LoRA named constants and fromOptions()
// ============================================================================

TEST(AQLLoRAFinetunerConfigTest, DefaultsMatchNamedConstants) {
    AQLLoRAFinetuner::Config cfg;
    EXPECT_EQ(cfg.hyperparameters.rank,           AQLLoRAFinetuner::Config::kDefaultRank);
    EXPECT_FLOAT_EQ(cfg.hyperparameters.alpha,    AQLLoRAFinetuner::Config::kDefaultAlpha);
    EXPECT_FLOAT_EQ(cfg.hyperparameters.dropout,  AQLLoRAFinetuner::Config::kDefaultDropout);
    EXPECT_EQ(cfg.hyperparameters.batch_size,     AQLLoRAFinetuner::Config::kDefaultBatchSize);
    EXPECT_EQ(cfg.hyperparameters.num_epochs,     AQLLoRAFinetuner::Config::kDefaultEpochs);
    EXPECT_EQ(cfg.hyperparameters.max_seq_length, AQLLoRAFinetuner::Config::kDefaultMaxSeqLength);
    EXPECT_EQ(cfg.hyperparameters.warmup_steps,   AQLLoRAFinetuner::Config::kDefaultWarmupSteps);
}

TEST(AQLLoRAFinetunerConfigTest, FromOptionsOverridesRank) {
    auto cfg = AQLLoRAFinetuner::Config::fromOptions({{"rank", "16"}});
    EXPECT_EQ(cfg.hyperparameters.rank, 16);
    // Other fields should still be defaults
    EXPECT_EQ(cfg.hyperparameters.num_epochs, AQLLoRAFinetuner::Config::kDefaultEpochs);
}

TEST(AQLLoRAFinetunerConfigTest, FromOptionsOverridesMultipleFields) {
    auto cfg = AQLLoRAFinetuner::Config::fromOptions({
        {"rank",          "4"},
        {"epochs",        "10"},
        {"batch_size",    "8"},
        {"learning_rate", "0.0001"},
        {"dropout",       "0.1"},
    });
    EXPECT_EQ(cfg.hyperparameters.rank,        4);
    EXPECT_EQ(cfg.hyperparameters.num_epochs,  10);
    EXPECT_EQ(cfg.hyperparameters.batch_size,  8);
    EXPECT_NEAR(cfg.hyperparameters.learning_rate, 0.0001f, 1e-7f);
    EXPECT_FLOAT_EQ(cfg.hyperparameters.dropout, 0.1f);
}

TEST(AQLLoRAFinetunerConfigTest, FromOptionsInvalidRankThrows) {
    EXPECT_THROW(
        AQLLoRAFinetuner::Config::fromOptions({{"rank", "0"}}),
        std::invalid_argument
    );
    EXPECT_THROW(
        AQLLoRAFinetuner::Config::fromOptions({{"rank", "512"}}),
        std::invalid_argument
    );
}

TEST(AQLLoRAFinetunerConfigTest, FromOptionsInvalidDropoutThrows) {
    EXPECT_THROW(
        AQLLoRAFinetuner::Config::fromOptions({{"dropout", "1.0"}}),
        std::invalid_argument
    );
    EXPECT_THROW(
        AQLLoRAFinetuner::Config::fromOptions({{"dropout", "-0.1"}}),
        std::invalid_argument
    );
}

TEST(AQLLoRAFinetunerConfigTest, FromOptionsUnknownKeysIgnored) {
    EXPECT_NO_THROW({
        auto cfg = AQLLoRAFinetuner::Config::fromOptions({{"unknown_key", "42"}});
        // Defaults intact
        EXPECT_EQ(cfg.hyperparameters.rank, AQLLoRAFinetuner::Config::kDefaultRank);
    });
}

// ============================================================================
// Feature 15: DocsAssistantFunctions degraded-mode reporting
// ============================================================================

TEST(DocsAssistantDegradedTest, NotReadyReportsReason) {
    DocsAssistantFunctions daf;
    // In the test environment there is no docs database, so the assistant
    // will be in degraded mode.  We just check that the API contract holds.
    if (!daf.isReady()) {
        std::string reason = daf.degradedReason();
        // Reason must be non-empty when not ready
        EXPECT_FALSE(reason.empty());
    }
}

TEST(DocsAssistantDegradedTest, IsFullyReadyFalseWhenNotReady) {
    DocsAssistantFunctions daf = {};
    if (!daf.isReady()) {
        EXPECT_FALSE(daf.isFullyReady());
    }
}

// ============================================================================
// Feature 10: stripMarkdownFences helper
// ============================================================================

TEST(StripMarkdownFencesTest, NoFenceReturnedUnchanged) {
    // Access via public translateNLToAQL with a mock executor that returns plain text
    LLMAQLHandler handler;
    handler.setChatExecutor([](const std::vector<themis::llm::ChatMessage>&) {
        return "FOR u IN users RETURN u";
    });
    auto result = handler.translateNLToAQL("find all users");
    EXPECT_EQ(result, "FOR u IN users RETURN u");
}

TEST(StripMarkdownFencesTest, AQLFenceStripped) {
    LLMAQLHandler handler;
    handler.setChatExecutor([](const std::vector<themis::llm::ChatMessage>&) {
        return "```aql\nFOR u IN users RETURN u\n```";
    });
    auto result = handler.translateNLToAQL("find all users");
    EXPECT_EQ(result, "FOR u IN users RETURN u");
}

TEST(StripMarkdownFencesTest, PlainFenceStripped) {
    LLMAQLHandler handler;
    handler.setChatExecutor([](const std::vector<themis::llm::ChatMessage>&) {
        return "```\nFOR u IN users RETURN u\n```";
    });
    auto result = handler.translateNLToAQL("find all users");
    EXPECT_EQ(result, "FOR u IN users RETURN u");
}

// ============================================================================
// Feature 8: Semantic Few-Shot Example Selection
// ============================================================================

namespace {

/// Minimal stub embedding provider that returns a deterministic embedding
/// based on the hash of the input.
class StubEmbeddingProvider : public IEmbeddingProvider {
public:
    explicit StubEmbeddingProvider(std::size_t dim = 4) : dim_(dim) {}

    std::vector<float> embed(const std::string& text) override {
        // Produce a simple deterministic unit vector
        std::vector<float> v(dim_, 0.0f);
        if (text.empty()) {
          return v;
        }
        // Use character sum to create a simple embedding
        float angle = static_cast<float>(
            std::accumulate(text.begin(), text.end(), 0u,
                [](unsigned acc, char c) { return acc + static_cast<unsigned>(c); })
        );
        v[0] = std::cos(angle);
        v[1] = std::sin(angle);
        // Normalise
        float norm = std::sqrt(v[0]*v[0] + v[1]*v[1]);
        if (norm > 1e-6f) { v[0] /= norm; v[1] /= norm; }
        return v;
    }

private:
    std::size_t dim_;
};

} // anonymous namespace

TEST(AQLFewShotSemanticTest, FindRelevantWithProviderReturnsResults) {
    AQLFewShotExampleLibrary lib;
    StubEmbeddingProvider provider;
    lib.setEmbeddingProvider(&provider);

    auto results = lib.findRelevant("Find all users", 3);
    EXPECT_GT(results.size(), 0u);
    EXPECT_LE(results.size(), 3u);
}

TEST(AQLFewShotSemanticTest, RebuildEmbeddingIndexDoesNotThrow) {
    AQLFewShotExampleLibrary lib;
    StubEmbeddingProvider provider;
    lib.setEmbeddingProvider(&provider);
    EXPECT_NO_THROW(lib.rebuildEmbeddingIndex());
}

TEST(AQLFewShotSemanticTest, SetNullProviderRevertsToJaccard) {
    AQLFewShotExampleLibrary lib;
    StubEmbeddingProvider provider;
    lib.setEmbeddingProvider(&provider);
    lib.setEmbeddingProvider(nullptr);

    // After clearing, findRelevant should still work (Jaccard fallback)
    auto results = lib.findRelevant("Find all users", 3);
    EXPECT_GT(results.size(), 0u);
}

TEST(AQLFewShotSemanticTest, RebuildIndexWithNullProviderIsNoop) {
    AQLFewShotExampleLibrary lib;
    // No provider set; rebuildEmbeddingIndex should be a safe no-op
    EXPECT_NO_THROW(lib.rebuildEmbeddingIndex());
}
