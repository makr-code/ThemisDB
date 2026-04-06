/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_qdrant_adapter.cpp                            ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:22:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     861                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • df18228ceb  2026-02-28  Add QdrantFactoryTest, QdrantSecurityTest, and QdrantPerf... ║
    • e481c0e03a  2026-02-27  feat(chimera): Add Qdrant native vector database adapter ║
    • f16d5f90b5  2026-02-27  fix(chimera): audit fixes – security tests, performance b... ║
    • 3f0220a435  2026-02-26  feat(chimera): implement Weaviate native vector database ... ║
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

// ---------------------------------------------------------------------------
// Factory integration tests
// ---------------------------------------------------------------------------

class QdrantFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // The QdrantAdapter auto-registers itself when the translation unit is
        // linked.  Calling register_adapter() again returns false (indicating
        // the name was already taken) without overwriting the existing entry,
        // so this call is safe and ensures the factory is populated regardless
        // of static-init ordering.
        AdapterFactory::register_adapter(
            "Qdrant",
            []() { return std::make_unique<QdrantAdapter>(); });
    }

    static constexpr const char* kConnString = "http://localhost:6333";
};

TEST_F(QdrantFactoryTest, RegisterAndCreate) {
    EXPECT_TRUE(AdapterFactory::is_supported("Qdrant"));

    auto adapter = AdapterFactory::create("Qdrant");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(adapter->is_connected());
}

TEST_F(QdrantFactoryTest, ConnectViaFactory) {
    auto adapter = AdapterFactory::create("Qdrant");
    ASSERT_NE(adapter, nullptr);

    auto result = adapter->connect(kConnString);
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter->is_connected());
}

TEST_F(QdrantFactoryTest, CapabilitiesViaFactory) {
    auto adapter = AdapterFactory::create("Qdrant");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(adapter->has_capability(Capability::VECTOR_SEARCH));
    EXPECT_TRUE(adapter->has_capability(Capability::DOCUMENT_STORE));
    EXPECT_FALSE(adapter->has_capability(Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(adapter->has_capability(Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(adapter->has_capability(Capability::TRANSACTIONS));
}

// ---------------------------------------------------------------------------
// Security audit: API key / credential handling
// ---------------------------------------------------------------------------

class QdrantSecurityTest : public ::testing::Test {
protected:
    QdrantAdapter adapter;
};

TEST_F(QdrantSecurityTest, ApiKeyNotExposedInSystemInfo) {
    // Connect with an API key passed via the options map.
    adapter.connect("https://my-cluster.qdrant.io",
                    {{"api_key", "s3cr3t-api-key-12345"}});

    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    const auto& info = result.value.value();

    // The raw API key must not appear in any build_info or configuration field.
    for (const auto& kv : info.build_info) {
        EXPECT_EQ(kv.second.find("s3cr3t"), std::string::npos)
            << "API key leaked in build_info[" << kv.first << "]";
    }
    for (const auto& kv : info.configuration) {
        if (std::holds_alternative<std::string>(kv.second)) {
            EXPECT_EQ(std::get<std::string>(kv.second).find("s3cr3t"),
                      std::string::npos)
                << "API key leaked in configuration[" << kv.first << "]";
        }
    }
}

TEST_F(QdrantSecurityTest, ConnectWithoutApiKeySucceeds) {
    // No credentials in URL or options – typical local-instance usage.
    auto result = adapter.connect("http://localhost:6333");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(QdrantSecurityTest, ConnectWithApiKeyInOptionsSucceeds) {
    auto result = adapter.connect("https://my-cluster.qdrant.io",
                                  {{"api_key", "my-secret-key"}});
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(QdrantSecurityTest, SystemInfoEndpointDoesNotLeakKey) {
    adapter.connect("https://cluster.qdrant.io",
                    {{"api_key", "super-secret-qdrant-key"}});

    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());

    // Endpoint field should contain the base URL but not the key.
    auto ep_it = result.value.value().configuration.find("endpoint");
    if (ep_it != result.value.value().configuration.end() &&
        std::holds_alternative<std::string>(ep_it->second)) {
        const auto& ep = std::get<std::string>(ep_it->second);
        EXPECT_EQ(ep.find("super-secret"), std::string::npos)
            << "API key leaked via 'endpoint' configuration field";
    }
}

// ---------------------------------------------------------------------------
// Performance / overhead benchmarks
//
// All tests run against the in-process simulation mode (no live Qdrant
// server required). The limits are intentionally generous to accommodate
// slow debug/CI builds while still guarding against major regressions.
// ---------------------------------------------------------------------------

class QdrantPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:6333");
    }

    QdrantAdapter adapter;
    static constexpr const char* kVecCol = "perf_vecs";
    static constexpr const char* kDocCol = "perf_docs";

    static constexpr size_t kDocCount   = 1000;
    static constexpr size_t kVecDim     = 32;
    static constexpr size_t kVecCount   = 200;
    static constexpr size_t kSearchK    = 10;
    static constexpr size_t kSearchRuns = 20;

    // Generous wall-clock limits for the in-process simulation layer.
    static constexpr int64_t kBulkDocMs = 2000;
    static constexpr int64_t kBulkVecMs = 2000;
    static constexpr int64_t kSearchMs  = 2000;
};

TEST_F(QdrantPerformanceTest, BulkDocumentInsertOverhead) {
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

TEST_F(QdrantPerformanceTest, BulkVectorInsertOverhead) {
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

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, kBulkVecMs)
        << "Bulk insert of " << kVecCount << " vectors (" << kVecDim
        << "D) took " << elapsed_ms << "ms (limit: " << kBulkVecMs << "ms)";
}

TEST_F(QdrantPerformanceTest, VectorSearchOverhead) {
    for (size_t i = 0; i < kVecCount; ++i) {
        Vector v;
        v.data.resize(kVecDim, static_cast<float>(i) / kVecCount);
        adapter.insert_vector(kVecCol, v);
    }

    Vector query;
    query.data.resize(kVecDim, 0.5f);

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
