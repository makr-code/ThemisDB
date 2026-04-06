/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_elasticsearch_adapter.cpp                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:22:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     785                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5b182fc5c6  2026-02-28  Add Elasticsearch adapter: header, implementation, tests,... ║
    • e481c0e03a  2026-02-27  feat(chimera): Add Qdrant native vector database adapter ║
    • f16d5f90b5  2026-02-27  fix(chimera): audit fixes – security tests, performance b... ║
    • 3f0220a435  2026-02-26  feat(chimera): implement Weaviate native vector database ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_elasticsearch_adapter.cpp
 * @brief Unit tests for ElasticsearchAdapter (CHIMERA Suite)
 *
 * @details Tests cover connection management, vector search (kNN),
 *          full-text search (primary capabilities), document CRUD,
 *          capability reporting, and NOT_IMPLEMENTED paths for unsupported
 *          operations (relational, graph, transactions).
 *
 * All tests run without a live Elasticsearch cluster – the adapter operates
 * in its in-process simulation mode.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/elasticsearch_adapter.hpp"

#include <algorithm>

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

static Document make_text_doc(const std::string& id,
                               const std::string& body) {
    Document doc;
    doc.id = id;
    doc.fields["body"] = Scalar{body};
    return doc;
}

// ---------------------------------------------------------------------------
// Connection tests
// ---------------------------------------------------------------------------

class ElasticsearchConnectionTest : public ::testing::Test {
protected:
    ElasticsearchAdapter adapter;
};

TEST_F(ElasticsearchConnectionTest, InitiallyDisconnected) {
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(ElasticsearchConnectionTest, ConnectWithHttpUri) {
    auto result = adapter.connect("http://localhost:9200");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(ElasticsearchConnectionTest, ConnectWithHttpsUri) {
    auto result = adapter.connect("https://my-cluster.es.io:9243");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(ElasticsearchConnectionTest, ConnectWithEmptyStringReturnsError) {
    auto result = adapter.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(ElasticsearchConnectionTest, ConnectWithInvalidSchemeReturnsError) {
    auto result = adapter.connect("mongodb://localhost:27017/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(ElasticsearchConnectionTest, ConnectWithPostgresSchemeReturnsError) {
    auto result = adapter.connect("postgresql://localhost:5432/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ElasticsearchConnectionTest, Disconnect) {
    adapter.connect("http://localhost:9200");
    EXPECT_TRUE(adapter.is_connected());

    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(ElasticsearchConnectionTest, ConnectWithApiKey) {
    std::map<std::string, std::string> options;
    options["api_key"] = "my-api-key-token";
    auto result = adapter.connect("https://my-cluster.es.io:9243", options);
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(ElasticsearchConnectionTest, ConnectWithPortInUri) {
    auto result = adapter.connect("http://localhost:9200");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

// ---------------------------------------------------------------------------
// Vector adapter tests (kNN dense vector search)
// ---------------------------------------------------------------------------

class ElasticsearchVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:9200");
    }

    ElasticsearchAdapter adapter;
    static constexpr const char* kCollection = "embeddings";
};

TEST_F(ElasticsearchVectorTest, InsertAndSearchVectors) {
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

TEST_F(ElasticsearchVectorTest, SearchReturnsSortedByDescendingScore) {
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

TEST_F(ElasticsearchVectorTest, InsertVectorWithMetadata) {
    Vector v;
    v.data = {0.5f, 0.5f};
    v.metadata["category"] = Scalar{std::string{"article"}};
    v.metadata["language"] = Scalar{std::string{"en"}};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(ElasticsearchVectorTest, SearchWithMetadataFilter) {
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

TEST_F(ElasticsearchVectorTest, InsertEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.insert_vector(kCollection, empty);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ElasticsearchVectorTest, SearchEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.search_vectors(kCollection, empty, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ElasticsearchVectorTest, SearchEmptyCollectionReturnsEmpty) {
    Vector query;
    query.data = {1.0f, 0.0f};

    auto result = adapter.search_vectors("nonexistent_index", query, 5);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(ElasticsearchVectorTest, BatchInsertVectors) {
    std::vector<Vector> vecs(3);
    vecs[0].data = {1.0f, 0.0f};
    vecs[1].data = {0.0f, 1.0f};
    vecs[2].data = {0.5f, 0.5f};

    auto result = adapter.batch_insert_vectors(kCollection, vecs);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 3u);
}

TEST_F(ElasticsearchVectorTest, CreateIndexSucceeds) {
    auto result = adapter.create_index(kCollection, 768);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ElasticsearchVectorTest, CreateIndexWithDenseVectorParamsSucceeds) {
    std::map<std::string, Scalar> params;
    params["index_type"] = Scalar{std::string{"hnsw"}};
    params["similarity"] = Scalar{std::string{"cosine"}};
    params["dims"]       = Scalar{int64_t{128}};

    auto result = adapter.create_index(kCollection, 128, params);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ElasticsearchVectorTest, VectorIdGeneratedIsNotEmpty) {
    Vector v;
    v.data = {1.0f, 2.0f, 3.0f};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("es_doc_"), std::string::npos);
}

TEST_F(ElasticsearchVectorTest, MultipleInsertIdsAreUnique) {
    Vector v;
    v.data = {1.0f, 0.0f};

    auto r1 = adapter.insert_vector(kCollection, v);
    auto r2 = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(r1.is_ok());
    ASSERT_TRUE(r2.is_ok());
    EXPECT_NE(r1.value.value(), r2.value.value());
}

TEST_F(ElasticsearchVectorTest, OperationsWhileDisconnectedReturnError) {
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
// Full-text search tests (primary Elasticsearch capability)
// ---------------------------------------------------------------------------

class ElasticsearchFullTextTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:9200");
    }

    ElasticsearchAdapter adapter;
    static constexpr const char* kIndex = "articles";
};

TEST_F(ElasticsearchFullTextTest, BasicFullTextSearch) {
    adapter.insert_document(kIndex,
        make_text_doc("a1", "Elasticsearch is a distributed search engine"));
    adapter.insert_document(kIndex,
        make_text_doc("a2", "PostgreSQL is a relational database"));
    adapter.insert_document(kIndex,
        make_text_doc("a3", "Elasticsearch supports vector search since 8.0"));

    auto result = adapter.full_text_search(kIndex, "body", "Elasticsearch");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 2u);
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchIsCaseInsensitive) {
    adapter.insert_document(kIndex,
        make_text_doc("b1", "Elasticsearch is Fast"));
    adapter.insert_document(kIndex,
        make_text_doc("b2", "PostgreSQL is reliable"));

    auto result = adapter.full_text_search(kIndex, "body", "elasticsearch");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "b1");
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchMultipleTerms) {
    adapter.insert_document(kIndex,
        make_text_doc("c1", "vector search with Elasticsearch kNN support"));
    adapter.insert_document(kIndex,
        make_text_doc("c2", "Elasticsearch full-text indexing guide"));
    adapter.insert_document(kIndex,
        make_text_doc("c3", "vector database comparison benchmark"));

    // Both terms must appear in the same document
    auto result = adapter.full_text_search(kIndex, "body", "elasticsearch vector");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "c1");
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchAllFields) {
    Document doc;
    doc.id = "d1";
    doc.fields["title"] = Scalar{std::string{"Advanced Elasticsearch Techniques"}};
    doc.fields["tags"]  = Scalar{std::string{"search indexing"}};
    adapter.insert_document(kIndex, doc);

    // Use "_all" to match across all string fields
    auto result = adapter.full_text_search(kIndex, "_all", "advanced indexing");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 1u);
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchEmptyQueryReturnsError) {
    auto result = adapter.full_text_search(kIndex, "body", "");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchNonExistentIndexReturnsEmpty) {
    auto result = adapter.full_text_search("no_such_index", "body", "hello");
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchNoMatchReturnsEmpty) {
    adapter.insert_document(kIndex,
        make_text_doc("e1", "Elasticsearch distributed search"));

    auto result = adapter.full_text_search(kIndex, "body", "mongodb");
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchRespectsLimit) {
    for (int i = 0; i < 20; ++i) {
        adapter.insert_document(kIndex,
            make_text_doc("lim" + std::to_string(i), "Elasticsearch document " + std::to_string(i)));
    }

    auto result = adapter.full_text_search(kIndex, "body", "elasticsearch", 5);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 5u);
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchWhileDisconnectedReturnsError) {
    adapter.disconnect();
    auto result = adapter.full_text_search(kIndex, "body", "hello");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(ElasticsearchFullTextTest, FullTextSearchFieldNotFoundReturnsEmpty) {
    adapter.insert_document(kIndex,
        make_text_doc("f1", "some text here"));

    // Search in a field that does not exist on the document
    auto result = adapter.full_text_search(kIndex, "nonexistent_field", "some");
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

// ---------------------------------------------------------------------------
// Document adapter tests (Elasticsearch index store)
// ---------------------------------------------------------------------------

class ElasticsearchDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:9200");
    }

    ElasticsearchAdapter adapter;
    static constexpr const char* kIndex = "articles";
};

TEST_F(ElasticsearchDocumentTest, InsertAndRetrieveDocument) {
    auto doc = make_doc("doc1", "Alice", 42);
    auto insert_result = adapter.insert_document(kIndex, doc);
    ASSERT_TRUE(insert_result.is_ok());
    EXPECT_EQ(insert_result.value.value(), "doc1");

    auto find_result = adapter.find_documents(kIndex, {});
    ASSERT_TRUE(find_result.is_ok());
    ASSERT_EQ(find_result.value.value().size(), 1u);
    EXPECT_EQ(find_result.value.value()[0].id, "doc1");
}

TEST_F(ElasticsearchDocumentTest, InsertGeneratesIdWhenEmpty) {
    Document doc;
    doc.fields["title"] = Scalar{std::string{"Auto-ID Doc"}};
    auto result = adapter.insert_document(kIndex, doc);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("es_doc_"), std::string::npos);
}

TEST_F(ElasticsearchDocumentTest, InsertDuplicateIdReturnsError) {
    auto doc = make_doc("dup1", "Bob", 10);
    adapter.insert_document(kIndex, doc);

    auto dup_result = adapter.insert_document(kIndex, doc);
    EXPECT_TRUE(dup_result.is_err());
    EXPECT_EQ(dup_result.error_code, ErrorCode::ALREADY_EXISTS);
}

TEST_F(ElasticsearchDocumentTest, BatchInsertDocuments) {
    std::vector<Document> docs;
    for (int i = 0; i < 5; ++i) {
        docs.push_back(make_doc("bdoc" + std::to_string(i),
                                "Name" + std::to_string(i),
                                static_cast<int64_t>(i)));
    }
    auto result = adapter.batch_insert_documents(kIndex, docs);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 5u);
}

TEST_F(ElasticsearchDocumentTest, FindWithFilter) {
    adapter.insert_document(kIndex, make_doc("f1", "Alice", 10));
    adapter.insert_document(kIndex, make_doc("f2", "Bob",   20));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Alice"}};

    auto result = adapter.find_documents(kIndex, filter);
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "f1");
}

TEST_F(ElasticsearchDocumentTest, FindByIdFilter) {
    adapter.insert_document(kIndex, make_doc("id1", "Carol", 99));
    adapter.insert_document(kIndex, make_doc("id2", "Dave",  50));

    std::map<std::string, Scalar> filter;
    filter["id"] = Scalar{std::string{"id1"}};

    auto result = adapter.find_documents(kIndex, filter);
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "id1");
}

TEST_F(ElasticsearchDocumentTest, FindNonExistentIndexReturnsEmpty) {
    auto result = adapter.find_documents("nonexistent_index", {});
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(ElasticsearchDocumentTest, FindRespectsLimit) {
    for (int i = 0; i < 10; ++i) {
        adapter.insert_document(kIndex,
            make_doc("lim" + std::to_string(i), "X", static_cast<int64_t>(i)));
    }

    auto result = adapter.find_documents(kIndex, {}, 3);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 3u);
}

TEST_F(ElasticsearchDocumentTest, UpdateMatchingDocuments) {
    adapter.insert_document(kIndex, make_doc("u1", "Before", 1));
    adapter.insert_document(kIndex, make_doc("u2", "Before", 2));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Before"}};

    std::map<std::string, Scalar> updates;
    updates["name"] = Scalar{std::string{"After"}};

    auto update_result = adapter.update_documents(kIndex, filter, updates);
    ASSERT_TRUE(update_result.is_ok());
    EXPECT_EQ(update_result.value.value(), 2u);

    auto find_result = adapter.find_documents(kIndex, updates);
    ASSERT_TRUE(find_result.is_ok());
    EXPECT_EQ(find_result.value.value().size(), 2u);
}

TEST_F(ElasticsearchDocumentTest, UpdateWithEmptyUpdatesReturnsError) {
    adapter.insert_document(kIndex, make_doc("ue1", "Test", 1));

    auto result = adapter.update_documents(kIndex, {}, {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(ElasticsearchDocumentTest, UpdateNonExistentIndexReturnsZero) {
    std::map<std::string, Scalar> updates;
    updates["field"] = Scalar{std::string{"value"}};

    auto result = adapter.update_documents("no_such_index", {}, updates);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 0u);
}

TEST_F(ElasticsearchDocumentTest, OperationsWhileDisconnectedReturnError) {
    adapter.disconnect();

    auto r1 = adapter.insert_document(kIndex, make_doc("x", "y", 1));
    EXPECT_EQ(r1.error_code, ErrorCode::CONNECTION_ERROR);

    auto r2 = adapter.find_documents(kIndex, {});
    EXPECT_EQ(r2.error_code, ErrorCode::CONNECTION_ERROR);

    auto r3 = adapter.batch_insert_documents(kIndex, {make_doc("x", "y", 1)});
    EXPECT_EQ(r3.error_code, ErrorCode::CONNECTION_ERROR);

    std::map<std::string, Scalar> updates;
    updates["f"] = Scalar{std::string{"v"}};
    auto r4 = adapter.update_documents(kIndex, {}, updates);
    EXPECT_EQ(r4.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Graph adapter tests – all NOT_IMPLEMENTED
// ---------------------------------------------------------------------------

class ElasticsearchGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:9200");
    }

    ElasticsearchAdapter adapter;
};

TEST_F(ElasticsearchGraphTest, InsertNodeReturnsNotImplemented) {
    GraphNode node;
    node.id    = "node1";
    node.label = "Entity";

    auto result = adapter.insert_node(node);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchGraphTest, InsertEdgeReturnsNotImplemented) {
    GraphEdge edge;
    edge.id        = "e1";
    edge.source_id = "n1";
    edge.target_id = "n2";
    edge.label     = "LINKS_TO";

    auto result = adapter.insert_edge(edge);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchGraphTest, ShortestPathReturnsNotImplemented) {
    auto result = adapter.shortest_path("n1", "n2", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchGraphTest, TraverseReturnsNotImplemented) {
    auto result = adapter.traverse("n1", 3);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchGraphTest, ExecuteGraphQueryReturnsNotImplemented) {
    auto result = adapter.execute_graph_query("MATCH (n) RETURN n");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// Relational adapter tests – all NOT_IMPLEMENTED
// ---------------------------------------------------------------------------

class ElasticsearchRelationalTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:9200");
    }

    ElasticsearchAdapter adapter;
};

TEST_F(ElasticsearchRelationalTest, ExecuteQueryReturnsNotImplemented) {
    auto result = adapter.execute_query("SELECT * FROM table");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchRelationalTest, InsertRowReturnsNotImplemented) {
    RelationalRow row;
    row.columns["col"] = Scalar{std::string{"val"}};
    auto result = adapter.insert_row("table", row);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchRelationalTest, BatchInsertReturnsNotImplemented) {
    RelationalRow row;
    row.columns["col"] = Scalar{std::string{"val"}};
    auto result = adapter.batch_insert("table", {row});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchRelationalTest, GetQueryStatisticsReturnsOk) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Transaction adapter tests – all NOT_IMPLEMENTED
// ---------------------------------------------------------------------------

class ElasticsearchTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:9200");
    }

    ElasticsearchAdapter adapter;
};

TEST_F(ElasticsearchTransactionTest, BeginTransactionReturnsNotImplemented) {
    auto result = adapter.begin_transaction();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchTransactionTest, CommitTransactionReturnsNotImplemented) {
    auto result = adapter.commit_transaction("txn_001");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(ElasticsearchTransactionTest, RollbackTransactionReturnsNotImplemented) {
    auto result = adapter.rollback_transaction("txn_001");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// System info and capability tests
// ---------------------------------------------------------------------------

class ElasticsearchSystemInfoTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("http://localhost:9200");
    }

    ElasticsearchAdapter adapter;
};

TEST_F(ElasticsearchSystemInfoTest, GetSystemInfoReturnsElasticsearch) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().system_name, "Elasticsearch");
    EXPECT_FALSE(result.value.value().version.empty());
}

TEST_F(ElasticsearchSystemInfoTest, GetMetricsReturnsOk) {
    auto result = adapter.get_metrics();
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ElasticsearchSystemInfoTest, HasFullTextSearchCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::FULL_TEXT_SEARCH));
}

TEST_F(ElasticsearchSystemInfoTest, HasVectorSearchCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::VECTOR_SEARCH));
}

TEST_F(ElasticsearchSystemInfoTest, HasDocumentStoreCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::DOCUMENT_STORE));
}

TEST_F(ElasticsearchSystemInfoTest, HasBatchOperationsCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::BATCH_OPERATIONS));
}

TEST_F(ElasticsearchSystemInfoTest, HasSecondaryIndexesCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::SECONDARY_INDEXES));
}

TEST_F(ElasticsearchSystemInfoTest, HasDistributedQueriesCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::DISTRIBUTED_QUERIES));
}

TEST_F(ElasticsearchSystemInfoTest, HasShardingCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::SHARDING));
}

TEST_F(ElasticsearchSystemInfoTest, DoesNotHaveTransactionsCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::TRANSACTIONS));
}

TEST_F(ElasticsearchSystemInfoTest, DoesNotHaveRelationalQueriesCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
}

TEST_F(ElasticsearchSystemInfoTest, DoesNotHaveGraphTraversalCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
}

TEST_F(ElasticsearchSystemInfoTest, GetCapabilitiesContainsExpectedSet) {
    auto caps = adapter.get_capabilities();
    EXPECT_FALSE(caps.empty());

    auto has = [&](Capability c) {
        return std::find(caps.begin(), caps.end(), c) != caps.end();
    };

    EXPECT_TRUE(has(Capability::FULL_TEXT_SEARCH));
    EXPECT_TRUE(has(Capability::VECTOR_SEARCH));
    EXPECT_TRUE(has(Capability::DOCUMENT_STORE));
    EXPECT_TRUE(has(Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(has(Capability::SECONDARY_INDEXES));
    EXPECT_TRUE(has(Capability::DISTRIBUTED_QUERIES));
    EXPECT_TRUE(has(Capability::SHARDING));
}
