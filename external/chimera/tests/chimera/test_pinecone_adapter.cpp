/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_pinecone_adapter.cpp                          ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:22:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     678                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 31a9305f66  2026-02-28  feat(chimera): Add Pinecone managed vector search adapter ║
    • e481c0e03a  2026-02-27  feat(chimera): Add Qdrant native vector database adapter ║
    • f16d5f90b5  2026-02-27  fix(chimera): audit fixes – security tests, performance b... ║
    • 3f0220a435  2026-02-26  feat(chimera): implement Weaviate native vector database ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_pinecone_adapter.cpp
 * @brief Unit tests for PineconeAdapter (CHIMERA Suite)
 *
 * @details Tests cover connection management, vector search (primary
 *          capability), document/metadata CRUD, capability reporting, and
 *          NOT_IMPLEMENTED paths for unsupported operations (relational,
 *          graph, transactions).
 *
 * All tests run without a live Pinecone service – the adapter operates in
 * its in-process simulation mode.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/pinecone_adapter.hpp"

#include <chrono>
#include <cmath>

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

class PineconeConnectionTest : public ::testing::Test {
protected:
    PineconeAdapter adapter;
};

TEST_F(PineconeConnectionTest, InitiallyDisconnected) {
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PineconeConnectionTest, ConnectWithHttpsUri) {
    auto result = adapter.connect("https://my-index-abc123.svc.pinecone.io");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(PineconeConnectionTest, ConnectWithEmptyStringReturnsError) {
    auto result = adapter.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PineconeConnectionTest, ConnectWithHttpSchemeReturnsError) {
    auto result = adapter.connect("http://my-index.svc.pinecone.io");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PineconeConnectionTest, ConnectWithInvalidSchemeReturnsError) {
    auto result = adapter.connect("mongodb://localhost:27017/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PineconeConnectionTest, Disconnect) {
    adapter.connect("https://my-index.svc.pinecone.io");
    EXPECT_TRUE(adapter.is_connected());

    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(PineconeConnectionTest, ConnectWithApiKey) {
    std::map<std::string, std::string> options;
    options["api_key"] = "test-pinecone-api-key";
    auto result = adapter.connect("https://my-index.svc.pinecone.io", options);
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(PineconeConnectionTest, SystemInfoDoesNotLeakApiKey) {
    std::map<std::string, std::string> options;
    options["api_key"] = "super-secret-key";
    adapter.connect("https://my-index.svc.pinecone.io", options);

    auto info_result = adapter.get_system_info();
    ASSERT_TRUE(info_result.is_ok());
    const auto& info = info_result.value.value();

    // The API key must not appear in any field of system info
    for (const auto& kv : info.build_info) {
        EXPECT_EQ(kv.second.find("super-secret-key"), std::string::npos)
            << "API key leaked in build_info[" << kv.first << "]";
    }
    for (const auto& kv : info.configuration) {
        if (std::holds_alternative<std::string>(kv.second)) {
            EXPECT_EQ(std::get<std::string>(kv.second).find("super-secret-key"),
                      std::string::npos)
                << "API key leaked in configuration[" << kv.first << "]";
        }
    }
}

// ---------------------------------------------------------------------------
// Vector adapter tests (primary capability)
// ---------------------------------------------------------------------------

class PineconeVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("https://my-index.svc.pinecone.io");
    }

    PineconeAdapter adapter;
    static constexpr const char* kCollection = "embeddings";
};

TEST_F(PineconeVectorTest, InsertAndSearchVectors) {
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

TEST_F(PineconeVectorTest, SearchReturnsSortedByDescendingScore) {
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

TEST_F(PineconeVectorTest, CosineScoreNormalisedBetweenZeroAndOne) {
    Vector v1;
    v1.data = {3.0f, 4.0f};  // unit vector after normalization: (0.6, 0.8)

    adapter.insert_vector(kCollection, v1);

    Vector query;
    query.data = {3.0f, 4.0f};  // identical direction

    auto result = adapter.search_vectors(kCollection, query, 1);
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.value.value().size(), 1u);
    // Cosine similarity of identical vectors must be ~1.0
    EXPECT_NEAR(result.value.value()[0].second, 1.0, 1e-6);
}

TEST_F(PineconeVectorTest, InsertVectorWithMetadata) {
    Vector v;
    v.data = {0.5f, 0.5f};
    v.metadata["category"] = Scalar{std::string{"article"}};
    v.metadata["language"] = Scalar{std::string{"en"}};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(PineconeVectorTest, SearchWithMetadataFilter) {
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

TEST_F(PineconeVectorTest, InsertEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.insert_vector(kCollection, empty);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PineconeVectorTest, SearchEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.search_vectors(kCollection, empty, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PineconeVectorTest, SearchEmptyNamespaceReturnsEmpty) {
    Vector query;
    query.data = {1.0f, 0.0f};

    auto result = adapter.search_vectors("nonexistent_namespace", query, 5);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(PineconeVectorTest, BatchInsertVectors) {
    std::vector<Vector> vecs(3);
    vecs[0].data = {1.0f, 0.0f};
    vecs[1].data = {0.0f, 1.0f};
    vecs[2].data = {0.5f, 0.5f};

    auto result = adapter.batch_insert_vectors(kCollection, vecs);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 3u);
}

TEST_F(PineconeVectorTest, CreateIndexSucceeds) {
    auto result = adapter.create_index(kCollection, 768);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(PineconeVectorTest, CreateIndexWithParamsSucceeds) {
    std::map<std::string, Scalar> params;
    params["metric"]     = Scalar{std::string{"cosine"}};
    params["cloud"]      = Scalar{std::string{"aws"}};
    params["region"]     = Scalar{std::string{"us-east-1"}};

    auto result = adapter.create_index(kCollection, 1536, params);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(PineconeVectorTest, VectorIdGeneratedIsNotEmpty) {
    Vector v;
    v.data = {1.0f, 2.0f, 3.0f};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("pinecone_vec_"), std::string::npos);
}

TEST_F(PineconeVectorTest, MultipleInsertIdsAreUnique) {
    Vector v;
    v.data = {1.0f, 0.0f};

    auto r1 = adapter.insert_vector(kCollection, v);
    auto r2 = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(r1.is_ok());
    ASSERT_TRUE(r2.is_ok());
    EXPECT_NE(r1.value.value(), r2.value.value());
}

TEST_F(PineconeVectorTest, OperationsWhileDisconnectedReturnError) {
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
// Document adapter tests (Pinecone metadata store)
// ---------------------------------------------------------------------------

class PineconeDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("https://my-index.svc.pinecone.io");
    }

    PineconeAdapter adapter;
    static constexpr const char* kCollection = "articles";
};

TEST_F(PineconeDocumentTest, InsertAndRetrieveDocument) {
    auto doc = make_doc("doc1", "Alice", 42);
    auto insert_result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(insert_result.is_ok());
    EXPECT_EQ(insert_result.value.value(), "doc1");

    auto find_result = adapter.find_documents(kCollection, {});
    ASSERT_TRUE(find_result.is_ok());
    ASSERT_EQ(find_result.value.value().size(), 1u);
    EXPECT_EQ(find_result.value.value()[0].id, "doc1");
}

TEST_F(PineconeDocumentTest, InsertGeneratesIdWhenEmpty) {
    Document doc;
    doc.fields["title"] = Scalar{std::string{"Auto-ID Doc"}};
    auto result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("pinecone_vec_"), std::string::npos);
}

TEST_F(PineconeDocumentTest, InsertDuplicateIdReturnsError) {
    auto doc = make_doc("dup1", "Bob", 10);
    adapter.insert_document(kCollection, doc);

    auto dup_result = adapter.insert_document(kCollection, doc);
    EXPECT_TRUE(dup_result.is_err());
    EXPECT_EQ(dup_result.error_code, ErrorCode::ALREADY_EXISTS);
}

TEST_F(PineconeDocumentTest, BatchInsertDocuments) {
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

TEST_F(PineconeDocumentTest, FindWithFilter) {
    adapter.insert_document(kCollection, make_doc("f1", "Alice", 10));
    adapter.insert_document(kCollection, make_doc("f2", "Bob",   20));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Alice"}};

    auto result = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "f1");
}

TEST_F(PineconeDocumentTest, FindByIdFilter) {
    adapter.insert_document(kCollection, make_doc("id1", "Carol", 99));
    adapter.insert_document(kCollection, make_doc("id2", "Dave",  50));

    std::map<std::string, Scalar> filter;
    filter["id"] = Scalar{std::string{"id1"}};

    auto result = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "id1");
}

TEST_F(PineconeDocumentTest, FindNonExistentNamespaceReturnsEmpty) {
    auto result = adapter.find_documents("nonexistent_namespace", {});
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(PineconeDocumentTest, FindRespectsLimit) {
    for (int i = 0; i < 10; ++i) {
        adapter.insert_document(kCollection,
            make_doc("lim" + std::to_string(i), "X", static_cast<int64_t>(i)));
    }

    auto result = adapter.find_documents(kCollection, {}, 3);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 3u);
}

TEST_F(PineconeDocumentTest, UpdateMatchingDocuments) {
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

TEST_F(PineconeDocumentTest, UpdateWithEmptyUpdatesReturnsError) {
    adapter.insert_document(kCollection, make_doc("ue1", "Test", 1));

    auto result = adapter.update_documents(kCollection, {}, {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(PineconeDocumentTest, UpdateNonExistentNamespaceReturnsZero) {
    std::map<std::string, Scalar> updates;
    updates["field"] = Scalar{std::string{"value"}};

    auto result = adapter.update_documents("no_such_namespace", {}, updates);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 0u);
}

TEST_F(PineconeDocumentTest, OperationsWhileDisconnectedReturnError) {
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

class PineconeGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("https://my-index.svc.pinecone.io");
    }

    PineconeAdapter adapter;
};

TEST_F(PineconeGraphTest, InsertNodeReturnsNotImplemented) {
    GraphNode node;
    node.id    = "node1";
    node.label = "Entity";

    auto result = adapter.insert_node(node);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeGraphTest, InsertEdgeReturnsNotImplemented) {
    GraphEdge edge;
    edge.id        = "e1";
    edge.source_id = "n1";
    edge.target_id = "n2";
    edge.label     = "LINKS_TO";

    auto result = adapter.insert_edge(edge);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeGraphTest, ShortestPathReturnsNotImplemented) {
    auto result = adapter.shortest_path("n1", "n2", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeGraphTest, TraverseReturnsNotImplemented) {
    auto result = adapter.traverse("n1", 3);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeGraphTest, ExecuteGraphQueryReturnsNotImplemented) {
    auto result = adapter.execute_graph_query("MATCH (n) RETURN n");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// Relational adapter tests – all NOT_IMPLEMENTED
// ---------------------------------------------------------------------------

class PineconeRelationalTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("https://my-index.svc.pinecone.io");
    }

    PineconeAdapter adapter;
};

TEST_F(PineconeRelationalTest, ExecuteQueryReturnsNotImplemented) {
    auto result = adapter.execute_query("SELECT * FROM table");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeRelationalTest, InsertRowReturnsNotImplemented) {
    RelationalRow row;
    row.columns["col"] = Scalar{std::string{"val"}};
    auto result = adapter.insert_row("table", row);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeRelationalTest, BatchInsertReturnsNotImplemented) {
    RelationalRow row;
    row.columns["col"] = Scalar{std::string{"val"}};
    auto result = adapter.batch_insert("table", {row});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeRelationalTest, GetQueryStatisticsReturnsOk) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Transaction adapter tests – all NOT_IMPLEMENTED
// ---------------------------------------------------------------------------

class PineconeTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("https://my-index.svc.pinecone.io");
    }

    PineconeAdapter adapter;
};

TEST_F(PineconeTransactionTest, BeginTransactionReturnsNotImplemented) {
    auto result = adapter.begin_transaction();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeTransactionTest, CommitTransactionReturnsNotImplemented) {
    auto result = adapter.commit_transaction("txn_001");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(PineconeTransactionTest, RollbackTransactionReturnsNotImplemented) {
    auto result = adapter.rollback_transaction("txn_001");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// System info and capability tests
// ---------------------------------------------------------------------------

class PineconeSystemInfoTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("https://my-index.svc.pinecone.io");
    }

    PineconeAdapter adapter;
};

TEST_F(PineconeSystemInfoTest, GetSystemInfoReturnsPinecone) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().system_name, "Pinecone");
    EXPECT_FALSE(result.value.value().version.empty());
}

TEST_F(PineconeSystemInfoTest, GetMetricsReturnsOk) {
    auto result = adapter.get_metrics();
    EXPECT_TRUE(result.is_ok());
}

TEST_F(PineconeSystemInfoTest, HasVectorSearchCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::VECTOR_SEARCH));
}

TEST_F(PineconeSystemInfoTest, HasBatchOperationsCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::BATCH_OPERATIONS));
}

TEST_F(PineconeSystemInfoTest, HasSecondaryIndexesCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::SECONDARY_INDEXES));
}

TEST_F(PineconeSystemInfoTest, DoesNotHaveTransactionsCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::TRANSACTIONS));
}

TEST_F(PineconeSystemInfoTest, DoesNotHaveRelationalQueriesCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
}

TEST_F(PineconeSystemInfoTest, DoesNotHaveGraphTraversalCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
}

TEST_F(PineconeSystemInfoTest, DoesNotHaveDocumentStoreCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::DOCUMENT_STORE));
}

TEST_F(PineconeSystemInfoTest, GetCapabilitiesContainsExpectedSet) {
    auto caps = adapter.get_capabilities();
    EXPECT_FALSE(caps.empty());

    auto has = [&](Capability c) {
        return std::find(caps.begin(), caps.end(), c) != caps.end();
    };

    EXPECT_TRUE(has(Capability::VECTOR_SEARCH));
    EXPECT_TRUE(has(Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(has(Capability::SECONDARY_INDEXES));
    EXPECT_FALSE(has(Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(has(Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(has(Capability::TRANSACTIONS));
}
