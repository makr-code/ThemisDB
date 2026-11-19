#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/query_engine.h"
#include "query/functions/process_mining_functions.h"
#include "index/secondary_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace themis;
using namespace themis::query;

class AQLSTQueryEngineTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        themis::query::functions::registerProcessMiningFunctions(
            themis::query::functions::FunctionRegistry::instance());
    }

    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping unstable AQL ST query engine tests on Windows";
#endif
        std::filesystem::remove_all("data/themis_aql_st_test");

        RocksDBWrapper::Config cfg;
        cfg.db_path = "data/themis_aql_st_test";
        cfg.memtable_size_mb = 32;
        cfg.block_cache_size_mb = 64;

        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());
        secIdx = std::make_unique<SecondaryIndexManager>(*db);
        engine = std::make_unique<QueryEngine>(*db, *secIdx);

        // Insert documents into 'places' with geometry as GeoJSON string (clean JSON, not escaped)
        BaseEntity p1("p1");
        p1.setField("name", std::string("inside"));
        p1.setField("geom", std::string(R"({"type":"Point","coordinates":[1.0,1.0]})"));
        secIdx->put("places", p1);

        BaseEntity p2("p2");
        p2.setField("name", std::string("outside"));
        p2.setField("geom", std::string(R"({"type":"Point","coordinates":[10.0,10.0]})"));
        secIdx->put("places", p2);
    }

    void TearDown() override {
#ifdef _WIN32
        return; // SetUp() was skipped on Windows; nothing to clean up
#endif
        engine.reset();
        secIdx.reset();
        db.reset();
        std::filesystem::remove_all("data/themis_aql_st_test");
    }

    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<SecondaryIndexManager> secIdx;
    std::unique_ptr<QueryEngine> engine;
};

TEST_F(AQLSTQueryEngineTest, ProcessMiningFunctionsRunThroughLetEvaluator) {
    BaseEntity seed("seed");
    seed.setField("value", std::string("once"));
    ASSERT_TRUE(secIdx->put("seeds", seed).ok);

    for (int case_index = 0; case_index < 3; ++case_index) {
        for (int activity_index = 0; activity_index < 3; ++activity_index) {
            BaseEntity event(
                "event-" + std::to_string(case_index) + "-" + std::to_string(activity_index));
            event.setField("case_id", "case-" + std::to_string(case_index));
            event.setField("activity", std::string(1, static_cast<char>('A' + activity_index)));
            event.setField("timestamp", static_cast<int64_t>((activity_index + 1) * 1000));
            ASSERT_TRUE(secIdx->put("events", event).ok);
        }
    }

    auto collection = std::make_shared<LiteralExpr>(
        LiteralValue(std::string("events")));
    auto extract_log = std::make_shared<FunctionCallExpr>(
        "PM_EXTRACT_LOG",
        std::vector<std::shared_ptr<Expression>>{collection});
    auto discover = std::make_shared<FunctionCallExpr>(
        "PM_DISCOVER_PROCESS",
        std::vector<std::shared_ptr<Expression>>{std::make_shared<VariableExpr>("event_log")});
    auto conformance = std::make_shared<FunctionCallExpr>(
        "PM_CONFORMANCE",
        std::vector<std::shared_ptr<Expression>>{
            std::make_shared<VariableExpr>("event_log"),
            std::make_shared<VariableExpr>("model")});
    auto export_bpmn = std::make_shared<FunctionCallExpr>(
        "PM_EXPORT_BPMN",
        std::vector<std::shared_ptr<Expression>>{std::make_shared<VariableExpr>("model")});

    std::vector<LetNode> lets{
        LetNode{"event_log", extract_log},
        LetNode{"model", discover},
        LetNode{"conformance", conformance},
        LetNode{"bpmn", export_bpmn}
    };
    std::vector<ForNode> fors(1);
    fors[0].variable = "seed";
    fors[0].collection = "seeds";

    auto result_expr = std::make_shared<ObjectConstructExpr>(
        std::vector<std::pair<std::string, std::shared_ptr<Expression>>>{
            {"event_count", std::make_shared<FieldAccessExpr>(
                std::make_shared<VariableExpr>("event_log"), "total_events")},
            {"model", std::make_shared<VariableExpr>("model")},
            {"conformance", std::make_shared<VariableExpr>("conformance")},
            {"bpmn", std::make_shared<VariableExpr>("bpmn")}
        });
    auto result = engine->executeJoin(
        fors, {}, lets, std::make_shared<ReturnNode>(result_expr), nullptr, nullptr);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 1u);
    const auto& output = result->front();
    EXPECT_EQ(output["event_count"], 9);
    ASSERT_TRUE(output["model"].is_object()) << output.dump();
    EXPECT_FALSE(output["model"].contains("error")) << output.dump();
    ASSERT_TRUE(output["conformance"].is_object()) << output.dump();
    EXPECT_TRUE(output["conformance"].contains("fitness")) << output.dump();
    ASSERT_TRUE(output["bpmn"].is_string()) << output.dump();
    EXPECT_NE(output["bpmn"].get<std::string>().find("definitions"), std::string::npos);
}

TEST_F(AQLSTQueryEngineTest, ProcessMiningCollectionReadHonorsAccessPolicy) {
    BaseEntity seed("seed");
    seed.setField("value", std::string("once"));
    ASSERT_TRUE(secIdx->put("seeds", seed).ok);
    engine->setCollectionAccessChecker(
        [](const std::string& collection, const std::string&) {
            return collection != "events";
        },
        "test-user");

    auto extract_log = std::make_shared<FunctionCallExpr>(
        "PM_EXTRACT_LOG",
        std::vector<std::shared_ptr<Expression>>{
            std::make_shared<LiteralExpr>(LiteralValue(std::string("events")))});
    std::vector<LetNode> lets{LetNode{"event_log", extract_log}};
    std::vector<ForNode> fors(1);
    fors[0].variable = "seed";
    fors[0].collection = "seeds";

    auto result = engine->executeJoin(
        fors, {}, lets,
        std::make_shared<ReturnNode>(std::make_shared<VariableExpr>("event_log")),
        nullptr, nullptr);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 1u);
    ASSERT_TRUE(result->front().is_object());
    EXPECT_TRUE(result->front().contains("error")) << result->front().dump();
}

// FILTER with ST_*: ST_Within(ST_GeomFromGeoJSON(doc.geom), ST_GeomFromText("POLYGON(...))")
TEST_F(AQLSTQueryEngineTest, Filter_ST_Within_GeoJSON_Field)
{
    // Build AST manually
    auto varDoc = std::make_shared<VariableExpr>("doc");
    auto fieldGeom = std::make_shared<FieldAccessExpr>(varDoc, std::string("geom"));

    // Left arg: ST_GeomFromGeoJSON(doc.geom)
    auto callGeomFromGeoJSON = std::make_shared<FunctionCallExpr>(
        std::string("ST_GeomFromGeoJSON"),
        std::vector<std::shared_ptr<Expression>>{ fieldGeom }
    );

    // Right arg: ST_GeomFromText("POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))")
    auto litWkt = std::make_shared<LiteralExpr>(LiteralValue(std::string("POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))")));
    auto callGeomFromText = std::make_shared<FunctionCallExpr>(
        std::string("ST_GeomFromText"),
        std::vector<std::shared_ptr<Expression>>{ litWkt }
    );

    // Condition: ST_Within(left, right)
    auto cond = std::make_shared<FunctionCallExpr>(
        std::string("ST_Within"),
        std::vector<std::shared_ptr<Expression>>{ callGeomFromGeoJSON, callGeomFromText }
    );

    // Return: doc._key
    auto fieldKey = std::make_shared<FieldAccessExpr>(varDoc, std::string("_key"));

    // One FOR
    std::vector<ForNode> fors;
    ForNode f; f.variable = "doc"; f.collection = "places"; fors.push_back(f);

    // One FILTER
    auto filter = std::make_shared<FilterNode>(cond);
    std::vector<std::shared_ptr<FilterNode>> filters; filters.push_back(filter);

    // RETURN
    auto ret = std::make_shared<ReturnNode>(fieldKey);

    auto results_result = engine->executeJoin(fors, filters, {}, ret, nullptr, nullptr);
    ASSERT_TRUE(results_result.has_value()) << results_result.error().message();
    auto& results = *results_result;

    ASSERT_EQ(results.size(), 1u);
    ASSERT_TRUE(results[0].is_string());
    EXPECT_EQ(results[0].get<std::string>(), "p1");
}

// RETURN with ST_*: Return WKT of buffered point
TEST_F(AQLSTQueryEngineTest, Return_ST_AsText_Buffer_Result)
{
    auto varDoc = std::make_shared<VariableExpr>("doc");
    auto fieldGeom = std::make_shared<FieldAccessExpr>(varDoc, std::string("geom"));

    auto callGeomFromGeoJSON = std::make_shared<FunctionCallExpr>(
        std::string("ST_GeomFromGeoJSON"),
        std::vector<std::shared_ptr<Expression>>{ fieldGeom }
    );
    auto litDist = std::make_shared<LiteralExpr>(LiteralValue(1.0));
    auto callBuffer = std::make_shared<FunctionCallExpr>(
        std::string("ST_Buffer"),
        std::vector<std::shared_ptr<Expression>>{ callGeomFromGeoJSON, litDist }
    );
    auto callAsText = std::make_shared<FunctionCallExpr>(
        std::string("ST_AsText"),
        std::vector<std::shared_ptr<Expression>>{ callBuffer }
    );

    // FOR and FILTER to pick the inside point
    std::vector<ForNode> fors; ForNode f; f.variable="doc"; f.collection="places"; fors.push_back(f);

    // Filter doc.name == "inside"
    auto fieldName = std::make_shared<FieldAccessExpr>(varDoc, std::string("name"));
    auto litInside = std::make_shared<LiteralExpr>(LiteralValue(std::string("inside")));
    auto cond = std::make_shared<BinaryOpExpr>(BinaryOperator::Eq, fieldName, litInside);
    auto filter = std::make_shared<FilterNode>(cond);

    auto results_result = engine->executeJoin(fors, {filter}, {}, std::make_shared<ReturnNode>(callAsText), nullptr, nullptr);
    ASSERT_TRUE(results_result.has_value()) << results_result.error().message();
    auto& results = *results_result;
    ASSERT_EQ(results.size(), 1u);
    ASSERT_TRUE(results[0].is_string());
    std::string wkt = results[0].get<std::string>();
    // Expect a POLYGON WKT (buffer result is polygon)
    ASSERT_FALSE(wkt.empty());
    ASSERT_EQ(wkt.rfind("POLYGON(", 0), 0u);
}

// FILTER with ST_DWithin: near (0,0) within radius 2 should pick only p1
TEST_F(AQLSTQueryEngineTest, Filter_ST_DWithin_GeoJSON_Field)
{
    auto varDoc = std::make_shared<VariableExpr>("doc");
    auto fieldGeom = std::make_shared<FieldAccessExpr>(varDoc, std::string("geom"));

    auto callGeomFromGeoJSON = std::make_shared<FunctionCallExpr>(
        std::string("ST_GeomFromGeoJSON"),
        std::vector<std::shared_ptr<Expression>>{ fieldGeom }
    );

    auto litX = std::make_shared<LiteralExpr>(LiteralValue(0.0));
    auto litY = std::make_shared<LiteralExpr>(LiteralValue(0.0));
    auto callCenter = std::make_shared<FunctionCallExpr>(
        std::string("ST_Point"),
        std::vector<std::shared_ptr<Expression>>{ litX, litY }
    );
    auto litRadius = std::make_shared<LiteralExpr>(LiteralValue(2.0));

    auto cond = std::make_shared<FunctionCallExpr>(
        std::string("ST_DWithin"),
        std::vector<std::shared_ptr<Expression>>{ callGeomFromGeoJSON, callCenter, litRadius }
    );

    // Return key
    auto fieldKey = std::make_shared<FieldAccessExpr>(varDoc, std::string("_key"));

    std::vector<ForNode> fors; ForNode f; f.variable = "doc"; f.collection = "places"; fors.push_back(f);
    auto filter = std::make_shared<FilterNode>(cond);
    auto results_result = engine->executeJoin(fors, {filter}, {}, std::make_shared<ReturnNode>(fieldKey), nullptr, nullptr);
    ASSERT_TRUE(results_result.has_value()) << results_result.error().message();
    auto& results = *results_result;
    ASSERT_EQ(results.size(), 1u);
    ASSERT_TRUE(results[0].is_string());
    EXPECT_EQ(results[0].get<std::string>(), "p1");
}

// FILTER with ST_ZBetween: 3D point in Z-range
TEST_F(AQLSTQueryEngineTest, Filter_ST_ZBetween_3D_Point)
{
    // Insert 3D point
    BaseEntity p3("p3");
    p3.setField("name", std::string("elevated"));
    p3.setField("geom", std::string(R"({"type":"Point","coordinates":[1.0,1.0,50.0]})"));
    secIdx->put("places", p3);

    auto varDoc = std::make_shared<VariableExpr>("doc");
    auto fieldGeom = std::make_shared<FieldAccessExpr>(varDoc, std::string("geom"));

    auto callGeomFromGeoJSON = std::make_shared<FunctionCallExpr>(
        std::string("ST_GeomFromGeoJSON"),
        std::vector<std::shared_ptr<Expression>>{ fieldGeom }
    );

    auto litZMin = std::make_shared<LiteralExpr>(LiteralValue(40.0));
    auto litZMax = std::make_shared<LiteralExpr>(LiteralValue(60.0));
    auto cond = std::make_shared<FunctionCallExpr>(
        std::string("ST_ZBetween"),
        std::vector<std::shared_ptr<Expression>>{ callGeomFromGeoJSON, litZMin, litZMax }
    );

    auto fieldKey = std::make_shared<FieldAccessExpr>(varDoc, std::string("_key"));

    std::vector<ForNode> fors; ForNode f; f.variable = "doc"; f.collection = "places"; fors.push_back(f);
    auto filter = std::make_shared<FilterNode>(cond);
    auto results_result = engine->executeJoin(fors, {filter}, {}, std::make_shared<ReturnNode>(fieldKey), nullptr, nullptr);
    ASSERT_TRUE(results_result.has_value()) << results_result.error().message();
    auto& results = *results_result;
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].get<std::string>(), "p3");
}

// RETURN with ST_Union: combine two geometries
TEST_F(AQLSTQueryEngineTest, Return_ST_Union_Two_Points)
{
    auto varDoc = std::make_shared<VariableExpr>("doc");
    auto fieldGeom = std::make_shared<FieldAccessExpr>(varDoc, std::string("geom"));

    auto callGeomFromGeoJSON = std::make_shared<FunctionCallExpr>(
        std::string("ST_GeomFromGeoJSON"),
        std::vector<std::shared_ptr<Expression>>{ fieldGeom }
    );

    auto litX = std::make_shared<LiteralExpr>(LiteralValue(5.0));
    auto litY = std::make_shared<LiteralExpr>(LiteralValue(5.0));
    auto callPoint = std::make_shared<FunctionCallExpr>(
        std::string("ST_Point"),
        std::vector<std::shared_ptr<Expression>>{ litX, litY }
    );

    auto callUnion = std::make_shared<FunctionCallExpr>(
        std::string("ST_Union"),
        std::vector<std::shared_ptr<Expression>>{ callGeomFromGeoJSON, callPoint }
    );

    // Filter doc.name == "inside"
    auto fieldName = std::make_shared<FieldAccessExpr>(varDoc, std::string("name"));
    auto litInside = std::make_shared<LiteralExpr>(LiteralValue(std::string("inside")));
    auto cond = std::make_shared<BinaryOpExpr>(BinaryOperator::Eq, fieldName, litInside);
    auto filter = std::make_shared<FilterNode>(cond);

    std::vector<ForNode> fors; ForNode f; f.variable="doc"; f.collection="places"; fors.push_back(f);
    auto results_result = engine->executeJoin(fors, {filter}, {}, std::make_shared<ReturnNode>(callUnion), nullptr, nullptr);
    ASSERT_TRUE(results_result.has_value()) << results_result.error().message();
    auto& results = *results_result;
    ASSERT_EQ(results.size(), 1u);
    ASSERT_TRUE(results[0].is_object());
    EXPECT_EQ(results[0]["type"], "Polygon");
}

