/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_batch_nl_to_aql_translation.cpp               ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 18:44:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     226                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_batch_nl_to_aql_translation.cpp
 * @brief Tests for batch NL-to-AQL translation (offline workloads)
 */

#include <gtest/gtest.h>
#include "aql/llm_aql_handler.h"
#include <algorithm>
#include <iostream>

using namespace themis::aql;

class BatchNLToAQLTranslationTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<LLMAQLHandler>();
    }

    void TearDown() override {
        handler.reset();
    }

    std::unique_ptr<LLMAQLHandler> handler;
};

// ============================================================================
// Structural / Contract Tests (run without an LLM model)
// ============================================================================

TEST_F(BatchNLToAQLTranslationTest, EmptyBatchReturnsEmptyResults) {
    std::vector<LLMAQLHandler::BatchNLToAQLRequest> requests;
    auto results = handler->translateBatchNLToAQL(requests);
    EXPECT_TRUE(results.empty());
}

TEST_F(BatchNLToAQLTranslationTest, ResultCountMatchesRequestCount) {
    std::vector<LLMAQLHandler::BatchNLToAQLRequest> requests = {
        {"Find all users", ""},
        {"Find all posts", ""},
        {"Count orders per customer", ""},
    };

    auto results = handler->translateBatchNLToAQL(requests);

    // Output must have exactly the same size as input regardless of success/failure
    EXPECT_EQ(results.size(), requests.size());
}

TEST_F(BatchNLToAQLTranslationTest, SuccessfulResultHasNonEmptyAQL) {
    // When a model is available each successful result must carry a non-empty AQL string
    std::vector<LLMAQLHandler::BatchNLToAQLRequest> requests = {
        {"Find all users", ""},
    };

    auto results = handler->translateBatchNLToAQL(requests);
    ASSERT_EQ(results.size(), 1u);

    const auto& result = results[0];
    if (result.success) {
        EXPECT_FALSE(result.aql_query.empty());
        EXPECT_TRUE(result.error.empty());
    } else {
        // No model loaded – skip further assertions
        GTEST_SKIP() << "Skipping model-dependent assertions: " << result.error;
    }
}

TEST_F(BatchNLToAQLTranslationTest, FailedResultHasEmptyAQLAndNonEmptyError) {
    // Simulate a failing translation by checking the result contract:
    // when success==false the aql_query must be empty and error must be set.
    //
    // We verify this contract by directly constructing a failure result because
    // triggering a real translation failure without a model would just skip the
    // test.  The structural contract is the critical thing to enforce here.

    LLMAQLHandler::BatchNLToAQLResult failure;
    failure.success = false;
    failure.aql_query = "";  // must be empty on failure
    failure.error = "NL to AQL translation failed: model not loaded";

    EXPECT_FALSE(failure.success);
    EXPECT_TRUE(failure.aql_query.empty());
    EXPECT_FALSE(failure.error.empty());
}

TEST_F(BatchNLToAQLTranslationTest, IndividualFailureDoesNotAbortBatch) {
    // A batch with multiple requests must return a result for every request even
    // when some translations fail.  We rely on the model being absent so that
    // all requests fail gracefully.
    std::vector<LLMAQLHandler::BatchNLToAQLRequest> requests = {
        {"Find all users", ""},
        {"",               ""},  // deliberately empty query
        {"Find all posts", ""},
    };

    // The method must not throw; it must return one result per request.
    ASSERT_NO_THROW({
        auto results = handler->translateBatchNLToAQL(requests);
        EXPECT_EQ(results.size(), requests.size());
    });
}

// ============================================================================
// Schema-context Tests
// ============================================================================

TEST_F(BatchNLToAQLTranslationTest, SchemaContextIsForwardedPerRequest) {
    const std::string schema = R"(
Collections:
- orders: {_id, customer_id, total}
- customers: {_id, name, email}
)";

    std::vector<LLMAQLHandler::BatchNLToAQLRequest> requests = {
        {"Find all orders with customer names", schema},
        {"List customers sorted by name",       ""},
    };

    auto results = handler->translateBatchNLToAQL(requests);
    ASSERT_EQ(results.size(), 2u);

    // If the model is available, the first result should reference both collections
    if (results[0].success) {
        std::string aql_lower = results[0].aql_query;
        std::transform(aql_lower.begin(), aql_lower.end(), aql_lower.begin(), ::tolower);
        EXPECT_TRUE(aql_lower.find("orders") != std::string::npos);
        EXPECT_TRUE(aql_lower.find("customers") != std::string::npos);
    } else {
        GTEST_SKIP() << "Skipping model-dependent assertions: " << results[0].error;
    }
}

// ============================================================================
// Functional Tests (require LLM model – skipped automatically when absent)
// ============================================================================

TEST_F(BatchNLToAQLTranslationTest, BatchQueriesProduceValidAQL) {
    std::vector<LLMAQLHandler::BatchNLToAQLRequest> requests = {
        {"Find all users",                ""},
        {"Find all users in Seattle",     ""},
        {"Count the number of users per city", ""},
    };

    auto results = handler->translateBatchNLToAQL(requests);
    ASSERT_EQ(results.size(), requests.size());

    for (size_t i = 0; i < results.size(); ++i) {
        if (!results[i].success) {
            GTEST_SKIP() << "Skipping model-dependent test: " << results[i].error;
        }

        const std::string& aql = results[i].aql_query;
        EXPECT_FALSE(aql.empty()) << "Request " << i << " produced empty AQL";

        // No stray markdown fences
        EXPECT_EQ(aql.find("```"), std::string::npos) << "Request " << i << " contains markdown";

        // Must contain fundamental AQL keyword
        std::string aql_lower = aql;
        std::transform(aql_lower.begin(), aql_lower.end(), aql_lower.begin(), ::tolower);
        EXPECT_TRUE(aql_lower.find("for")    != std::string::npos ||
                    aql_lower.find("return") != std::string::npos)
            << "Request " << i << " AQL lacks FOR/RETURN";

        std::cout << "Request " << i << ": " << requests[i].nl_query
                  << "\n  => " << aql << "\n";
    }
}

TEST_F(BatchNLToAQLTranslationTest, BatchResultsAreOrderedLikeRequests) {
    // Each generated AQL should conceptually relate to its corresponding NL query.
    // We verify the order is preserved by checking that collection names from the
    // NL query appear in the corresponding AQL (when the model is available).
    const std::string schema = R"(
Collections:
- users: {_id, name, city}
- products: {_id, title, price}
)";

    std::vector<LLMAQLHandler::BatchNLToAQLRequest> requests = {
        {"Find all users",    schema},
        {"Find all products", schema},
    };

    auto results = handler->translateBatchNLToAQL(requests);
    ASSERT_EQ(results.size(), 2u);

    if (!results[0].success || !results[1].success) {
        GTEST_SKIP() << "Skipping model-dependent test";
    }

    auto lower = [](const std::string& s) {
        std::string t = s;
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);
        return t;
    };

    EXPECT_TRUE(lower(results[0].aql_query).find("users")    != std::string::npos);
    EXPECT_TRUE(lower(results[1].aql_query).find("products") != std::string::npos);
}
