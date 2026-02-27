/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_qdrant_adapter.cpp                            ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-27                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     580                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_qdrant_adapter.cpp
 * @brief Unit tests for QdrantAdapter (CHIMERA Suite)
 *
 * @details Tests cover connection management, vector search (primary
 *          capability), document/payload CRUD, capability reporting, and
 *          NOT_IMPLEMENTED paths for unsupported operations (relational,
 *          graph, transactions).
 *
 * All tests run without a live Qdrant server – the adapter operates in
 * its in-process simulation mode.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/qdrant_adapter.hpp"

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

// ---------------------------------------------------------------------------
// Connection tests
// ---------------------------------------------------------------------------

class QdrantConnectionTest : public ::testing::Test {
protected:
    QdrantAdapter adapter;
};

TEST_F(QdrantConnectionTest, InitiallyDisconnected) {
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(QdrantConnectionTest, ConnectWithHttpUri) {
    auto result = adapter.connect("http://localhost:6333");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(QdrantConnectionTest, ConnectWithHttpsUri) {
    auto result = adapter.connect("https://my-cluster.qdrant.io");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(QdrantConnectionTest, ConnectWithEmptyStringReturnsError) {
    auto result = adapter.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(QdrantConnectionTest, ConnectWithInvalidSchemeReturnsError) {
    auto result = adapter.connect("mongodb://localhost:27017/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(QdrantConnectionTest, ConnectWithPostgresSchemeReturnsError) {
    auto result = adapter.connect("postgresql://localhost:5432/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(QdrantConnectionTest, Disconnect) {
    adapter.connect("http://localhost:6333");
    EXPECT_TRUE(adapter.is_connected());

    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(QdrantConnectionTest, ConnectWithApiKey) {
    std::map<std::string, std::string> options;
    options["api_key"] = "my-secret-key";
    auto result = adapter.connect("https://my-cluster.qdrant.io", options);
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(QdrantConnectionTest, ConnectWithPortInUri) {
    auto result = adapter.connect("http://localhost:6333");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

// ---------------------------------------------------------------------------
// Vector adapter tests (primary capability)
// ---------------------------------------------------------------------------

class QdrantVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:6333");
    }

    QdrantAdapter adapter;
    static constexpr const char* kCollection = "embeddings";
};

TEST_F(QdrantVectorTest, InsertAndSearchVectors) {
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

TEST_F(QdrantVectorTest, SearchReturnsSortedByDescendingScore) {
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

TEST_F(QdrantVectorTest, InsertVectorWithPayload) {
    Vector v;
    v.data = {0.5f, 0.5f};
    v.metadata["category"] = Scalar{std::string{"article"}};
    v.metadata["language"] = Scalar{std::string{"en"}};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(QdrantVectorTest, SearchWithPayloadFilter) {
    Vector v1, v2;
    v1.data = {1.0f, 0.0f};
    v1.metadata["category"] = Scalar{std::string{"article"}};
    v2.data = {1.0f, 0.0f};
    v2.metadata["category"] = Scalar{std::string{"image"}};

    adapter.insert_vector(kCollection, v1);
    adapter.insert_vector(kCollection, v2);

    Vector query;
    query.data = {1.0f, 0.0f};

    std::map<std::string, Scalar> filter;
    filter["category"] = Scalar{std::string{"article"}};

    auto result = adapter.search_vectors(kCollection, query, 10, filter);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 1u);
}

TEST_F(QdrantVectorTest, InsertEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.insert_vector(kCollection, empty);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(QdrantVectorTest, SearchEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.search_vectors(kCollection, empty, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(QdrantVectorTest, SearchEmptyCollectionReturnsEmpty) {
    Vector query;
    query.data = {1.0f, 0.0f};

    auto result = adapter.search_vectors("nonexistent_collection", query, 5);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(QdrantVectorTest, BatchInsertVectors) {
    std::vector<Vector> vecs(3);
    vecs[0].data = {1.0f, 0.0f};
    vecs[1].data = {0.0f, 1.0f};
    vecs[2].data = {0.5f, 0.5f};

    auto result = adapter.batch_insert_vectors(kCollection, vecs);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 3u);
}

TEST_F(QdrantVectorTest, CreateIndexSucceeds) {
    auto result = adapter.create_index(kCollection, 768);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(QdrantVectorTest, CreateIndexWithHnswParamsSucceeds) {
    std::map<std::string, Scalar> params;
    params["index_type"]      = Scalar{std::string{"hnsw"}};
    params["m"]               = Scalar{int64_t{16}};
    params["ef_construction"] = Scalar{int64_t{128}};
    params["distance"]        = Scalar{std::string{"Dot"}};

    auto result = adapter.create_index(kCollection, 768, params);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(QdrantVectorTest, VectorIdGeneratedIsNotEmpty) {
    Vector v;
    v.data = {1.0f, 2.0f, 3.0f};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("qdrant_point_"), std::string::npos);
}

TEST_F(QdrantVectorTest, MultipleInsertIdsAreUnique) {
    Vector v;
    v.data = {1.0f, 0.0f};

    auto r1 = adapter.insert_vector(kCollection, v);
    auto r2 = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(r1.is_ok());
    ASSERT_TRUE(r2.is_ok());
    EXPECT_NE(r1.value.value(), r2.value.value());
}

TEST_F(QdrantVectorTest, OperationsWhileDisconnectedReturnError) {
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
// Document adapter tests (Qdrant payload store)
// ---------------------------------------------------------------------------

class QdrantDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:6333");
    }

    QdrantAdapter adapter;
    static constexpr const char* kCollection = "articles";
};

TEST_F(QdrantDocumentTest, InsertAndRetrieveDocument) {
    auto doc = make_doc("doc1", "Alice", 42);
    auto insert_result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(insert_result.is_ok());
    EXPECT_EQ(insert_result.value.value(), "doc1");

    auto find_result = adapter.find_documents(kCollection, {});
    ASSERT_TRUE(find_result.is_ok());
    ASSERT_EQ(find_result.value.value().size(), 1u);
    EXPECT_EQ(find_result.value.value()[0].id, "doc1");
}

TEST_F(QdrantDocumentTest, InsertGeneratesIdWhenEmpty) {
    Document doc;
    doc.fields["title"] = Scalar{std::string{"Auto-ID Doc"}};
    auto result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("qdrant_point_"), std::string::npos);
}

TEST_F(QdrantDocumentTest, InsertDuplicateIdReturnsError) {
    auto doc = make_doc("dup1", "Bob", 10);
    adapter.insert_document(kCollection, doc);

    auto dup_result = adapter.insert_document(kCollection, doc);
    EXPECT_TRUE(dup_result.is_err());
    EXPECT_EQ(dup_result.error_code, ErrorCode::ALREADY_EXISTS);
}

TEST_F(QdrantDocumentTest, BatchInsertDocuments) {
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

TEST_F(QdrantDocumentTest, FindWithFilter) {
    adapter.insert_document(kCollection, make_doc("f1", "Alice", 10));
    adapter.insert_document(kCollection, make_doc("f2", "Bob",   20));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Alice"}};

    auto result = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "f1");
}

TEST_F(QdrantDocumentTest, FindByIdFilter) {
    adapter.insert_document(kCollection, make_doc("id1", "Carol", 99));
    adapter.insert_document(kCollection, make_doc("id2", "Dave",  50));

    std::map<std::string, Scalar> filter;
    filter["id"] = Scalar{std::string{"id1"}};

    auto result = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "id1");
}

TEST_F(QdrantDocumentTest, FindNonExistentCollectionReturnsEmpty) {
    auto result = adapter.find_documents("nonexistent_collection", {});
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(QdrantDocumentTest, FindRespectsLimit) {
    for (int i = 0; i < 10; ++i) {
        adapter.insert_document(kCollection,
            make_doc("lim" + std::to_string(i), "X", static_cast<int64_t>(i)));
    }

    auto result = adapter.find_documents(kCollection, {}, 3);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 3u);
}

TEST_F(QdrantDocumentTest, UpdateMatchingDocuments) {
    adapter.insert_document(kCollection, make_doc("u1", "Before", 1));
    adapter.insert_document(kCollection, make_doc("u2", "Before", 2));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Before"}};

    std::map<std::string, Scalar> updates;
    updates["name"] = Scalar{std::string{"After"}};

    auto update_result = adapter.update_documents(kCollection, filter, updates);
    ASSERT_TRUE(update_result.is_ok());
    EXPECT_EQ(update_result.value.value(), 2u);

    auto find_result = adapter.find_documents(kCollection, updates);
    ASSERT_TRUE(find_result.is_ok());
    EXPECT_EQ(find_result.value.value().size(), 2u);
}

TEST_F(QdrantDocumentTest, UpdateWithEmptyUpdatesReturnsError) {
    adapter.insert_document(kCollection, make_doc("ue1", "Test", 1));

    auto result = adapter.update_documents(kCollection, {}, {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(QdrantDocumentTest, UpdateNonExistentCollectionReturnsZero) {
    std::map<std::string, Scalar> updates;
    updates["field"] = Scalar{std::string{"value"}};

    auto result = adapter.update_documents("no_such_collection", {}, updates);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 0u);
}

TEST_F(QdrantDocumentTest, OperationsWhileDisconnectedReturnError) {
    adapter.disconnect();

    auto r1 = adapter.insert_document(kCollection, make_doc("x", "y", 1));
    EXPECT_EQ(r1.error_code, ErrorCode::CONNECTION_ERROR);

    auto r2 = adapter.find_documents(kCollection, {});
    EXPECT_EQ(r2.error_code, ErrorCode::CONNECTION_ERROR);

    auto r3 = adapter.batch_insert_documents(kCollection,
                                             {make_doc("x", "y", 1)});
    EXPECT_EQ(r3.error_code, ErrorCode::CONNECTION_ERROR);

    std::map<std::string, Scalar> updates;
    updates["f"] = Scalar{std::string{"v"}};
    auto r4 = adapter.update_documents(kCollection, {}, updates);
    EXPECT_EQ(r4.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Graph adapter tests – all NOT_IMPLEMENTED
// ---------------------------------------------------------------------------

class QdrantGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:6333");
    }

    QdrantAdapter adapter;
};

TEST_F(QdrantGraphTest, InsertNodeReturnsNotImplemented) {
    GraphNode node;
    node.id    = "node1";
    node.label = "Entity";

    auto result = adapter.insert_node(node);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantGraphTest, InsertEdgeReturnsNotImplemented) {
    GraphEdge edge;
    edge.id        = "e1";
    edge.source_id = "n1";
    edge.target_id = "n2";
    edge.label     = "LINKS_TO";

    auto result = adapter.insert_edge(edge);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantGraphTest, ShortestPathReturnsNotImplemented) {
    auto result = adapter.shortest_path("n1", "n2", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantGraphTest, TraverseReturnsNotImplemented) {
    auto result = adapter.traverse("n1", 3);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantGraphTest, ExecuteGraphQueryReturnsNotImplemented) {
    auto result = adapter.execute_graph_query("MATCH (n) RETURN n");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// Relational adapter tests – all NOT_IMPLEMENTED
// ---------------------------------------------------------------------------

class QdrantRelationalTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:6333");
    }

    QdrantAdapter adapter;
};

TEST_F(QdrantRelationalTest, ExecuteQueryReturnsNotImplemented) {
    auto result = adapter.execute_query("SELECT * FROM table");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantRelationalTest, InsertRowReturnsNotImplemented) {
    RelationalRow row;
    row.columns["col"] = Scalar{std::string{"val"}};
    auto result = adapter.insert_row("table", row);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantRelationalTest, BatchInsertReturnsNotImplemented) {
    RelationalRow row;
    row.columns["col"] = Scalar{std::string{"val"}};
    auto result = adapter.batch_insert("table", {row});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantRelationalTest, GetQueryStatisticsReturnsOk) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Transaction adapter tests – all NOT_IMPLEMENTED
// ---------------------------------------------------------------------------

class QdrantTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:6333");
    }

    QdrantAdapter adapter;
};

TEST_F(QdrantTransactionTest, BeginTransactionReturnsNotImplemented) {
    auto result = adapter.begin_transaction();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantTransactionTest, CommitTransactionReturnsNotImplemented) {
    auto result = adapter.commit_transaction("txn_001");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantTransactionTest, RollbackTransactionReturnsNotImplemented) {
    auto result = adapter.rollback_transaction("txn_001");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// System info and capability tests
// ---------------------------------------------------------------------------

class QdrantSystemInfoTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:6333");
    }

    QdrantAdapter adapter;
};

TEST_F(QdrantSystemInfoTest, GetSystemInfoReturnsQdrant) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().system_name, "Qdrant");
    EXPECT_FALSE(result.value.value().version.empty());
}

TEST_F(QdrantSystemInfoTest, GetMetricsReturnsOk) {
    auto result = adapter.get_metrics();
    EXPECT_TRUE(result.is_ok());
}

TEST_F(QdrantSystemInfoTest, HasVectorSearchCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::VECTOR_SEARCH));
}

TEST_F(QdrantSystemInfoTest, HasDocumentStoreCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::DOCUMENT_STORE));
}

TEST_F(QdrantSystemInfoTest, HasBatchOperationsCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::BATCH_OPERATIONS));
}

TEST_F(QdrantSystemInfoTest, HasSecondaryIndexesCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::SECONDARY_INDEXES));
}

TEST_F(QdrantSystemInfoTest, DoesNotHaveTransactionsCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::TRANSACTIONS));
}

TEST_F(QdrantSystemInfoTest, DoesNotHaveRelationalQueriesCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
}

TEST_F(QdrantSystemInfoTest, DoesNotHaveGraphTraversalCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
}

TEST_F(QdrantSystemInfoTest, GetCapabilitiesContainsExpectedSet) {
    auto caps = adapter.get_capabilities();
    EXPECT_FALSE(caps.empty());

    auto has = [&](Capability c) {
        return std::find(caps.begin(), caps.end(), c) != caps.end();
    };

    EXPECT_TRUE(has(Capability::VECTOR_SEARCH));
    EXPECT_TRUE(has(Capability::DOCUMENT_STORE));
    EXPECT_TRUE(has(Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(has(Capability::SECONDARY_INDEXES));
}
