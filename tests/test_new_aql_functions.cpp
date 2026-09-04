/**
 * @file test_new_aql_functions.cpp
 * @brief Tests for newly registered AQL functions (fulltext, ethics, process mining)
 */

#include <gtest/gtest.h>
#include "query/functions/function_registry.h"
#include <memory>
#include <unordered_map>

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
    EXPECT_TRUE(result["decision_text"].get<std::string>().find("ethics_ai plugin") != std::string::npos);
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
            if (name == "kant") {
              hasKant = true;
            }
            if (name == "utilitarianism") {
              hasUtilitarianism = true;
            }
        }
    }
    EXPECT_TRUE(hasKant);
    EXPECT_TRUE(hasUtilitarianism);
}

TEST_F(NewAQLFunctionsTest, EthicsGetArgumentsFiltersCollection) {
    auto& reg = FunctionRegistry::instance();

    auto collections = std::make_shared<std::unordered_map<std::string, std::vector<json>>>(
        std::unordered_map<std::string, std::vector<json>>{
            {"ethics_arguments", {
                {{"id", "arg-1"}, {"philosophy_school", "kant"}, {"argument_type", "pro"}, {"content", "Duty first"}},
                {{"id", "arg-2"}, {"philosophy_school", "kant"}, {"argument_type", "contra"}, {"content", "Consequences matter"}},
                {{"id", "arg-3"}, {"philosophy_school", "utilitarianism"}, {"argument_type", "pro"}, {"content", "Utility"}}
            }}
        });
    ctx.setCollectionScanner([collections](const std::string& collection,
                                           const std::function<bool(const json&)>& predicate) {
        std::vector<json> results;
        const auto it = collections->find(collection);
        if (it == collections->end()) {
            return results;
        }
        for (const auto& doc : it->second) {
            if (predicate(doc)) {
                results.push_back(doc);
            }
        }
        return results;
    });

    auto result = reg.call("ETHICS_GET_ARGUMENTS", {"kant", json::array({"pro"}), 10}, ctx);
    EXPECT_TRUE(result.is_array());
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].value("id", ""), "arg-1");
}

TEST_F(NewAQLFunctionsTest, EthicsFindSimilarDilemmasRanksMatches) {
    auto& reg = FunctionRegistry::instance();

    auto collections = std::make_shared<std::unordered_map<std::string, std::vector<json>>>(
        std::unordered_map<std::string, std::vector<json>>{
            {"ethics_dilemmas", {
                {{"id", "d1"}, {"description", "AI triage in hospital diagnosis workflows"}},
                {{"id", "d2"}, {"description", "Bridge toll policy debate"}},
                {{"id", "d3"}, {"description", "Medical diagnosis support with machine learning"}}
            }}
        });
    ctx.setCollectionScanner([collections](const std::string& collection,
                                           const std::function<bool(const json&)>& predicate) {
        std::vector<json> results;
        const auto it = collections->find(collection);
        if (it == collections->end()) {
            return results;
        }
        for (const auto& doc : it->second) {
            if (predicate(doc)) {
                results.push_back(doc);
            }
        }
        return results;
    });

    auto result = reg.call("ETHICS_FIND_SIMILAR_DILEMMAS",
                           {"AI diagnosis for hospitals", 0.2, 2}, ctx);
    ASSERT_TRUE(result.is_array());
    ASSERT_GE(result.size(), 1u);
    EXPECT_TRUE(result[0].contains("similarity"));
    EXPECT_EQ(result[0].value("id", ""), "ethics_dilemmas/d1");
}

TEST_F(NewAQLFunctionsTest, EthicsTraverseChainBuildsTraversal) {
    auto& reg = FunctionRegistry::instance();

    auto collections = std::make_shared<std::unordered_map<std::string, std::vector<json>>>(
        std::unordered_map<std::string, std::vector<json>>{
            {"ethics_arguments", {
                {{"id", "arg-1"}, {"content", "Start"}},
                {{"id", "arg-2"}, {"content", "Child A"}},
                {{"id", "arg-3"}, {"content", "Child B"}}
            }},
            {"ethics_arguments_graph", {
                {{"_from", "ethics_arguments/arg-1"}, {"_to", "ethics_arguments/arg-2"}, {"relation", "supports"}},
                {{"_from", "ethics_arguments/arg-2"}, {"_to", "ethics_arguments/arg-3"}, {"relation", "rebuts"}}
            }}
        });
    ctx.setCollectionScanner([collections](const std::string& collection,
                                           const std::function<bool(const json&)>& predicate) {
        std::vector<json> results;
        const auto it = collections->find(collection);
        if (it == collections->end()) {
            return results;
        }
        for (const auto& doc : it->second) {
            if (predicate(doc)) {
                results.push_back(doc);
            }
        }
        return results;
    });

    auto result = reg.call("ETHICS_TRAVERSE_CHAIN", {"arg-1", 3}, ctx);
    ASSERT_TRUE(result.is_array());
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0]["depth"], 1);
    EXPECT_EQ(result[1]["depth"], 2);
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

TEST_F(NewAQLFunctionsTest, PmFindSimilarReturnsRankedMatches) {
    auto& reg = FunctionRegistry::instance();

    ctx.setVariable("pm_event_log", json{
        {"traces", json::array({
            {{"case_id", "case-001"}, {"events", json::array({
                {{"activity", "A"}, {"timestamp_ms", 1000}},
                {{"activity", "B"}, {"timestamp_ms", 2000}},
                {{"activity", "C"}, {"timestamp_ms", 3000}}
            })}},
            {{"case_id", "case-002"}, {"events", json::array({
                {{"activity", "A"}, {"timestamp_ms", 1000}},
                {{"activity", "D"}, {"timestamp_ms", 2000}},
                {{"activity", "E"}, {"timestamp_ms", 3000}}
            })}}
        })}
    });

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
    ASSERT_EQ(result["total"], 1);
    EXPECT_EQ(result["results"][0].value("case_id", ""), "case-001");
}

TEST_F(NewAQLFunctionsTest, PmHasPatternMatchesTrace) {
    auto& reg = FunctionRegistry::instance();

    ctx.setVariable("pm_event_log", json{
        {"traces", json::array({
            {{"case_id", "case-001"}, {"events", json::array({
                {{"activity", "A"}, {"timestamp_ms", 1000}},
                {{"activity", "B"}, {"timestamp_ms", 2000}},
                {{"activity", "C"}, {"timestamp_ms", 3000}}
            })}}
        })}
    });

    json pattern = {
        {"activities", json::array({"A", "B"})}
    };
    
    auto result = reg.call("PM_HAS_PATTERN", {"case-001", pattern}, ctx);
    EXPECT_TRUE(result.is_boolean());
    EXPECT_TRUE(result.get<bool>());
}

TEST_F(NewAQLFunctionsTest, PmCompareIdealReturnsMetrics) {
    auto& reg = FunctionRegistry::instance();

    ctx.setVariable("pm_event_log", json{
        {"traces", json::array({
            {{"case_id", "case-001"}, {"events", json::array({
                {{"activity", "A"}, {"timestamp_ms", 1000}},
                {{"activity", "B"}, {"timestamp_ms", 2000}},
                {{"activity", "C"}, {"timestamp_ms", 3000}}
            })}}
        })}
    });

    json ideal = {
        {"activities", json::array({"A", "B", "C"})},
        {"edges", json::array({
            {{"from", "A"}, {"to", "B"}},
            {{"from", "B"}, {"to", "C"}}
        })}
    };

    auto result = reg.call("PM_COMPARE_IDEAL", {"case-001", ideal}, ctx);
    ASSERT_TRUE(result.is_object());
    EXPECT_GE(result.value("fitness", 0.0), 0.9);
    EXPECT_TRUE(result.contains("comparison"));
}

TEST_F(NewAQLFunctionsTest, PmExtractTraceUsesContextEventLog) {
    auto& reg = FunctionRegistry::instance();

    ctx.setVariable("pm_event_log", json{
        {"traces", json::array({
            {{"case_id", "case-001"}, {"events", json::array({
                {{"activity", "A"}, {"timestamp_ms", 1000}},
                {{"activity", "B"}, {"timestamp_ms", 2000}}
            })}}
        })}
    });

    auto result = reg.call("PM_EXTRACT_TRACE", {"case-001"}, ctx);
    ASSERT_TRUE(result.is_object());
    EXPECT_EQ(result.value("case_id", ""), "case-001");
    ASSERT_TRUE(result["events"].is_array());
    EXPECT_EQ(result["events"].size(), 2);
}

TEST_F(NewAQLFunctionsTest, PmListAdminModelsFromContext) {
    auto& reg = FunctionRegistry::instance();

    ctx.setVariable("pm_admin_models", json::array({
        {
            {"id", "bauantrag_standard"},
            {"name", "Bauantrag Standard"},
            {"domain", "public_admin"},
            {"model", {{"nodes", json::array()}, {"edges", json::array()}}}
        },
        {
            {"id", "beschaffung_vergaberecht"},
            {"name", "Beschaffung Vergaberecht"},
            {"domain", "procurement"},
            {"model", {{"nodes", json::array()}, {"edges", json::array()}}}
        }
    }));

    auto result = reg.call("PM_LIST_ADMIN_MODELS", {}, ctx);
    EXPECT_TRUE(result.is_array());
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].value("id", ""), "bauantrag_standard");
    EXPECT_EQ(result[1].value("id", ""), "beschaffung_vergaberecht");
}

TEST_F(NewAQLFunctionsTest, PmLoadAdminModelFromContext) {
    auto& reg = FunctionRegistry::instance();

    ctx.setVariable("pm_admin_models", json::array({
        {
            {"id", "bauantrag_standard"},
            {"name", "Bauantrag Standard"},
            {"domain", "public_admin"},
            {"model", {{"nodes", json::array({"start", "end"})}, {"edges", json::array()}}}
        }
    }));

    auto found = reg.call("PM_LOAD_ADMIN_MODEL", {"bauantrag_standard"}, ctx);
    EXPECT_TRUE(found.is_object());
    EXPECT_EQ(found.value("id", ""), "bauantrag_standard");
    EXPECT_TRUE(found.contains("model"));

    auto missing = reg.call("PM_LOAD_ADMIN_MODEL", {"does_not_exist"}, ctx);
    EXPECT_TRUE(missing.is_object());
    EXPECT_TRUE(missing.contains("error"));
}

TEST_F(NewAQLFunctionsTest, PmPredictEndUsesCaseIdMap) {
    auto& reg = FunctionRegistry::instance();

    ctx.setVariable("pm_predicted_end_by_case", json{
        {"V-2024-0001", "2026-05-20T12:00:00Z"},
        {"V-2024-0002", "2026-05-21T12:00:00Z"}
    });

    auto predicted = reg.call("PM_PREDICT_END", {"V-2024-0001"}, ctx);
    EXPECT_TRUE(predicted.is_object());
    EXPECT_EQ(predicted["predicted_end"], "2026-05-20T12:00:00Z");

    auto unknown = reg.call("PM_PREDICT_END", {"V-unknown"}, ctx);
    EXPECT_TRUE(unknown.is_object());
    EXPECT_TRUE(unknown["predicted_end"].is_null());
}

TEST_F(NewAQLFunctionsTest, PmExportBpmnStub) {
    auto& reg = FunctionRegistry::instance();
    
    // Test PM_EXPORT_BPMN returns XML string even without an engine
    json model = {
        {"nodes", json::array()},
        {"edges", json::array()}
    };
    
    auto result = reg.call("PM_EXPORT_BPMN", {model}, ctx);
    EXPECT_TRUE(result.is_string());
    std::string xml = result.get<std::string>();
    // Without an injected engine the function returns a minimal BPMN envelope
    EXPECT_TRUE(xml.find("definitions") != std::string::npos ||
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

// ============================================================================
// PM function dispatch tests with real ProcessMining engine
// ============================================================================

#if !defined(_WIN32) && !defined(_WIN64)

#include "analytics/process_mining.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>

class PmFunctionEngineTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        registerBuiltinFunctions();
    }

    void SetUp() override {
        db_path_ = std::filesystem::temp_directory_path() /
                   ("pm_fn_test_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_.string();
        db_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        pm_ = std::make_unique<themis::ProcessMining>(*db_);
        ctx_.setProcessMining(pm_.get());
    }

    void TearDown() override {
        pm_.reset();
        db_.reset();
        std::error_code ec;
        std::filesystem::remove_all(db_path_, ec);
    }

    // Build a minimal event log JSON with 3 traces (A→B→C pattern)
    static json makeSimpleEventLog() {
        json log;
        log["traces"] = json::array();
        for (int t = 0; t < 3; ++t) {
            json trace;
            trace["case_id"] = "case-" + std::to_string(t);
            trace["events"] = json::array({
                {{"activity", "A"}, {"timestamp_ms", 1000}},
                {{"activity", "B"}, {"timestamp_ms", 2000}},
                {{"activity", "C"}, {"timestamp_ms", 3000}}
            });
            log["traces"].push_back(trace);
        }
        return log;
    }

    FunctionContext ctx_;
    std::filesystem::path db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::ProcessMining> pm_;
};

TEST_F(PmFunctionEngineTest, DiscoverProcessReturnsModel) {
    auto& reg = FunctionRegistry::instance();
    const json log = makeSimpleEventLog();
    const json config = {{"algorithm", "alpha"}};

    const json result = reg.call("PM_DISCOVER_PROCESS", {log, config}, ctx_);

    ASSERT_TRUE(result.is_object());
    EXPECT_FALSE(result.contains("error")) << result.dump();
    // Must not carry the old stub marker
    EXPECT_FALSE(result.value("_stub", false));
    EXPECT_TRUE(result.contains("nodes"));
    EXPECT_TRUE(result.contains("edges"));
    EXPECT_GE(result["nodes"].size(), 1u);
}

TEST_F(PmFunctionEngineTest, DiscoverProcessHeuristicAlgorithm) {
    auto& reg = FunctionRegistry::instance();
    const json log = makeSimpleEventLog();
    const json config = {{"algorithm", "heuristic"}, {"dependency_threshold", 0.5}};

    const json result = reg.call("PM_DISCOVER_PROCESS", {log, config}, ctx_);

    ASSERT_TRUE(result.is_object());
    EXPECT_FALSE(result.contains("error")) << result.dump();
    EXPECT_TRUE(result.contains("activities_count"));
    EXPECT_GE(result.value("activities_count", 0u), 1u);
}

TEST_F(PmFunctionEngineTest, VariantsReturnsArray) {
    auto& reg = FunctionRegistry::instance();
    const json log = makeSimpleEventLog();

    const json result = reg.call("PM_VARIANTS", {log, 10}, ctx_);

    ASSERT_TRUE(result.is_array());
    // 3 traces with identical sequence → 1 variant
    ASSERT_GE(result.size(), 1u);
    EXPECT_TRUE(result[0].contains("variant_id"));
    EXPECT_TRUE(result[0].contains("activities"));
    EXPECT_TRUE(result[0].contains("frequency"));
}

TEST_F(PmFunctionEngineTest, ConformanceWithDiscoveredModel) {
    auto& reg = FunctionRegistry::instance();
    const json log = makeSimpleEventLog();

    // First discover the model
    const json model = reg.call("PM_DISCOVER_PROCESS", {log}, ctx_);
    ASSERT_FALSE(model.contains("error")) << model.dump();

    // Now check conformance of the same log against its own model
    const json conf = reg.call("PM_CONFORMANCE", {log, model}, ctx_);
    ASSERT_TRUE(conf.is_object());
    EXPECT_TRUE(conf.contains("fitness"));
    EXPECT_TRUE(conf.contains("precision"));
    EXPECT_GE(conf.value("fitness", -1.0), 0.0);
    EXPECT_LE(conf.value("fitness", 2.0), 1.0);
}

TEST_F(PmFunctionEngineTest, BottlenecksReturnsList) {
    auto& reg = FunctionRegistry::instance();
    const json log = makeSimpleEventLog();

    const json result = reg.call("PM_BOTTLENECKS", {log, 0.5}, ctx_);
    // Result is an array (possibly empty for a simple log)
    ASSERT_TRUE(result.is_array());
}

TEST_F(PmFunctionEngineTest, ExportBpmnWithRealModel) {
    auto& reg = FunctionRegistry::instance();
    const json log = makeSimpleEventLog();
    const json model = reg.call("PM_DISCOVER_PROCESS", {log}, ctx_);
    ASSERT_FALSE(model.contains("error")) << model.dump();

    const json bpmn = reg.call("PM_EXPORT_BPMN", {model}, ctx_);
    ASSERT_TRUE(bpmn.is_string());
    const std::string xml = bpmn.get<std::string>();
    // Must be non-trivial BPMN XML
    EXPECT_FALSE(xml.empty());
    EXPECT_TRUE(xml.find("definitions") != std::string::npos ||
                xml.find("process") != std::string::npos ||
                xml.find("<?xml") != std::string::npos);
}

TEST_F(PmFunctionEngineTest, DiscoverProcessWithoutEngineReturnsStub) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext no_engine_ctx;  // No ProcessMining injected
    const json log = makeSimpleEventLog();

    const json result = reg.call("PM_DISCOVER_PROCESS", {log}, no_engine_ctx);
    ASSERT_TRUE(result.is_object());
    // Stub result must carry _stub=true and correct shape
    EXPECT_TRUE(result.value("_stub", false));
    EXPECT_EQ(result.value("activities_count", -1), 0);
}

TEST_F(PmFunctionEngineTest, VariantsWithoutEngineReturnsEmptyArray) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext no_engine_ctx;
    const json log = makeSimpleEventLog();

    const json result = reg.call("PM_VARIANTS", {log}, no_engine_ctx);
    ASSERT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 0u);
}

#endif // !_WIN32

// ====================================================
// REL-09: NGRAM_MATCH totalSz size-safety (issue #5177)
// ====================================================

TEST(NewAQLFunctionsTest, NgramMatchIdenticalStringsReturnOne) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    // Identical strings: all ngrams match → similarity must be exactly 1.0.
    auto result = reg.call("NGRAM_MATCH", {json("hello"), json("hello")}, ctx);
    ASSERT_TRUE(result.is_number());
    EXPECT_DOUBLE_EQ(result.get<double>(), 1.0);
}

TEST(NewAQLFunctionsTest, NgramMatchCompletelyDifferentStringsReturnZero) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    // No common bigrams between "aaa" and "zzz".
    auto result = reg.call("NGRAM_MATCH", {json("aaa"), json("zzz")}, ctx);
    ASSERT_TRUE(result.is_number());
    EXPECT_DOUBLE_EQ(result.get<double>(), 0.0);
}

TEST(NewAQLFunctionsTest, NgramMatchReturnValueInUnitRange) {
    auto& reg = FunctionRegistry::instance();
    FunctionContext ctx;
    auto result = reg.call("NGRAM_MATCH", {json("machine"), json("matching")}, ctx);
    ASSERT_TRUE(result.is_number());
    double v = result.get<double>();
    EXPECT_GE(v, 0.0);
    EXPECT_LE(v, 1.0);
}
