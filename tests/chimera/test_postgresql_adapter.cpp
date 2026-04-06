/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_postgresql_adapter.cpp                        ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:22:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     940                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • cab29d448f  2026-02-26  audit(chimera): fix all gaps found in PostgreSQL adapter ... ║
    • 261a690e71  2026-02-26  feat(chimera): implement PostgreSQL + pgvector adapter fo... ║
    • 5554ae8cdc  2026-02-22  Code audit and bugfix: fix document_matches id field, mas... ║
    • d34adc2bf9  2026-02-22  Implement MongoDB vendor adapter for Chimera module ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_postgresql_adapter.cpp
 * @brief Unit tests for PostgreSQLAdapter (CHIMERA Suite)
 *
 * @details Tests cover connection management, relational CRUD, vector search
 *          (pgvector simulation), document JSONB operations, transaction
 *          lifecycle, capability reporting, security (credential masking),
 *          NOT_IMPLEMENTED paths for unsupported operations (graph), and
 *          basic overhead benchmarks.
 *
 * All tests run without a live PostgreSQL server – the adapter operates in
 * its in-process simulation mode.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/postgresql_adapter.hpp"

#include <chrono>

using namespace chimera;

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------

static Document make_doc(const std::string& id,
                          const std::string& name,
                          int64_t value) {
    Document doc;
    doc.id = id;
    doc.fields["name"]  = Scalar{name};
    doc.fields["value"] = Scalar{value};
    return doc;
}

static RelationalRow make_row(const std::string& name, int64_t value) {
    RelationalRow row;
    row.columns["name"]  = Scalar{name};
    row.columns["value"] = Scalar{value};
    return row;
}

// ---------------------------------------------------------------------------
// Connection tests
// ---------------------------------------------------------------------------

class PostgreSQLConnectionTest : public ::testing::Test {
protected:
    PostgreSQLAdapter adapter;
};

TEST_F(PostgreSQLConnectionTest, InitiallyDisconnected) {
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PostgreSQLConnectionTest, ConnectWithPostgresqlScheme) {
    auto result = adapter.connect("postgresql://localhost:5432/testdb");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(PostgreSQLConnectionTest, ConnectWithPostgresShortScheme) {
    auto result = adapter.connect("postgres://localhost:5432/testdb");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(PostgreSQLConnectionTest, ConnectWithCredentials) {
    auto result = adapter.connect("postgresql://user:pass@localhost:5432/mydb");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(PostgreSQLConnectionTest, ConnectWithEmptyStringReturnsError) {
    auto result = adapter.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PostgreSQLConnectionTest, ConnectWithInvalidSchemeReturnsError) {
    auto result = adapter.connect("mongodb://localhost:27017/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PostgreSQLConnectionTest, ConnectWithMysqlSchemeReturnsError) {
    auto result = adapter.connect("mysql://localhost:3306/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PostgreSQLConnectionTest, Disconnect) {
    adapter.connect("postgresql://localhost:5432/testdb");
    EXPECT_TRUE(adapter.is_connected());

    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PostgreSQLConnectionTest, ConnectWithoutDatabaseNameDefaultsToPostgres) {
    auto result = adapter.connect("postgresql://localhost:5432");
    EXPECT_TRUE(result.is_ok());

    auto info = adapter.get_system_info();
    ASSERT_TRUE(info.is_ok());
    EXPECT_EQ(std::get<std::string>(info.value.value().configuration.at("database")),
              "postgres");
}

// ---------------------------------------------------------------------------
// Relational adapter tests
// ---------------------------------------------------------------------------

class PostgreSQLRelationalTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("postgresql://localhost:5432/testdb");
    }

    PostgreSQLAdapter adapter;
    static constexpr const char* kTable = "test_table";
};

TEST_F(PostgreSQLRelationalTest, InsertRow) {
    auto row = make_row("Alice", 42);
    auto result = adapter.insert_row(kTable, row);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 1u);
}

TEST_F(PostgreSQLRelationalTest, InsertRowWithEmptyTableNameReturnsError) {
    auto row = make_row("Alice", 42);
    auto result = adapter.insert_row("", row);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PostgreSQLRelationalTest, InsertRowWithEmptyRowReturnsError) {
    auto result = adapter.insert_row(kTable, RelationalRow{});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PostgreSQLRelationalTest, BatchInsertRows) {
    std::vector<RelationalRow> rows;
    for (int i = 0; i < 5; ++i) {
        rows.push_back(make_row("Name" + std::to_string(i), static_cast<int64_t>(i)));
    }
    auto result = adapter.batch_insert(kTable, rows);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 5u);
}

TEST_F(PostgreSQLRelationalTest, BatchInsertEmptyIsOk) {
    auto result = adapter.batch_insert(kTable, {});
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 0u);
}

TEST_F(PostgreSQLRelationalTest, ExecuteQueryReturnsEmptyResultInSimulation) {
    auto result = adapter.execute_query("SELECT * FROM test_table");
    ASSERT_TRUE(result.is_ok());
}

TEST_F(PostgreSQLRelationalTest, ExecuteQueryWithEmptyStringReturnsError) {
    auto result = adapter.execute_query("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PostgreSQLRelationalTest, GetQueryStatisticsReturnsOk) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

TEST_F(PostgreSQLRelationalTest, OperationsWhileDisconnectedReturnError) {
    adapter.disconnect();

    auto r1 = adapter.insert_row(kTable, make_row("X", 1));
    EXPECT_EQ(r1.error_code, ErrorCode::CONNECTION_ERROR);

    auto r2 = adapter.batch_insert(kTable, {make_row("Y", 2)});
    EXPECT_EQ(r2.error_code, ErrorCode::CONNECTION_ERROR);

    auto r3 = adapter.execute_query("SELECT 1");
    EXPECT_EQ(r3.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Vector adapter tests (pgvector simulation)
// ---------------------------------------------------------------------------

class PostgreSQLVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("postgresql://localhost:5432/vectordb");
    }

    PostgreSQLAdapter adapter;
    static constexpr const char* kCollection = "embeddings";
};

TEST_F(PostgreSQLVectorTest, InsertAndSearchVectors) {
    Vector v1, v2, v3;
    v1.data = {1.0f, 0.0f, 0.0f};
    v2.data = {0.0f, 1.0f, 0.0f};
    v3.data = {1.0f, 1.0f, 0.0f};

    ASSERT_TRUE(adapter.insert_vector(kCollection, v1).is_ok());
    ASSERT_TRUE(adapter.insert_vector(kCollection, v2).is_ok());
    ASSERT_TRUE(adapter.insert_vector(kCollection, v3).is_ok());

    Vector query;
    query.data = {1.0f, 0.1f, 0.0f};

    auto result = adapter.search_vectors(kCollection, query, 2);
    ASSERT_TRUE(result.is_ok());
    EXPECT_LE(result.value.value().size(), 2u);
    EXPECT_GT(result.value.value()[0].second, 0.0);
}

TEST_F(PostgreSQLVectorTest, SearchReturnsSortedByDescendingSimilarity) {
    Vector v1, v2;
    v1.data = {1.0f, 0.0f};
    v2.data = {0.0f, 1.0f};

    adapter.insert_vector(kCollection, v1);
    adapter.insert_vector(kCollection, v2);

    Vector query;
    query.data = {0.9f, 0.1f};

    auto result = adapter.search_vectors(kCollection, query, 10);
    ASSERT_TRUE(result.is_ok());
    const auto& hits = result.value.value();
    ASSERT_EQ(hits.size(), 2u);
    EXPECT_GE(hits[0].second, hits[1].second);
}

TEST_F(PostgreSQLVectorTest, InsertVectorWithMetadata) {
    Vector v;
    v.data = {0.5f, 0.5f};
    v.metadata["category"] = Scalar{std::string{"embeddings"}};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(PostgreSQLVectorTest, SearchWithMetadataFilter) {
    Vector v1, v2;
    v1.data = {1.0f, 0.0f};
    v1.metadata["category"] = Scalar{std::string{"A"}};
    v2.data = {1.0f, 0.0f};
    v2.metadata["category"] = Scalar{std::string{"B"}};

    adapter.insert_vector(kCollection, v1);
    adapter.insert_vector(kCollection, v2);

    Vector query;
    query.data = {1.0f, 0.0f};

    std::map<std::string, Scalar> filter;
    filter["category"] = Scalar{std::string{"A"}};

    auto result = adapter.search_vectors(kCollection, query, 10, filter);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 1u);
}

TEST_F(PostgreSQLVectorTest, InsertEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.insert_vector(kCollection, empty);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PostgreSQLVectorTest, SearchEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.search_vectors(kCollection, empty, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PostgreSQLVectorTest, SearchEmptyCollectionReturnsEmpty) {
    Vector query;
    query.data = {1.0f, 0.0f};

    auto result = adapter.search_vectors("empty_col", query, 5);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(PostgreSQLVectorTest, BatchInsertVectors) {
    std::vector<Vector> vecs(3);
    vecs[0].data = {1.0f, 0.0f};
    vecs[1].data = {0.0f, 1.0f};
    vecs[2].data = {0.5f, 0.5f};

    auto result = adapter.batch_insert_vectors(kCollection, vecs);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 3u);
}

TEST_F(PostgreSQLVectorTest, CreateIndexSucceeds) {
    auto result = adapter.create_index(kCollection, 128);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(PostgreSQLVectorTest, CreateIndexWithHnswParamsSucceeds) {
    std::map<std::string, Scalar> params;
    params["index_type"]  = Scalar{std::string{"hnsw"}};
    params["m"]           = Scalar{int64_t{16}};
    params["ef_construction"] = Scalar{int64_t{64}};

    auto result = adapter.create_index(kCollection, 128, params);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(PostgreSQLVectorTest, VectorIdGeneratedIsNotEmpty) {
    Vector v;
    v.data = {1.0f, 2.0f, 3.0f};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("pg_doc_"), std::string::npos);
}

TEST_F(PostgreSQLVectorTest, OperationsWhileDisconnectedReturnError) {
    adapter.disconnect();

    Vector v;
    v.data = {1.0f, 0.0f};

    auto r1 = adapter.insert_vector(kCollection, v);
    EXPECT_EQ(r1.error_code, ErrorCode::CONNECTION_ERROR);

    auto r2 = adapter.search_vectors(kCollection, v, 5);
    EXPECT_EQ(r2.error_code, ErrorCode::CONNECTION_ERROR);

    auto r3 = adapter.batch_insert_vectors(kCollection, {v});
    EXPECT_EQ(r3.error_code, ErrorCode::CONNECTION_ERROR);

    auto r4 = adapter.create_index(kCollection, 3);
    EXPECT_EQ(r4.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Document adapter tests (JSONB simulation)
// ---------------------------------------------------------------------------

class PostgreSQLDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("postgresql://localhost:5432/testdb");
    }

    PostgreSQLAdapter adapter;
    static constexpr const char* kCollection = "jsonb_docs";
};

TEST_F(PostgreSQLDocumentTest, InsertAndRetrieveDocument) {
    auto doc = make_doc("doc1", "Alice", 42);
    auto insert_result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(insert_result.is_ok());
    EXPECT_EQ(insert_result.value.value(), "doc1");

    auto find_result = adapter.find_documents(kCollection, {});
    ASSERT_TRUE(find_result.is_ok());
    ASSERT_EQ(find_result.value.value().size(), 1u);
    EXPECT_EQ(find_result.value.value()[0].id, "doc1");
}

TEST_F(PostgreSQLDocumentTest, InsertGeneratesIdWhenEmpty) {
    Document doc;
    doc.fields["name"] = Scalar{std::string{"NoId"}};
    auto result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("pg_doc_"), std::string::npos);
}

TEST_F(PostgreSQLDocumentTest, InsertDuplicateIdReturnsError) {
    auto doc = make_doc("dup1", "Bob", 10);
    adapter.insert_document(kCollection, doc);

    auto dup_result = adapter.insert_document(kCollection, doc);
    EXPECT_TRUE(dup_result.is_err());
    EXPECT_EQ(dup_result.error_code, ErrorCode::ALREADY_EXISTS);
}

TEST_F(PostgreSQLDocumentTest, InsertEmptyCollectionNameReturnsError) {
    auto doc = make_doc("x", "X", 0);
    auto result = adapter.insert_document("", doc);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PostgreSQLDocumentTest, BatchInsertDocuments) {
    std::vector<Document> docs;
    for (int i = 0; i < 5; ++i) {
        docs.push_back(make_doc("bdoc" + std::to_string(i),
                                "Name" + std::to_string(i),
                                static_cast<int64_t>(i)));
    }
    auto result = adapter.batch_insert_documents(kCollection, docs);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 5u);
}

TEST_F(PostgreSQLDocumentTest, FindWithFilter) {
    adapter.insert_document(kCollection, make_doc("f1", "Alice", 1));
    adapter.insert_document(kCollection, make_doc("f2", "Bob",   2));
    adapter.insert_document(kCollection, make_doc("f3", "Alice", 3));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Alice"}};

    auto result = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 2u);
}

TEST_F(PostgreSQLDocumentTest, FindWithLimit) {
    for (int i = 0; i < 10; ++i) {
        adapter.insert_document(kCollection,
            make_doc("lim" + std::to_string(i), "User", static_cast<int64_t>(i)));
    }

    auto result = adapter.find_documents(kCollection, {}, /*limit=*/3);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 3u);
}

TEST_F(PostgreSQLDocumentTest, FindInNonExistentCollectionReturnsEmpty) {
    auto result = adapter.find_documents("no_such_collection", {});
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(PostgreSQLDocumentTest, FindByIdField) {
    adapter.insert_document(kCollection, make_doc("alpha", "A", 1));
    adapter.insert_document(kCollection, make_doc("beta",  "B", 2));

    std::map<std::string, Scalar> filter;
    filter["id"] = Scalar{std::string{"alpha"}};

    auto result = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "alpha");
}

TEST_F(PostgreSQLDocumentTest, UpdateDocuments) {
    adapter.insert_document(kCollection, make_doc("u1", "Charlie", 5));
    adapter.insert_document(kCollection, make_doc("u2", "Charlie", 6));
    adapter.insert_document(kCollection, make_doc("u3", "Dave",    7));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Charlie"}};

    std::map<std::string, Scalar> updates;
    updates["name"] = Scalar{std::string{"Charles"}};

    auto result = adapter.update_documents(kCollection, filter, updates);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 2u);

    // Verify update was applied
    std::map<std::string, Scalar> verify_filter;
    verify_filter["name"] = Scalar{std::string{"Charles"}};
    auto verify = adapter.find_documents(kCollection, verify_filter);
    ASSERT_TRUE(verify.is_ok());
    EXPECT_EQ(verify.value.value().size(), 2u);
}

TEST_F(PostgreSQLDocumentTest, UpdateWithEmptyUpdatesReturnsError) {
    adapter.insert_document(kCollection, make_doc("eu1", "Eve", 1));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Eve"}};

    auto result = adapter.update_documents(kCollection, filter, {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PostgreSQLDocumentTest, UpdateNonExistentCollectionReturnsZero) {
    std::map<std::string, Scalar> updates;
    updates["x"] = Scalar{int64_t{1}};

    auto result = adapter.update_documents("missing_coll", {}, updates);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 0u);
}

TEST_F(PostgreSQLDocumentTest, OperationsWhileDisconnectedReturnError) {
    adapter.disconnect();

    auto r1 = adapter.insert_document(kCollection, make_doc("x", "x", 0));
    EXPECT_EQ(r1.error_code, ErrorCode::CONNECTION_ERROR);

    auto r2 = adapter.find_documents(kCollection, {});
    EXPECT_EQ(r2.error_code, ErrorCode::CONNECTION_ERROR);

    std::map<std::string, Scalar> upd;
    upd["k"] = Scalar{int64_t{1}};
    auto r3 = adapter.update_documents(kCollection, {}, upd);
    EXPECT_EQ(r3.error_code, ErrorCode::CONNECTION_ERROR);

    auto r4 = adapter.batch_insert_documents(kCollection, {});
    EXPECT_EQ(r4.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Transaction tests
// ---------------------------------------------------------------------------

class PostgreSQLTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("postgresql://localhost:5432/txndb");
    }

    PostgreSQLAdapter adapter;
};

TEST_F(PostgreSQLTransactionTest, BeginCommitCycle) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());
    const std::string txn_id = begin.value.value();
    EXPECT_FALSE(txn_id.empty());
    EXPECT_NE(txn_id.find("pg_txn_"), std::string::npos);

    auto commit = adapter.commit_transaction(txn_id);
    EXPECT_TRUE(commit.is_ok());
}

TEST_F(PostgreSQLTransactionTest, BeginRollbackCycle) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());
    const std::string txn_id = begin.value.value();

    auto rollback = adapter.rollback_transaction(txn_id);
    EXPECT_TRUE(rollback.is_ok());
}

TEST_F(PostgreSQLTransactionTest, CommitUnknownTransactionReturnsNotFound) {
    auto result = adapter.commit_transaction("no_such_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(PostgreSQLTransactionTest, RollbackUnknownTransactionReturnsNotFound) {
    auto result = adapter.rollback_transaction("no_such_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(PostgreSQLTransactionTest, DoubleCommitReturnsTransactionAborted) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());
    const std::string txn_id = begin.value.value();

    ASSERT_TRUE(adapter.commit_transaction(txn_id).is_ok());

    auto second_commit = adapter.commit_transaction(txn_id);
    EXPECT_TRUE(second_commit.is_err());
    EXPECT_EQ(second_commit.error_code, ErrorCode::TRANSACTION_ABORTED);
}

TEST_F(PostgreSQLTransactionTest, DoubleRollbackReturnsTransactionAborted) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());
    const std::string txn_id = begin.value.value();

    ASSERT_TRUE(adapter.rollback_transaction(txn_id).is_ok());

    auto second_rollback = adapter.rollback_transaction(txn_id);
    EXPECT_TRUE(second_rollback.is_err());
    EXPECT_EQ(second_rollback.error_code, ErrorCode::TRANSACTION_ABORTED);
}

TEST_F(PostgreSQLTransactionTest, MultipleIndependentTransactions) {
    auto t1 = adapter.begin_transaction();
    auto t2 = adapter.begin_transaction();
    ASSERT_TRUE(t1.is_ok());
    ASSERT_TRUE(t2.is_ok());
    EXPECT_NE(t1.value.value(), t2.value.value());

    EXPECT_TRUE(adapter.commit_transaction(t1.value.value()).is_ok());
    EXPECT_TRUE(adapter.rollback_transaction(t2.value.value()).is_ok());
}

TEST_F(PostgreSQLTransactionTest, TransactionWithIsolationLevelOptions) {
    TransactionOptions opts;
    opts.isolation_level = TransactionOptions::IsolationLevel::SERIALIZABLE;
    opts.read_only = false;

    auto begin = adapter.begin_transaction(opts);
    ASSERT_TRUE(begin.is_ok());

    EXPECT_TRUE(adapter.commit_transaction(begin.value.value()).is_ok());
}

TEST_F(PostgreSQLTransactionTest, OperationsWhileDisconnectedReturnError) {
    adapter.disconnect();

    auto r1 = adapter.begin_transaction();
    EXPECT_EQ(r1.error_code, ErrorCode::CONNECTION_ERROR);

    auto r2 = adapter.commit_transaction("x");
    EXPECT_EQ(r2.error_code, ErrorCode::CONNECTION_ERROR);

    auto r3 = adapter.rollback_transaction("x");
    EXPECT_EQ(r3.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// NOT_IMPLEMENTED paths (graph operations)
// ---------------------------------------------------------------------------

class PostgreSQLNotImplementedTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("postgresql://localhost:5432/testdb");
    }

    PostgreSQLAdapter adapter;
};

TEST_F(PostgreSQLNotImplementedTest, InsertNodeNotImplemented) {
    auto result = adapter.insert_node(GraphNode{});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PostgreSQLNotImplementedTest, InsertEdgeNotImplemented) {
    auto result = adapter.insert_edge(GraphEdge{});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PostgreSQLNotImplementedTest, ShortestPathNotImplemented) {
    auto result = adapter.shortest_path("a", "b");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PostgreSQLNotImplementedTest, TraverseNotImplemented) {
    auto result = adapter.traverse("a", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PostgreSQLNotImplementedTest, ExecuteGraphQueryNotImplemented) {
    auto result = adapter.execute_graph_query("MATCH (n) RETURN n");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// Capability and system info tests
// ---------------------------------------------------------------------------

class PostgreSQLCapabilityTest : public ::testing::Test {
protected:
    PostgreSQLAdapter adapter;
};

TEST_F(PostgreSQLCapabilityTest, SupportedCapabilities) {
    EXPECT_TRUE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(adapter.has_capability(Capability::VECTOR_SEARCH));
    EXPECT_TRUE(adapter.has_capability(Capability::DOCUMENT_STORE));
    EXPECT_TRUE(adapter.has_capability(Capability::FULL_TEXT_SEARCH));
    EXPECT_TRUE(adapter.has_capability(Capability::TRANSACTIONS));
    EXPECT_TRUE(adapter.has_capability(Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(adapter.has_capability(Capability::SECONDARY_INDEXES));
    EXPECT_TRUE(adapter.has_capability(Capability::MATERIALIZED_VIEWS));
}

TEST_F(PostgreSQLCapabilityTest, UnsupportedCapabilities) {
    EXPECT_FALSE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(adapter.has_capability(Capability::DISTRIBUTED_QUERIES));
    EXPECT_FALSE(adapter.has_capability(Capability::TIME_SERIES));
    EXPECT_FALSE(adapter.has_capability(Capability::STREAM_PROCESSING));
    EXPECT_FALSE(adapter.has_capability(Capability::SHARDING));
}

TEST_F(PostgreSQLCapabilityTest, GetCapabilitiesReturnsNonEmpty) {
    auto caps = adapter.get_capabilities();
    EXPECT_FALSE(caps.empty());
    EXPECT_GE(caps.size(), 8u);
}

TEST_F(PostgreSQLCapabilityTest, SystemInfoReturnsPostgreSQLName) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().system_name, "PostgreSQL");
    EXPECT_FALSE(result.value.value().version.empty());
}

TEST_F(PostgreSQLCapabilityTest, SystemInfoIncludesPgvectorExtension) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    auto it = result.value.value().build_info.find("extensions");
    ASSERT_NE(it, result.value.value().build_info.end());
    EXPECT_NE(it->second.find("pgvector"), std::string::npos);
}

TEST_F(PostgreSQLCapabilityTest, GetMetricsReturnsOk) {
    auto result = adapter.get_metrics();
    EXPECT_TRUE(result.is_ok());
}

TEST_F(PostgreSQLCapabilityTest, QueryStatisticsReturnsOk) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Security audit: credential handling
// ---------------------------------------------------------------------------

class PostgreSQLSecurityTest : public ::testing::Test {
protected:
    PostgreSQLAdapter adapter;
};

TEST_F(PostgreSQLSecurityTest, CredentialsNotExposedInSystemInfo) {
    adapter.connect("postgresql://admin:s3cr3t@localhost:5432/mydb");

    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    const auto& info = result.value.value();

    for (const auto& kv : info.build_info) {
        EXPECT_EQ(kv.second.find("s3cr3t"), std::string::npos)
            << "Credential leaked in build_info[" << kv.first << "]";
    }
    for (const auto& kv : info.configuration) {
        if (std::holds_alternative<std::string>(kv.second)) {
            EXPECT_EQ(std::get<std::string>(kv.second).find("s3cr3t"),
                      std::string::npos)
                << "Credential leaked in configuration[" << kv.first << "]";
        }
    }
    ASSERT_TRUE(info.configuration.count("database") > 0);
    EXPECT_EQ(std::get<std::string>(info.configuration.at("database")), "mydb");
}

TEST_F(PostgreSQLSecurityTest, CredentialsNotExposedWithoutUserInfo) {
    adapter.connect("postgresql://localhost:5432/cleandb");

    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(
        std::get<std::string>(result.value.value().configuration.at("database")),
        "cleandb"
    );
}

TEST_F(PostgreSQLSecurityTest, ShortSchemeCredentialsNotExposed) {
    adapter.connect("postgres://user:pass@host:5432/proddb");

    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    for (const auto& kv : result.value.value().build_info) {
        EXPECT_EQ(kv.second.find("pass"), std::string::npos)
            << "Credential leaked in build_info[" << kv.first << "]";
    }
}

// ---------------------------------------------------------------------------
// AdapterFactory integration tests
// ---------------------------------------------------------------------------

class PostgreSQLFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register PostgreSQL adapter once; subsequent calls are no-ops (factory
        // rejects duplicate registrations) so tests are safe to run in any order.
        AdapterFactory::register_adapter("PostgreSQL",
            []() { return std::make_unique<PostgreSQLAdapter>(); });
    }

    static constexpr const char* kConnString = "postgresql://localhost:5432/testdb";
};

TEST_F(PostgreSQLFactoryTest, RegisterAndCreate) {
    EXPECT_TRUE(AdapterFactory::is_supported("PostgreSQL"));

    auto adapter = AdapterFactory::create("PostgreSQL");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(adapter->is_connected());
}

TEST_F(PostgreSQLFactoryTest, CapabilitiesViaFactory) {
    auto adapter = AdapterFactory::create("PostgreSQL");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(adapter->has_capability(Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(adapter->has_capability(Capability::VECTOR_SEARCH));
    EXPECT_FALSE(adapter->has_capability(Capability::GRAPH_TRAVERSAL));
}

TEST_F(PostgreSQLFactoryTest, ConnectViaFactory) {
    auto adapter = AdapterFactory::create("PostgreSQL");
    ASSERT_NE(adapter, nullptr);

    auto result = adapter->connect(kConnString);
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter->is_connected());
}

// ---------------------------------------------------------------------------
// Performance / overhead benchmarks
// ---------------------------------------------------------------------------

class PostgreSQLPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("postgresql://localhost:5432/perfdb");
    }

    PostgreSQLAdapter adapter;
    static constexpr const char* kTable      = "perf_rows";
    static constexpr const char* kVecCol     = "perf_vecs";

    static constexpr size_t kRowCount   = 1000;
    static constexpr size_t kVecDim     = 32;
    static constexpr size_t kVecCount   = 200;
    static constexpr size_t kSearchK    = 10;
    static constexpr size_t kSearchRuns = 20;

    static constexpr int64_t kBulkRowMs  = 2000;
    static constexpr int64_t kBulkVecMs  = 2000;
    static constexpr int64_t kSearchMs   = 2000;
};

TEST_F(PostgreSQLPerformanceTest, BulkRowInsertOverhead) {
    std::vector<RelationalRow> rows;
    rows.reserve(kRowCount);
    for (size_t i = 0; i < kRowCount; ++i) {
        RelationalRow row;
        row.columns["idx"]  = Scalar{int64_t(i)};
        row.columns["data"] = Scalar{std::string(32, 'x')};
        rows.push_back(std::move(row));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto result = adapter.batch_insert(kTable, rows);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), kRowCount);

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkRowMs)
        << "Bulk insert of " << kRowCount << " rows took "
        << elapsed_ms << "ms (limit: " << kBulkRowMs << "ms)";
}

TEST_F(PostgreSQLPerformanceTest, BulkVectorInsertOverhead) {
    std::vector<Vector> vecs;
    vecs.reserve(kVecCount);
    for (size_t i = 0; i < kVecCount; ++i) {
        Vector v;
        v.data.resize(kVecDim, static_cast<float>(i) / kVecCount);
        vecs.push_back(std::move(v));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto result = adapter.batch_insert_vectors(kVecCol, vecs);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), kVecCount);

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkVecMs)
        << "Bulk insert of " << kVecCount << " vectors (" << kVecDim
        << "D) took " << elapsed_ms << "ms (limit: " << kBulkVecMs << "ms)";
}

TEST_F(PostgreSQLPerformanceTest, VectorSearchOverhead) {
    for (size_t i = 0; i < kVecCount; ++i) {
        Vector v;
        v.data.resize(kVecDim, static_cast<float>(i) / kVecCount);
        adapter.insert_vector("search_vecs", v);
    }

    Vector query;
    query.data.resize(kVecDim, 0.5f);

    auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < kSearchRuns; ++i) {
        auto result = adapter.search_vectors("search_vecs", query, kSearchK);
        ASSERT_TRUE(result.is_ok());
    }
    auto t1 = std::chrono::steady_clock::now();

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kSearchMs)
        << kSearchRuns << " vector searches (k=" << kSearchK << ") over "
        << kVecCount << " vectors took " << elapsed_ms
        << "ms (limit: " << kSearchMs << "ms)";
}
