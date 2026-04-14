/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_new_aql_functions.cpp                         ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:48:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     346                                            ║
    • Open Issues:     TODOs: 0, Stubs: 7                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_new_aql_functions.cpp
 * @brief Tests for newly registered AQL functions (fulltext, ethics, process mining)
 */

#include <gtest/gtest.h>
#include "query/functions/function_registry.h"

using namespace themis::query::functions;
using json = nlohmann::json;

class NewAQLFunctionsTest : public ::testing::Test {
protected:
    // Register all functions once per test suite
    static void SetUpTestSuite() {
        registerBuiltinFunctions();
    }
    
    FunctionContext ctx;
};

// ============================================================================
// Fulltext Function Registration Tests
// ============================================================================

TEST_F(NewAQLFunctionsTest, FulltextFunctionsRegistered) {
    auto& reg = FunctionRegistry::instance();
    
    // Check that fulltext functions are registered
    EXPECT_TRUE(reg.hasFunction("FULLTEXT"));
    EXPECT_TRUE(reg.hasFunction("PHRASE"));
    EXPECT_TRUE(reg.hasFunction("FUZZY"));
    EXPECT_TRUE(reg.hasFunction("NGRAM_MATCH"));
    EXPECT_TRUE(reg.hasFunction("TOKENS"));
    EXPECT_TRUE(reg.hasFunction("SOUNDEX"));
    EXPECT_TRUE(reg.hasFunction("METAPHONE"));
    EXPECT_TRUE(reg.hasFunction("DOUBLE_METAPHONE"));
}

TEST_F(NewAQLFunctionsTest, NgramMatchFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Test NGRAM_MATCH function
    auto result = reg.call("NGRAM_MATCH", {"hello", "hallo"}, ctx);
    EXPECT_TRUE(result.is_number());
    EXPECT_GT(result.get<double>(), 0.0);
    EXPECT_LE(result.get<double>(), 1.0);
    
    // Exact match should give high similarity
    auto exactMatch = reg.call("NGRAM_MATCH", {"test", "test"}, ctx);
    EXPECT_NEAR(exactMatch.get<double>(), 1.0, 0.01);
    
    // No match should give low similarity
    auto noMatch = reg.call("NGRAM_MATCH", {"abc", "xyz"}, ctx);
    EXPECT_LT(noMatch.get<double>(), 0.5);
}

TEST_F(NewAQLFunctionsTest, TokensFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Test TOKENS function
    auto result = reg.call("TOKENS", {"Hello world, this is a test!"}, ctx);
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 6);  // "hello", "world", "this", "is", "a", "test"
    EXPECT_EQ(result[0], "hello");
    EXPECT_EQ(result[1], "world");
}

TEST_F(NewAQLFunctionsTest, SoundexFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Test SOUNDEX function
    auto result = reg.call("SOUNDEX", {"Smith"}, ctx);
    EXPECT_TRUE(result.is_string());
    EXPECT_EQ(result.get<std::string>(), "S530");
    
    // Similar sounding names should have same soundex
    auto result2 = reg.call("SOUNDEX", {"Smythe"}, ctx);
    EXPECT_EQ(result2.get<std::string>(), "S530");
}

TEST_F(NewAQLFunctionsTest, MetaphoneFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Test METAPHONE function
    auto result = reg.call("METAPHONE", {"Smith"}, ctx);
    EXPECT_TRUE(result.is_string());
    EXPECT_FALSE(result.get<std::string>().empty());
    
    // Test with max length
    auto result2 = reg.call("METAPHONE", {"Smith", 4}, ctx);
    EXPECT_TRUE(result2.is_string());
    EXPECT_LE(result2.get<std::string>().length(), 4);
}

TEST_F(NewAQLFunctionsTest, DoubleMetaphoneFunction) {
    auto& reg = FunctionRegistry::instance();
    
    // Test DOUBLE_METAPHONE function
    auto result = reg.call("DOUBLE_METAPHONE", {"Smith"}, ctx);
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("primary"));
    EXPECT_TRUE(result.contains("secondary"));
    EXPECT_TRUE(result["primary"].is_string());
    EXPECT_TRUE(result["secondary"].is_string());
}

TEST_F(NewAQLFunctionsTest, FulltextPlaceholder) {
    auto& reg = FunctionRegistry::instance();
    
    // Test that FULLTEXT returns a placeholder result (array with note)
    auto result = reg.call("FULLTEXT", {"articles", "content", "test"}, ctx);
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].contains("_note"));
}

TEST_F(NewAQLFunctionsTest, PhrasePlaceholder) {
    auto& reg = FunctionRegistry::instance();
    
    // Test that PHRASE returns a placeholder result (array with note)
    auto result = reg.call("PHRASE", {"articles", "content", "test phrase"}, ctx);
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].contains("_note"));
}

TEST_F(NewAQLFunctionsTest, FuzzyPlaceholder) {
    auto& reg = FunctionRegistry::instance();
    
    // Test that FUZZY returns a placeholder result (array with note)
    auto result = reg.call("FUZZY", {"articles", "name", "test"}, ctx);
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].contains("_note"));
}

// ============================================================================
// Ethics Function Registration Tests
// ============================================================================

TEST_F(NewAQLFunctionsTest, EthicsFunctionsRegistered) {
    auto& reg = FunctionRegistry::instance();
    
    // Check that ethics functions are registered
    EXPECT_TRUE(reg.hasFunction("ETHICS_MAKE_DECISION"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_INITIALIZE_DEBATE"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_EVALUATE"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_EVALUATE_DIMENSION"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_GET_ARGUMENTS"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_FIND_SIMILAR_DILEMMAS"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_TRAVERSE_CHAIN"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_LOAD_PROFILE"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_LIST_SCHOOLS"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_BUILD_CONTEXT"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_STATS"));
    EXPECT_TRUE(reg.hasFunction("ETHICS_METRICS"));
}

TEST_F(NewAQLFunctionsTest, EthicsMakeDecisionStub) {
    auto& reg = FunctionRegistry::instance();
    
    // Test that ETHICS_MAKE_DECISION returns stub response
    auto result = reg.call("ETHICS_MAKE_DECISION", {
        "test dilemma",
        json::array({"kant", "utilitarianism"})
    }, ctx);
    
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("decision_id"));
    EXPECT_TRUE(result.contains("decision_text"));
    EXPECT_TRUE(result["decision_text"].get<std::string>().find("Stub") != std::string::npos);
}

TEST_F(NewAQLFunctionsTest, EthicsListSchools) {
    auto& reg = FunctionRegistry::instance();
    
    // Test ETHICS_LIST_SCHOOLS
    auto result = reg.call("ETHICS_LIST_SCHOOLS", {}, ctx);
    EXPECT_TRUE(result.is_array());
    EXPECT_GT(result.size(), 0);
    
    // Check that known schools are present
    bool hasKant = false;
    bool hasUtilitarianism = false;
    for (const auto& school : result) {
        if (school.contains("name")) {
            std::string name = school["name"].get<std::string>();
            if (name == "kant") hasKant = true;
            if (name == "utilitarianism") hasUtilitarianism = true;
        }
    }
    EXPECT_TRUE(hasKant);
    EXPECT_TRUE(hasUtilitarianism);
}

TEST_F(NewAQLFunctionsTest, EthicsGetArgumentsEmpty) {
    auto& reg = FunctionRegistry::instance();
    
    // Test that ETHICS_GET_ARGUMENTS returns empty array (stub)
    auto result = reg.call("ETHICS_GET_ARGUMENTS", {"kant"}, ctx);
    EXPECT_TRUE(result.is_array());
    // Currently returns empty array as it's a stub
}

// ============================================================================
// Process Mining Function Registration Tests
// ============================================================================

TEST_F(NewAQLFunctionsTest, ProcessMiningFunctionsRegistered) {
    auto& reg = FunctionRegistry::instance();
    
    // Check that process mining functions are registered
    EXPECT_TRUE(reg.hasFunction("PM_FIND_SIMILAR"));
    EXPECT_TRUE(reg.hasFunction("PM_COMPARE_IDEAL"));
    EXPECT_TRUE(reg.hasFunction("PM_HAS_PATTERN"));
    EXPECT_TRUE(reg.hasFunction("PM_EXTRACT_LOG"));
    EXPECT_TRUE(reg.hasFunction("PM_EXTRACT_TRACE"));
    EXPECT_TRUE(reg.hasFunction("PM_DISCOVER_PROCESS"));
    EXPECT_TRUE(reg.hasFunction("PM_VARIANTS"));
    EXPECT_TRUE(reg.hasFunction("PM_LOAD_ADMIN_MODEL"));
    EXPECT_TRUE(reg.hasFunction("PM_LIST_ADMIN_MODELS"));
    EXPECT_TRUE(reg.hasFunction("PM_CONFORMANCE"));
    EXPECT_TRUE(reg.hasFunction("PM_DEVIATIONS"));
    EXPECT_TRUE(reg.hasFunction("PM_BOTTLENECKS"));
    EXPECT_TRUE(reg.hasFunction("PM_PREDICT_END"));
    EXPECT_TRUE(reg.hasFunction("PM_EXPORT_BPMN"));
}

TEST_F(NewAQLFunctionsTest, PmFindSimilarStub) {
    auto& reg = FunctionRegistry::instance();
    
    // Test PM_FIND_SIMILAR returns stub result
    json pattern = {
        {"activities", json::array({"A", "B", "C"})}
    };
    json config = {
        {"method", "graph"},
        {"threshold", 0.7}
    };
    
    auto result = reg.call("PM_FIND_SIMILAR", {pattern, config}, ctx);
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("results"));
    EXPECT_TRUE(result["results"].is_array());
}

TEST_F(NewAQLFunctionsTest, PmHasPatternStub) {
    auto& reg = FunctionRegistry::instance();
    
    // Test PM_HAS_PATTERN returns boolean
    json pattern = {
        {"activities", json::array({"A", "B"})}
    };
    
    auto result = reg.call("PM_HAS_PATTERN", {"case-001", pattern}, ctx);
    EXPECT_TRUE(result.is_boolean());
    // Stub returns false
    EXPECT_FALSE(result.get<bool>());
}

TEST_F(NewAQLFunctionsTest, PmListAdminModelsStub) {
    auto& reg = FunctionRegistry::instance();
    
    // Test PM_LIST_ADMIN_MODELS returns array
    auto result = reg.call("PM_LIST_ADMIN_MODELS", {}, ctx);
    EXPECT_TRUE(result.is_array());
    // Stub returns empty array
}

TEST_F(NewAQLFunctionsTest, PmExportBpmnStub) {
    auto& reg = FunctionRegistry::instance();
    
    // Test PM_EXPORT_BPMN returns XML string
    json model = {
        {"activities", json::array({"A", "B"})}
    };
    
    auto result = reg.call("PM_EXPORT_BPMN", {model}, ctx);
    EXPECT_TRUE(result.is_string());
    std::string xml = result.get<std::string>();
    EXPECT_TRUE(xml.find("<bpmn>") != std::string::npos || 
                xml.find("<?xml") != std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(NewAQLFunctionsTest, AllNewFunctionCategoriesRegistered) {
    auto& reg = FunctionRegistry::instance();
    
    // Verify we have functions from each new category
    int fulltextCount = 0;
    int ethicsCount = 0;
    int processCount = 0;
    
    auto allSignatures = reg.getAllSignatures();
    
    for (const auto& sig : allSignatures) {
        const auto& name = sig.name;
        if (name.find("FULLTEXT") != std::string::npos || 
            name.find("PHRASE") != std::string::npos ||
            name.find("FUZZY") != std::string::npos ||
            name.find("NGRAM") != std::string::npos ||
            name.find("TOKENS") != std::string::npos ||
            name.find("SOUNDEX") != std::string::npos ||
            name.find("METAPHONE") != std::string::npos) {
            fulltextCount++;
        }
        
        if (name.find("ETHICS_") != std::string::npos) {
            ethicsCount++;
        }
        
        if (name.find("PM_") != std::string::npos) {
            processCount++;
        }
    }
    
    EXPECT_GE(fulltextCount, 8) << "Expected at least 8 fulltext functions";
    EXPECT_GE(ethicsCount, 12) << "Expected at least 12 ethics functions";
    EXPECT_GE(processCount, 14) << "Expected at least 14 process mining functions";
}
