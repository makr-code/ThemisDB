/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_postgres_importer_v2.cpp                      ║
  Version:         2.1.0                                              ║
╠═════════════════════════════════════════════════════════════════════╣
  Tests for the 12 new v2.1+ importer modules:
    schema_inference, column_importance, crdt_importer,
    postgres_cdc, data_quality, audit_trail, adaptive_import,
    polyglot_mapper, temporal_support, blockchain_integrity,
    federated_learning, graphql_federation
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <thread>
#include <chrono>

// New v2.1+ headers
#include "importers/schema_inference.h"
#include "importers/column_importance.h"
#include "importers/crdt_importer.h"
#include "importers/postgres_cdc.h"
#include "importers/data_quality.h"
#include "importers/audit_trail.h"
#include "importers/adaptive_import.h"
#include "importers/polyglot_mapper.h"
#include "importers/temporal_support.h"
#include "importers/blockchain_integrity.h"
#include "importers/federated_learning.h"
#include "importers/graphql_federation.h"

using json = nlohmann::json;
using namespace themis::importers;

// ===========================================================================
// Helper: build a minimal InferenceTableSchema
// ===========================================================================
static InferenceTableSchema makeSchema(
    const std::string& name,
    const std::vector<std::string>& columns,
    const std::vector<std::string>& pks = {},
    const std::vector<std::pair<std::string,std::string>>& fks = {})
{
    InferenceTableSchema s;
    s.name        = name;
    s.schema_ns   = "public";
    s.columns     = columns;
    s.primary_keys = pks;
    s.foreign_keys = fks;
    for (const auto& col : columns) s.column_types[col] = "text";
    return s;
}

// ===========================================================================
// 1. Schema Inference Engine
// ===========================================================================

TEST(SchemaInferenceEngine, DefaultConstruct) {
    SchemaInferenceEngine engine;
    EXPECT_NO_THROW(engine.inferImplicitRelationships({}, {}));
}

TEST(SchemaInferenceEngine, InferImplicitRelationshipsByNameStem) {
    auto users  = makeSchema("users",  {"id", "name"},        {"id"});
    auto orders = makeSchema("orders", {"id", "user_id", "amount"}, {"id"});

    SchemaInferenceEngine engine;
    auto inferred = engine.inferImplicitRelationships({users, orders}, {});

    // orders table should have at least one likely relationship detected
    bool found = false;
    for (const auto& inf : inferred) {
        if (inf.table_name == "orders" && !inf.likely_relationships.empty()) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST(SchemaInferenceEngine, InferReturnsSchemasForEveryTable) {
    auto t1 = makeSchema("t1", {"id"}, {"id"});
    auto t2 = makeSchema("t2", {"id"}, {"id"});
    SchemaInferenceEngine engine;
    auto result = engine.inferImplicitRelationships({t1, t2}, {});
    EXPECT_EQ(result.size(), 2u);
}

TEST(SchemaInferenceEngine, SemanticTypeDetection_Email) {
    auto s = makeSchema("users", {"email"});
    SampleData sd;
    sd.table_name  = "users";
    sd.column_name = "email";
    sd.values      = {"alice@example.com", "bob@test.org", "carol@domain.net"};

    SchemaInferenceEngine engine;
    auto types = engine.detectSemanticTypes({s}, {sd});

    ASSERT_TRUE(types.count("users.email"));
    EXPECT_EQ(types.at("users.email"), SchemaInferenceEngine::SemanticType::EMAIL);
}

TEST(SchemaInferenceEngine, SemanticTypeDetection_UUID) {
    auto s = makeSchema("entities", {"uuid_col"});
    SampleData sd;
    sd.table_name  = "entities";
    sd.column_name = "uuid_col";
    sd.values = {
        "550e8400-e29b-41d4-a716-446655440000",
        "6ba7b810-9dad-11d1-80b4-00c04fd430c8",
        "6ba7b811-9dad-11d1-80b4-00c04fd430c8"
    };

    SchemaInferenceEngine engine;
    auto types = engine.detectSemanticTypes({s}, {sd});
    ASSERT_TRUE(types.count("entities.uuid_col"));
    EXPECT_EQ(types.at("entities.uuid_col"), SchemaInferenceEngine::SemanticType::UUID);
}

TEST(SchemaInferenceEngine, SemanticTypeToString) {
    EXPECT_EQ(SchemaInferenceEngine::semanticTypeToString(
                SchemaInferenceEngine::SemanticType::EMAIL),
              "EMAIL");
    EXPECT_EQ(SchemaInferenceEngine::semanticTypeToString(
                SchemaInferenceEngine::SemanticType::UNKNOWN),
              "UNKNOWN");
}

TEST(SchemaInferenceEngine, CardinalityEstimateFromStats) {
    auto orders = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});
    ColumnStatistics cs;
    cs.column_name   = "user_id";
    cs.table_name    = "orders";
    cs.total_rows    = 1000;
    cs.distinct_count = 100;
    cs.null_count    = 10;

    SchemaInferenceEngine engine;
    auto ests = engine.estimateCardinalities({orders},
        {{"orders.user_id", cs}});

    ASSERT_FALSE(ests.empty());
    EXPECT_GT(ests[0].one_to_many_ratio, 0.0);
    EXPECT_GE(ests[0].selectivity, 0.0);
    EXPECT_LE(ests[0].selectivity, 1.0);
    EXPECT_EQ(ests[0].confidence_interval.size(), 2u);
}

TEST(SchemaInferenceEngine, CardinalityEstimateNoStats) {
    auto orders = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});
    SchemaInferenceEngine engine;
    auto ests = engine.estimateCardinalities({orders}, {});
    EXPECT_FALSE(ests.empty());
}

// ===========================================================================
// 2. Column Importance Analyzer
// ===========================================================================

TEST(ColumnImportanceAnalyzer, AnalyzeEmptySamples) {
    ColumnImportanceAnalyzer analyzer;
    auto result = analyzer.analyzeImportance({}, {});
    EXPECT_TRUE(result.empty());
}

TEST(ColumnImportanceAnalyzer, EntropyIsNonNegative) {
    auto schema = makeSchema("logs", {"level", "message"});
    SampleData sd;
    sd.table_name  = "logs";
    sd.column_name = "level";
    sd.values      = {"INFO", "WARN", "ERROR", "INFO", "INFO"};

    ColumnImportanceAnalyzer analyzer;
    auto result = analyzer.analyzeImportance({schema}, {sd});

    ASSERT_FALSE(result.empty());
    for (const auto& ci : result) {
        EXPECT_GE(ci.entropy, 0.0);
        EXPECT_GE(ci.gini_impurity, 0.0);
        EXPECT_GE(ci.information_gain, 0.0);
    }
}

TEST(ColumnImportanceAnalyzer, ToJsonReturnsTable) {
    ColumnImportanceAnalyzer::ColumnImportance ci;
    ci.table_name  = "t";
    ci.column_name = "c";
    ci.entropy     = 1.5;
    auto j = ci.toJson();
    EXPECT_EQ(j["table"].get<std::string>(), "t");
    EXPECT_EQ(j["column"].get<std::string>(), "c");
    EXPECT_DOUBLE_EQ(j["entropy"].get<double>(), 1.5);
}

TEST(ColumnImportanceAnalyzer, FindRedundantColumnsEmpty) {
    ColumnImportanceAnalyzer analyzer;
    auto red = analyzer.findRedundantColumns({});
    EXPECT_TRUE(red.empty());
}

TEST(ColumnImportanceAnalyzer, FindRedundantColumnsDetected) {
    ColumnImportanceAnalyzer::ColumnImportance a, b;
    a.table_name = b.table_name = "t";
    a.column_name = "c1"; b.column_name = "c2";
    a.entropy = b.entropy = 2.0; // identical entropy

    ColumnImportanceAnalyzer analyzer;
    auto red = analyzer.findRedundantColumns({a, b}, 0.95);
    EXPECT_FALSE(red.empty());
}

// ===========================================================================
// 3. CRDT Importer
// ===========================================================================

TEST(CRDTTableState, ImportAndLookup) {
    crdt::CRDTTableState state;
    json r1 = {{"id", "a"}, {"value", 1}};
    json r2 = {{"id", "b"}, {"value", 2}};

    size_t written = state.importWithCRDT("orders", {r1, r2}, "replica_1");
    EXPECT_EQ(written, 2u);

    auto* rec = state.lookup("orders", "a");
    ASSERT_NE(rec, nullptr);
    EXPECT_EQ(rec->replica_id, "replica_1");
}

TEST(CRDTTableState, MergeLastWriteWins) {
    crdt::CRDTTableState::CRDTRecord left, right;
    left.id            = "x"; left.wall_clock_ns  = 100; left.lamport_clock = 1;
    left.replica_id    = "r1";
    right.id           = "x"; right.wall_clock_ns = 200; right.lamport_clock = 1;
    right.replica_id   = "r2";

    auto winner = crdt::CRDTTableState::CRDTRecord::merge(left, right);
    EXPECT_EQ(winner.wall_clock_ns, 200u);
    EXPECT_EQ(winner.replica_id, "r2");
}

TEST(CRDTTableState, MergeTiebreakByReplicaId) {
    crdt::CRDTTableState::CRDTRecord left, right;
    left.wall_clock_ns = right.wall_clock_ns = 100;
    left.lamport_clock = right.lamport_clock = 5;
    left.replica_id  = "z_replica";
    right.replica_id = "a_replica";

    auto winner = crdt::CRDTTableState::CRDTRecord::merge(left, right);
    EXPECT_EQ(winner.replica_id, "z_replica"); // lexicographically higher wins
}

TEST(CRDTTableState, SkipRecordsWithoutId) {
    crdt::CRDTTableState state;
    json r = {{"value", 42}}; // no "id"
    size_t written = state.importWithCRDT("t", {r}, "r1");
    EXPECT_EQ(written, 0u);
}

TEST(CRDTTableState, SerialiseDeserialise) {
    crdt::CRDTTableState::CRDTRecord r;
    r.id = "abc"; r.lamport_clock = 7; r.replica_id = "rx";
    r.wall_clock_ns = 999; r.value = json{{"x", 1}};

    auto j   = r.toJson();
    auto r2  = crdt::CRDTTableState::CRDTRecord::fromJson(j);
    EXPECT_EQ(r2.id, r.id);
    EXPECT_EQ(r2.lamport_clock, r.lamport_clock);
    EXPECT_EQ(r2.replica_id, r.replica_id);
}

TEST(CRDTTableState, TickClock) {
    crdt::CRDTTableState state;
    uint64_t t1 = state.tickClock();
    uint64_t t2 = state.tickClock();
    EXPECT_GT(t2, t1);
}

// ===========================================================================
// 4. PostgreSQL CDC
// ===========================================================================

TEST(PostgreSQLCDC, CreateDecoder) {
    auto dec = PostgreSQLCDC::createDecoder("host=localhost dbname=test");
    ASSERT_NE(dec, nullptr);
}

TEST(PostgreSQLCDC, CreatePublicationReturnTrue) {
    auto dec = PostgreSQLCDC::createDecoder("fake_conn");
    EXPECT_TRUE(dec->createPublication("pub1", {"orders", "users"}));
}

TEST(PostgreSQLCDC, CreateReplicationSlotReturnTrue) {
    auto dec = PostgreSQLCDC::createDecoder("fake_conn");
    EXPECT_TRUE(dec->createReplicationSlot("slot1", true));
}

TEST(PostgreSQLCDC, ConfirmLSNNoThrow) {
    auto dec = PostgreSQLCDC::createDecoder("fake_conn");
    EXPECT_NO_THROW(dec->confirmLSN(12345));
}

TEST(PostgreSQLCDC, CancelStopsSubscription) {
    auto dec = PostgreSQLCDC::createDecoder("fake_conn");

    int event_count = 0;
    // Run subscribeToChanges in a thread and cancel immediately
    std::atomic<bool> started{false};
    std::thread t([&]() {
        dec->subscribeToChanges("slot1", [&](const PostgreSQLCDC::ChangeEvent& e) {
            ++event_count;
            started = true;
        });
    });

    // Wait for sentinel event, then cancel
    while (!started) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    dec->cancel();
    t.join();

    EXPECT_GE(event_count, 1); // at least the sentinel
}

// ===========================================================================
// 5. Data Quality Framework
// ===========================================================================

TEST(DataQualityFramework, AssessTableEmpty) {
    DataQualityFramework::QualityAssessor qa;
    auto m = qa.assessTable("t", {});
    EXPECT_EQ(m.overall_quality_score, 0.0);
}

TEST(DataQualityFramework, AssessTablePerfectData) {
    DataQualityFramework::QualityAssessor qa;
    std::vector<json> rows = {
        {{"id", "1"}, {"name", "Alice"}},
        {{"id", "2"}, {"name", "Bob"}},
        {{"id", "3"}, {"name", "Carol"}}
    };
    auto m = qa.assessTable("users", rows);
    EXPECT_GT(m.overall_quality_score, 50.0);
    EXPECT_LE(m.completeness, 1.0);
    EXPECT_GE(m.completeness, 0.0);
}

TEST(DataQualityFramework, ToJsonFields) {
    DataQualityFramework::DataQualityMetrics m;
    m.completeness = 0.9; m.accuracy = 0.8;
    m.overall_quality_score = 85.0;
    auto j = m.toJson();
    EXPECT_DOUBLE_EQ(j["completeness"].get<double>(), 0.9);
    EXPECT_DOUBLE_EQ(j["overall_quality_score"].get<double>(), 85.0);
}

TEST(DataQualityFramework, GenerateQualityReportHasTimestamp) {
    DataQualityFramework::QualityAssessor qa;
    auto schema = makeSchema("t", {"id"}, {"id"});
    auto report = qa.generateQualityReport({schema});
    EXPECT_FALSE(report.generation_timestamp.empty());
}

TEST(DataQualityFramework, LowQualityTableTriggersIssue) {
    DataQualityFramework::QualityAssessor qa;
    // All null values → very low completeness
    std::vector<json> rows(5, json{{"id", nullptr}, {"val", nullptr}});
    auto schema = makeSchema("t", {"id", "val"});

    // Manually assess and check score is low
    auto m = qa.assessTable("t", rows);
    EXPECT_LT(m.completeness, 0.5);
}

// ===========================================================================
// 6. Audit Trail
// ===========================================================================

TEST(AuditTrail, RecordAndSize) {
    AuditedImporter::ImmutableAuditLog log;
    AuditedImporter::AuditEvent ev;
    ev.type               = AuditedImporter::EventType::IMPORT_STARTED;
    ev.timestamp          = "2026-03-11T00:00:00Z";
    ev.user_principal     = "user@example.com";
    ev.importer_instance_id = "inst-001";
    ev.correlation_id     = "corr-abc";
    ev.details            = json{{"rows", 100}};

    log.recordEvent(ev);
    EXPECT_EQ(log.size(), 1u);
}

TEST(AuditTrail, IntegrityVerifiesCleanLog) {
    AuditedImporter::ImmutableAuditLog log;
    for (int i = 0; i < 5; ++i) {
        AuditedImporter::AuditEvent ev;
        ev.type               = AuditedImporter::EventType::RECORD_IMPORTED;
        ev.timestamp          = "2026-03-11T00:00:0" + std::to_string(i) + "Z";
        ev.user_principal     = "svc";
        ev.importer_instance_id = "inst";
        ev.correlation_id     = "c";
        ev.details            = json{{"i", i}};
        log.recordEvent(ev);
    }
    EXPECT_TRUE(log.verifyIntegrity());
}

TEST(AuditTrail, ExportForSIEM_Raw) {
    AuditedImporter::ImmutableAuditLog log;
    AuditedImporter::AuditEvent ev;
    ev.type = AuditedImporter::EventType::IMPORT_COMPLETED;
    ev.timestamp = "2026-03-11T00:00:00Z";
    ev.details = json{{"tables", 3}};
    log.recordEvent(ev);

    auto exported = log.exportForSIEM("raw");
    ASSERT_TRUE(exported.is_array());
    EXPECT_EQ(exported.size(), 1u);
    EXPECT_EQ(exported[0]["event_type"].get<std::string>(), "IMPORT_COMPLETED");
}

TEST(AuditTrail, ExportForSIEM_Splunk) {
    AuditedImporter::ImmutableAuditLog log;
    AuditedImporter::AuditEvent ev;
    ev.type = AuditedImporter::EventType::IMPORT_STARTED;
    ev.timestamp = "2026-03-11T00:00:00Z";
    ev.details = json{};
    log.recordEvent(ev);

    auto exported = log.exportForSIEM("splunk");
    ASSERT_TRUE(exported.is_array());
    ASSERT_EQ(exported.size(), 1u);
    EXPECT_TRUE(exported[0].contains("sourcetype"));
}

TEST(AuditTrail, EventTypeToString) {
    EXPECT_EQ(AuditedImporter::eventTypeToString(
                AuditedImporter::EventType::IMPORT_STARTED),
              "IMPORT_STARTED");
    EXPECT_EQ(AuditedImporter::eventTypeToString(
                AuditedImporter::EventType::ERROR_OCCURRED),
              "ERROR_OCCURRED");
}

// ===========================================================================
// 7. Adaptive Import Optimizer
// ===========================================================================

TEST(AdaptiveImportOptimizer, OptimizePlanNoFK) {
    auto t1 = makeSchema("a", {"id"}, {"id"});
    auto t2 = makeSchema("b", {"id"}, {"id"});
    AdaptiveImportOptimizer opt;
    auto plan = opt.optimizeImportPlan({t1, t2});
    EXPECT_EQ(plan.import_order.size(), 2u);
    EXPECT_TRUE(plan.parallel_candidates.at("a"));
    EXPECT_TRUE(plan.parallel_candidates.at("b"));
}

TEST(AdaptiveImportOptimizer, TopologicalOrderRespectsFKs) {
    auto users  = makeSchema("users",  {"id"}, {"id"});
    auto orders = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});

    AdaptiveImportOptimizer opt;
    auto plan = opt.optimizeImportPlan({orders, users}); // reverse input order

    // "users" must appear before "orders"
    auto& ord = plan.import_order;
    auto u_pos = std::find(ord.begin(), ord.end(), "users");
    auto o_pos = std::find(ord.begin(), ord.end(), "orders");
    ASSERT_NE(u_pos, ord.end());
    ASSERT_NE(o_pos, ord.end());
    EXPECT_LT(u_pos, o_pos);
}

TEST(AdaptiveImportOptimizer, AdaptBatchSizeReducesUnderLoad) {
    AdaptiveImportOptimizer opt;
    double before = opt.currentBatchMultiplier();

    AdaptiveImportOptimizer::RuntimeMetrics metrics;
    metrics.cpu_utilization    = 95.0;
    metrics.memory_utilization = 85.0;
    opt.adaptBatchSize(metrics);

    EXPECT_LT(opt.currentBatchMultiplier(), before);
}

TEST(AdaptiveImportOptimizer, AdaptBatchSizeIncreasesUnderLowLoad) {
    AdaptiveImportOptimizer opt;
    double before = opt.currentBatchMultiplier();

    AdaptiveImportOptimizer::RuntimeMetrics metrics;
    metrics.cpu_utilization    = 20.0;
    metrics.memory_utilization = 20.0;
    opt.adaptBatchSize(metrics);

    EXPECT_GT(opt.currentBatchMultiplier(), before);
}

TEST(AdaptiveImportOptimizer, PerformancePredictorNoThrow) {
    auto schema = makeSchema("t", {"id"}, {"id"});
    AdaptiveImportOptimizer opt;
    auto plan = opt.optimizeImportPlan({schema});

    AdaptiveImportOptimizer::PerformancePredictor pp;
    EXPECT_NO_THROW(pp.predictPerformance(plan, {schema}));
}

// ===========================================================================
// 8. Polyglot Persistence Mapper
// ===========================================================================

TEST(PolyglotPersistenceMapper, RecommendRelationalByDefault) {
    auto schema = makeSchema("products",
        {"id", "name", "price"}, {"id"});

    PolyglotPersistenceMapper mapper;
    auto recommendations = mapper.recommendDataModels({schema});
    ASSERT_EQ(recommendations.size(), 1u);
    EXPECT_EQ(recommendations[0].source_table, "products");
}

TEST(PolyglotPersistenceMapper, RecommendTimeSeries) {
    auto schema = makeSchema("events",
        {"id", "timestamp", "value"}, {"id"});

    PolyglotPersistenceMapper mapper;
    auto recs = mapper.recommendDataModels({schema});
    ASSERT_EQ(recs.size(), 1u);
    EXPECT_EQ(recs[0].recommended_model, PolyglotPersistenceMapper::DataModel::TIMESERIES);
}

TEST(PolyglotPersistenceMapper, RecommendKeyValue) {
    auto schema = makeSchema("settings", {"key", "value"}, {"key"});

    PolyglotPersistenceMapper mapper;
    auto recs = mapper.recommendDataModels({schema});
    ASSERT_EQ(recs.size(), 1u);
    EXPECT_EQ(recs[0].recommended_model, PolyglotPersistenceMapper::DataModel::KEYVALUE);
}

TEST(PolyglotPersistenceMapper, DataModelToString) {
    EXPECT_EQ(PolyglotPersistenceMapper::dataModelToString(
                PolyglotPersistenceMapper::DataModel::GRAPH),
              "GRAPH");
    EXPECT_EQ(PolyglotPersistenceMapper::dataModelToString(
                PolyglotPersistenceMapper::DataModel::TIMESERIES),
              "TIMESERIES");
}

TEST(PolyglotPersistenceMapper, TableToDocumentAddsSchemaField) {
    auto schema = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});
    PolyglotPersistenceMapper::ModelTransformer t;
    json row = {{"id", "1"}, {"user_id", "42"}};
    auto doc = t.tableToDocument(row, schema);
    EXPECT_TRUE(doc.contains("_schema"));
    EXPECT_TRUE(doc.contains("users")); // nested FK
}

TEST(PolyglotPersistenceMapper, TableToGraphProducesNodesAndEdges) {
    auto schema = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});
    PolyglotPersistenceMapper::ModelTransformer t;
    std::vector<json> rows = {
        {{"id", "1"}, {"user_id", "10"}},
        {{"id", "2"}, {"user_id", "11"}}
    };
    auto [nodes, edges] = t.tableToGraph(rows, schema);
    EXPECT_EQ(nodes.size(), 2u);
    EXPECT_EQ(edges.size(), 2u);
}

// ===========================================================================
// 9. Temporal Database Support
// ===========================================================================

TEST(TemporalDatabaseSupport, DetectValidTime) {
    auto schema = makeSchema("contracts",
        {"id", "valid_from", "valid_to"}, {"id"});
    TemporalDatabaseSupport tds;
    auto detected = tds.detectTemporalDimensions({schema});
    ASSERT_EQ(detected.size(), 1u);
    EXPECT_EQ(detected[0].temporal_model,
              TemporalDatabaseSupport::TemporalModel::VALID_TIME);
    EXPECT_EQ(detected[0].valid_from_column, "valid_from");
    EXPECT_EQ(detected[0].valid_to_column,   "valid_to");
}

TEST(TemporalDatabaseSupport, DetectTransactionTime) {
    auto schema = makeSchema("log",
        {"id", "created_at", "updated_at"}, {"id"});
    TemporalDatabaseSupport tds;
    auto detected = tds.detectTemporalDimensions({schema});
    ASSERT_EQ(detected.size(), 1u);
    EXPECT_EQ(detected[0].temporal_model,
              TemporalDatabaseSupport::TemporalModel::TRANSACTION_TIME);
}

TEST(TemporalDatabaseSupport, DetectBiTemporal) {
    auto schema = makeSchema("facts",
        {"id", "valid_from", "valid_to", "created_at"}, {"id"});
    TemporalDatabaseSupport tds;
    auto detected = tds.detectTemporalDimensions({schema});
    ASSERT_EQ(detected.size(), 1u);
    EXPECT_EQ(detected[0].temporal_model,
              TemporalDatabaseSupport::TemporalModel::BI_TEMPORAL);
}

TEST(TemporalDatabaseSupport, NoBiTemporalColumns) {
    auto schema = makeSchema("products", {"id", "name"}, {"id"});
    TemporalDatabaseSupport tds;
    auto detected = tds.detectTemporalDimensions({schema});
    EXPECT_TRUE(detected.empty());
}

TEST(TemporalDatabaseSupport, PointInTimeQueryContainsTimestamp) {
    TemporalDatabaseSupport::TemporalSchema ts;
    ts.table_name        = "contracts";
    ts.temporal_model    = TemporalDatabaseSupport::TemporalModel::VALID_TIME;
    ts.valid_from_column = "valid_from";
    ts.valid_to_column   = "valid_to";

    TemporalDatabaseSupport::TemporalQueryBuilder builder;
    auto sql = builder.buildPointInTimeQuery(ts, "2023-06-01");
    EXPECT_NE(sql.find("2023-06-01"), std::string::npos);
    EXPECT_NE(sql.find("contracts"), std::string::npos);
}

TEST(TemporalDatabaseSupport, SystemTimeQueryContainsForSystemTime) {
    TemporalDatabaseSupport::TemporalSchema ts;
    ts.table_name = "orders";

    TemporalDatabaseSupport::TemporalQueryBuilder builder;
    auto sql = builder.buildSystemTimeQuery(ts, "2024-01-01");
    EXPECT_NE(sql.find("FOR SYSTEM_TIME AS OF"), std::string::npos);
}

TEST(TemporalDatabaseSupport, TemporalModelToString) {
    EXPECT_EQ(TemporalDatabaseSupport::temporalModelToString(
                TemporalDatabaseSupport::TemporalModel::BI_TEMPORAL),
              "BI_TEMPORAL");
}

// ===========================================================================
// 10. Blockchain Integrity Verifier
// ===========================================================================

TEST(BlockchainIntegrityVerifier, BuildMerkleTreeSingleRecord) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    json rec = {{"id", "1"}, {"value", "hello"}};
    auto root = builder.buildMerkleTree({rec});
    EXPECT_EQ(root.size(), 64u); // 64 hex chars
}

TEST(BlockchainIntegrityVerifier, BuildMerkleTreeMultipleRecords) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    std::vector<json> records = {
        {{"id", "1"}}, {{"id", "2"}}, {{"id", "3"}}
    };
    auto root = builder.buildMerkleTree(records);
    EXPECT_FALSE(root.empty());
}

TEST(BlockchainIntegrityVerifier, DifferentInputsDifferentRoot) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    auto r1 = builder.buildMerkleTree({{{"id", "a"}}});
    auto r2 = builder.buildMerkleTree({{{"id", "b"}}});
    EXPECT_NE(r1, r2);
}

TEST(BlockchainIntegrityVerifier, VerifyRecordInTree_SingleRecord) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    json rec = {{"id", "1"}};
    auto root = builder.buildMerkleTree({rec});
    EXPECT_TRUE(builder.verifyRecordInTree(rec, root));
}

TEST(BlockchainIntegrityVerifier, AnchorToBlockchain) {
    BlockchainIntegrityVerifier::BlockchainAnchor anchor;
    auto proof = anchor.anchorToBlockchain("a1b2c3d4e5f6" + std::string(52, '0'));
    EXPECT_FALSE(proof.merkle_root.empty());
    EXPECT_FALSE(proof.blockchain_tx_hash.empty());
    EXPECT_FALSE(proof.timestamp_rfc3339.empty());
}

TEST(BlockchainIntegrityVerifier, VerifyBlockchainAnchorValid) {
    BlockchainIntegrityVerifier::MerkleTreeBuilder builder;
    BlockchainIntegrityVerifier::BlockchainAnchor anchor;

    auto root = builder.buildMerkleTree({{{"id", "test"}}});
    auto proof = anchor.anchorToBlockchain(root);
    EXPECT_TRUE(anchor.verifyBlockchainAnchor(proof));
}

TEST(BlockchainIntegrityVerifier, VerifyBlockchainAnchorInvalidEmpty) {
    BlockchainIntegrityVerifier::BlockchainAnchor anchor;
    BlockchainIntegrityVerifier::IntegrityProof proof;
    EXPECT_FALSE(anchor.verifyBlockchainAnchor(proof));
}

// ===========================================================================
// 11. Federated Learning
// ===========================================================================

TEST(FederatedLearning, AggregateEmptyUpdates) {
    FederatedImportCoordinator::FederatedAggregator agg;
    auto result = agg.aggregateUpdates({});
    EXPECT_TRUE(result.is_object());
}

TEST(FederatedLearning, FedAvgAveragesNumericFields) {
    FederatedImportCoordinator::FederatedAggregator agg;
    FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate u1, u2;
    u1.participant_id  = "p1";
    u1.statistics      = json{{"row_count", 100.0}, {"null_rate", 0.1}};
    u1.schema_contribution = json{{"col_a", "integer"}};

    u2.participant_id  = "p2";
    u2.statistics      = json{{"row_count", 200.0}, {"null_rate", 0.2}};
    u2.schema_contribution = json{{"col_b", "text"}};

    auto result = agg.aggregateUpdates({u1, u2}, "FedAvg");
    ASSERT_TRUE(result.contains("row_count"));
    EXPECT_DOUBLE_EQ(result["row_count"].get<double>(), 150.0);
    EXPECT_EQ(result["_participants"].get<int>(), 2);
}

TEST(FederatedLearning, DifferentialPrivacyAddsNoise) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    json stats = {{"mean", 50.0}, {"std", 10.0}};

    // Run DP 10 times; at least one should differ from original
    bool any_changed = false;
    for (int i = 0; i < 10; ++i) {
        auto noisy = dp.addDifferentialPrivacy(stats, 0.1, 1e-5);
        if (noisy["mean"].get<double>() != 50.0) {
            any_changed = true;
        }
    }
    EXPECT_TRUE(any_changed);
}

TEST(FederatedLearning, DifferentialPrivacyInvalidEpsilon) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    EXPECT_THROW(dp.addDifferentialPrivacy(json{}, 0.0, 1e-5),
                 std::invalid_argument);
}

TEST(FederatedLearning, PrivacyBudgetCheck) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    EXPECT_TRUE(dp.verifyPrivacyBudget(0.5, 1e-5));
    EXPECT_FALSE(dp.verifyPrivacyBudget(2.0, 1e-5)); // budget exceeded
}

TEST(FederatedLearning, SpendBudgetAccumulates) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    dp.spendBudget(0.3);
    dp.spendBudget(0.2);
    EXPECT_DOUBLE_EQ(dp.totalEpsilonSpent(), 0.5);
}

TEST(FederatedLearning, SpendNegativeBudgetThrows) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;
    EXPECT_THROW(dp.spendBudget(-0.1), std::invalid_argument);
}

// ===========================================================================
// 12. GraphQL Federation Support
// ===========================================================================

TEST(GraphQLFederationSupport, GenerateFederatedSchema_ContainsTypeName) {
    auto schema = makeSchema("users", {"id", "name", "email"}, {"id"});
    schema.column_types["id"]    = "uuid";
    schema.column_types["name"]  = "varchar";
    schema.column_types["email"] = "text";

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generateFederatedSchema({schema}, "user-service");

    EXPECT_NE(sdl.find("type Users"), std::string::npos);
    EXPECT_NE(sdl.find("@key"), std::string::npos);
    EXPECT_NE(sdl.find("user-service"), std::string::npos);
}

TEST(GraphQLFederationSupport, GenerateFederatedSchema_FKAnnotation) {
    auto schema = makeSchema("orders", {"id", "user_id"}, {"id"},
                             {{"user_id", "users.id"}});

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generateFederatedSchema({schema}, "order-service");

    // FK column should have a comment annotation
    EXPECT_NE(sdl.find("FK ->"), std::string::npos);
}

TEST(GraphQLFederationSupport, GeneratePlainSchema_HasQueryType) {
    auto schema = makeSchema("products", {"id", "name"}, {"id"});

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generatePlainSchema({schema});

    EXPECT_NE(sdl.find("type Query"), std::string::npos);
    EXPECT_NE(sdl.find("products(id:"), std::string::npos);
}

TEST(GraphQLFederationSupport, TypeMapping_Integer) {
    auto schema = makeSchema("t", {"count"});
    schema.column_types["count"] = "integer";

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generatePlainSchema({schema});
    EXPECT_NE(sdl.find("count: Int"), std::string::npos);
}

TEST(GraphQLFederationSupport, TypeMapping_Boolean) {
    auto schema = makeSchema("t", {"active"});
    schema.column_types["active"] = "boolean";

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    auto sdl = gen.generatePlainSchema({schema});
    EXPECT_NE(sdl.find("active: Boolean"), std::string::npos);
}

TEST(GraphQLFederationSupport, ExternalEntityAnnotation) {
    auto user_schema  = makeSchema("users",  {"id"}, {"id"});
    auto order_schema = makeSchema("orders", {"id", "user_id"}, {"id"},
                                   {{"user_id", "users.id"}});

    GraphQLFederationSupport::GraphQLSchemaGenerator gen;
    // "users" is owned by another subgraph → should be "extend type"
    auto sdl = gen.generateFederatedSchema(
        {user_schema, order_schema}, "order-service", {"users"});

    EXPECT_NE(sdl.find("extend type Users"), std::string::npos);
}

// ===========================================================================
// main
// ===========================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
