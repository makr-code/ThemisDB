/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_themisdb_adapter.cpp                          ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 04:00:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     584                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a0b50b625  2026-02-28  feat(chimera): add test_themisdb_adapter.cpp with adapter... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_themisdb_adapter.cpp
 * @brief Unit tests and performance overhead benchmarks for ThemisDBAdapter
 *
 * @details
 * Tests cover:
 *   - Connection management (connect, disconnect, invalid URI)
 *   - Relational operations (execute_query, insert_row, batch_insert)
 *   - Vector operations (insert_vector, batch_insert_vectors, search_vectors,
 *     create_index)
 *   - Graph operations (insert_node, insert_edge, shortest_path, traverse,
 *     execute_graph_query)
 *   - Document operations (insert_document, batch_insert_documents,
 *     find_documents, update_documents)
 *   - Transaction lifecycle (begin, commit, rollback)
 *   - Capability reporting and system information
 *   - Performance overhead benchmarks (adapter call overhead measurement)
 *
 * All tests run without a live ThemisDB server – the adapter operates in
 * its in-process simulation mode.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"

#include <chrono>
#include <string>
#include <vector>

using namespace chimera;

// ---------------------------------------------------------------------------
// Helpers
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

static Vector make_vector(size_t dim, float fill) {
    Vector v;
    v.data.assign(dim, fill);
    return v;
}

// ---------------------------------------------------------------------------
// Connection tests
// ---------------------------------------------------------------------------

class ThemisDBConnectionTest : public ::testing::Test {
protected:
    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBConnectionTest, InitiallyDisconnected) {
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(ThemisDBConnectionTest, ConnectWithValidUri) {
    auto result = adapter.connect("themisdb://localhost:7777");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(ThemisDBConnectionTest, ConnectWithOptionsSucceeds) {
    auto result = adapter.connect("themisdb://localhost:7777",
                                  {{"timeout", "5000"}, {"pool_size", "10"}});
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(ThemisDBConnectionTest, DisconnectAfterConnect) {
    adapter.connect("themisdb://localhost:7777");
    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(ThemisDBConnectionTest, DisconnectWhenAlreadyDisconnected) {
    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

// ---------------------------------------------------------------------------
// Relational operations
// ---------------------------------------------------------------------------

class ThemisDBRelationalTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBRelationalTest, ExecuteQueryReturnsOk) {
    auto result = adapter.execute_query("SELECT * FROM users");
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBRelationalTest, ExecuteQueryWithParams) {
    auto result = adapter.execute_query(
        "SELECT * FROM users WHERE id = ?",
        {Scalar{int64_t(42)}}
    );
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBRelationalTest, ExecuteQueryWhenDisconnectedReturnsError) {
    adapter.disconnect();
    auto result = adapter.execute_query("SELECT 1");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(ThemisDBRelationalTest, InsertRowReturnsOk) {
    RelationalRow row;
    row.columns["name"]  = Scalar{std::string("Alice")};
    row.columns["score"] = Scalar{int64_t(100)};

    auto result = adapter.insert_row("users", row);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 1u);
}

TEST_F(ThemisDBRelationalTest, BatchInsertReturnsCorrectCount) {
    std::vector<RelationalRow> rows(5);
    for (size_t i = 0; i < rows.size(); ++i) {
        rows[i].columns["idx"] = Scalar{int64_t(i)};
    }

    auto result = adapter.batch_insert("users", rows);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), rows.size());
}

TEST_F(ThemisDBRelationalTest, GetQueryStatisticsReturnsOk) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Vector operations
// ---------------------------------------------------------------------------

class ThemisDBVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
    static constexpr const char* kCollection = "embeddings";
    static constexpr size_t kDim = 16;
};

TEST_F(ThemisDBVectorTest, CreateIndexReturnsOk) {
    auto result = adapter.create_index(kCollection, kDim);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBVectorTest, InsertVectorReturnsId) {
    auto v = make_vector(kDim, 0.5f);
    auto result = adapter.insert_vector(kCollection, v);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ThemisDBVectorTest, BatchInsertVectorsReturnsCorrectCount) {
    std::vector<Vector> vecs;
    for (size_t i = 0; i < 10; ++i) {
        vecs.push_back(make_vector(kDim, static_cast<float>(i) / 10.0f));
    }

    auto result = adapter.batch_insert_vectors(kCollection, vecs);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), vecs.size());
}

TEST_F(ThemisDBVectorTest, SearchVectorsReturnsOk) {
    auto query = make_vector(kDim, 0.5f);
    auto result = adapter.search_vectors(kCollection, query, 5);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBVectorTest, SearchVectorsWhenDisconnectedReturnsError) {
    adapter.disconnect();
    auto query = make_vector(kDim, 0.5f);
    auto result = adapter.search_vectors(kCollection, query, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Graph operations
// ---------------------------------------------------------------------------

class ThemisDBGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBGraphTest, InsertNodeReturnsId) {
    GraphNode node;
    node.id    = "node_1";
    node.label = "Person";
    node.properties["name"] = Scalar{std::string("Alice")};

    auto result = adapter.insert_node(node);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "node_1");
}

TEST_F(ThemisDBGraphTest, InsertNodeWithEmptyIdGeneratesId) {
    GraphNode node;
    node.label = "Entity";

    auto result = adapter.insert_node(node);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ThemisDBGraphTest, InsertEdgeReturnsId) {
    GraphEdge edge;
    edge.id        = "edge_1";
    edge.source_id = "node_1";
    edge.target_id = "node_2";
    edge.label     = "KNOWS";

    auto result = adapter.insert_edge(edge);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "edge_1");
}

TEST_F(ThemisDBGraphTest, ShortestPathReturnsOk) {
    auto result = adapter.shortest_path("node_1", "node_2");
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBGraphTest, TraverseReturnsOk) {
    auto result = adapter.traverse("node_1", 3);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBGraphTest, ExecuteGraphQueryReturnsOk) {
    auto result = adapter.execute_graph_query(
        "FOR v IN 1..2 OUTBOUND 'node_1' GRAPH 'g' RETURN v"
    );
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Document operations
// ---------------------------------------------------------------------------

class ThemisDBDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
    static constexpr const char* kCollection = "items";
};

TEST_F(ThemisDBDocumentTest, InsertDocumentReturnsId) {
    auto doc = make_doc("doc_1", "Item A", 10);
    auto result = adapter.insert_document(kCollection, doc);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "doc_1");
}

TEST_F(ThemisDBDocumentTest, InsertDocumentWithEmptyIdGeneratesId) {
    Document doc;
    doc.fields["x"] = Scalar{int64_t(1)};

    auto result = adapter.insert_document(kCollection, doc);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ThemisDBDocumentTest, BatchInsertDocumentsReturnsCorrectCount) {
    std::vector<Document> docs;
    for (size_t i = 0; i < 8; ++i) {
        docs.push_back(make_doc("d_" + std::to_string(i), "item", int64_t(i)));
    }

    auto result = adapter.batch_insert_documents(kCollection, docs);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), docs.size());
}

TEST_F(ThemisDBDocumentTest, FindDocumentsReturnsOk) {
    auto result = adapter.find_documents(kCollection, {{"name", Scalar{std::string("Item A")}}});
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBDocumentTest, UpdateDocumentsReturnsOk) {
    auto result = adapter.update_documents(
        kCollection,
        {{"name", Scalar{std::string("Item A")}}},
        {{"value", Scalar{int64_t(99)}}}
    );
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ThemisDBDocumentTest, BatchInsertWhenDisconnectedReturnsError) {
    adapter.disconnect();
    std::vector<Document> docs = {make_doc("x", "y", 0)};
    auto result = adapter.batch_insert_documents(kCollection, docs);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Transaction lifecycle
// ---------------------------------------------------------------------------

class ThemisDBTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBTransactionTest, BeginTransactionReturnsTxnId) {
    auto result = adapter.begin_transaction();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ThemisDBTransactionTest, CommitTransactionReturnsOk) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());

    auto commit = adapter.commit_transaction(begin.value.value());
    EXPECT_TRUE(commit.is_ok());
}

TEST_F(ThemisDBTransactionTest, RollbackTransactionReturnsOk) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());

    auto rollback = adapter.rollback_transaction(begin.value.value());
    EXPECT_TRUE(rollback.is_ok());
}

TEST_F(ThemisDBTransactionTest, BeginTransactionWhenDisconnectedReturnsError) {
    adapter.disconnect();
    auto result = adapter.begin_transaction();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Capability and system information
// ---------------------------------------------------------------------------

class ThemisDBCapabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;
};

TEST_F(ThemisDBCapabilityTest, HasRelationalCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
}

TEST_F(ThemisDBCapabilityTest, HasVectorSearchCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::VECTOR_SEARCH));
}

TEST_F(ThemisDBCapabilityTest, HasGraphTraversalCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
}

TEST_F(ThemisDBCapabilityTest, HasDocumentStoreCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::DOCUMENT_STORE));
}

TEST_F(ThemisDBCapabilityTest, HasTransactionCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::TRANSACTIONS));
}

TEST_F(ThemisDBCapabilityTest, GetCapabilitiesReturnsNonEmpty) {
    auto caps = adapter.get_capabilities();
    EXPECT_FALSE(caps.empty());
}

TEST_F(ThemisDBCapabilityTest, GetSystemInfoReturnsOk) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().system_name.empty());
    EXPECT_FALSE(result.value.value().version.empty());
}

TEST_F(ThemisDBCapabilityTest, GetMetricsReturnsOk) {
    auto result = adapter.get_metrics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Performance / overhead benchmarks
//
// All tests run against the in-process simulation layer (no live ThemisDB
// server required). Limits are intentionally generous to accommodate slow
// debug/CI builds while still guarding against major regressions.
// ---------------------------------------------------------------------------

class ThemisDBPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("themisdb://localhost:7777");
    }

    ThemisDBAdapter adapter;

    static constexpr const char* kDocCol = "perf_docs";
    static constexpr const char* kVecCol = "perf_vecs";
    static constexpr const char* kRowTbl = "perf_rows";

    static constexpr size_t kDocCount   = 1000;
    static constexpr size_t kVecDim     = 32;
    static constexpr size_t kVecCount   = 200;
    static constexpr size_t kSearchK    = 10;
    static constexpr size_t kSearchRuns = 20;
    static constexpr size_t kRowCount   = 500;
    static constexpr size_t kTxnRuns    = 100;

    // Generous wall-clock limits for the in-process simulation layer.
    static constexpr int64_t kBulkDocMs  = 2000;
    static constexpr int64_t kBulkVecMs  = 2000;
    static constexpr int64_t kSearchMs   = 2000;
    static constexpr int64_t kBulkRowMs  = 2000;
    static constexpr int64_t kTxnMs      = 2000;
};

TEST_F(ThemisDBPerformanceTest, BulkDocumentInsertOverhead) {
    std::vector<Document> docs;
    docs.reserve(kDocCount);
    for (size_t i = 0; i < kDocCount; ++i) {
        Document doc;
        doc.id = "pdoc_" + std::to_string(i);
        doc.fields["idx"]  = Scalar{int64_t(i)};
        doc.fields["data"] = Scalar{std::string(32, 'x')};
        docs.push_back(std::move(doc));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto result = adapter.batch_insert_documents(kDocCol, docs);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), kDocCount);

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkDocMs)
        << "Bulk insert of " << kDocCount << " documents took "
        << elapsed_ms << "ms (limit: " << kBulkDocMs << "ms)";
}

TEST_F(ThemisDBPerformanceTest, BulkVectorInsertOverhead) {
    std::vector<Vector> vecs;
    vecs.reserve(kVecCount);
    for (size_t i = 0; i < kVecCount; ++i) {
        vecs.push_back(make_vector(kVecDim, static_cast<float>(i) / kVecCount));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto result = adapter.batch_insert_vectors(kVecCol, vecs);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), kVecCount);

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkVecMs)
        << "Bulk insert of " << kVecCount << " vectors (" << kVecDim
        << "D) took " << elapsed_ms << "ms (limit: " << kBulkVecMs << "ms)";
}

TEST_F(ThemisDBPerformanceTest, VectorSearchOverhead) {
    for (size_t i = 0; i < kVecCount; ++i) {
        adapter.insert_vector(kVecCol, make_vector(kVecDim, static_cast<float>(i) / kVecCount));
    }

    auto query = make_vector(kVecDim, 0.5f);

    auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < kSearchRuns; ++i) {
        auto result = adapter.search_vectors(kVecCol, query, kSearchK);
        ASSERT_TRUE(result.is_ok());
    }
    auto t1 = std::chrono::steady_clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kSearchMs)
        << kSearchRuns << " vector searches (k=" << kSearchK << ") over "
        << kVecCount << " vectors took " << elapsed_ms
        << "ms (limit: " << kSearchMs << "ms)";
}

TEST_F(ThemisDBPerformanceTest, BulkRelationalInsertOverhead) {
    std::vector<RelationalRow> rows;
    rows.reserve(kRowCount);
    for (size_t i = 0; i < kRowCount; ++i) {
        RelationalRow row;
        row.columns["idx"]  = Scalar{int64_t(i)};
        row.columns["data"] = Scalar{std::string(16, 'y')};
        rows.push_back(std::move(row));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto result = adapter.batch_insert(kRowTbl, rows);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), kRowCount);

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkRowMs)
        << "Bulk insert of " << kRowCount << " rows took "
        << elapsed_ms << "ms (limit: " << kBulkRowMs << "ms)";
}

TEST_F(ThemisDBPerformanceTest, TransactionBeginCommitOverhead) {
    auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < kTxnRuns; ++i) {
        auto begin = adapter.begin_transaction();
        ASSERT_TRUE(begin.is_ok());
        auto commit = adapter.commit_transaction(begin.value.value());
        ASSERT_TRUE(commit.is_ok());
    }
    auto t1 = std::chrono::steady_clock::now();

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kTxnMs)
        << kTxnRuns << " begin+commit cycles took "
        << elapsed_ms << "ms (limit: " << kTxnMs << "ms)";
}
