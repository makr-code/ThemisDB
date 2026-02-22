/**
 * @file test_mongodb_adapter.cpp
 * @brief Unit tests for MongoDBAdapter (CHIMERA Suite)
 *
 * @details Tests cover connection management, document CRUD, vector search,
 *          transaction lifecycle, capability reporting, and NOT_IMPLEMENTED
 *          paths for unsupported operations (relational, graph).
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/mongodb_adapter.hpp"

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

class MongoDBConnectionTest : public ::testing::Test {
protected:
    MongoDBAdapter adapter;
};

TEST_F(MongoDBConnectionTest, InitiallyDisconnected) {
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(MongoDBConnectionTest, ConnectWithValidUri) {
    auto result = adapter.connect("mongodb://localhost:27017/testdb");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(MongoDBConnectionTest, ConnectWithSrvUri) {
    auto result = adapter.connect("mongodb+srv://user:pass@cluster.mongodb.net/mydb");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(MongoDBConnectionTest, ConnectWithEmptyString) {
    auto result = adapter.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(MongoDBConnectionTest, ConnectWithInvalidScheme) {
    auto result = adapter.connect("postgres://localhost:5432/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(MongoDBConnectionTest, Disconnect) {
    adapter.connect("mongodb://localhost:27017");
    EXPECT_TRUE(adapter.is_connected());

    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(MongoDBConnectionTest, ConnectWithoutDatabaseName) {
    // No db name – adapter falls back to "test"
    auto result = adapter.connect("mongodb://localhost:27017");
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Document adapter tests
// ---------------------------------------------------------------------------

class MongoDBDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("mongodb://localhost:27017/testdb");
    }

    MongoDBAdapter adapter;
    static constexpr const char* kCollection = "test_docs";
};

TEST_F(MongoDBDocumentTest, InsertAndRetrieveDocument) {
    auto doc = make_doc("doc1", "Alice", 42);
    auto insert_result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(insert_result.is_ok());
    EXPECT_EQ(insert_result.value.value(), "doc1");

    auto find_result = adapter.find_documents(kCollection, {});
    ASSERT_TRUE(find_result.is_ok());
    ASSERT_EQ(find_result.value.value().size(), 1u);
    EXPECT_EQ(find_result.value.value()[0].id, "doc1");
}

TEST_F(MongoDBDocumentTest, InsertGeneratesIdWhenEmpty) {
    Document doc;
    doc.fields["name"] = Scalar{std::string{"NoId"}};
    auto result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
    EXPECT_NE(result.value.value().find("mongo_doc_"), std::string::npos);
}

TEST_F(MongoDBDocumentTest, InsertDuplicateIdReturnsError) {
    auto doc = make_doc("dup1", "Bob", 10);
    adapter.insert_document(kCollection, doc);

    auto dup_result = adapter.insert_document(kCollection, doc);
    EXPECT_TRUE(dup_result.is_err());
    EXPECT_EQ(dup_result.error_code, ErrorCode::ALREADY_EXISTS);
}

TEST_F(MongoDBDocumentTest, InsertEmptyCollectionNameReturnsError) {
    auto doc = make_doc("x", "X", 0);
    auto result = adapter.insert_document("", doc);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(MongoDBDocumentTest, BatchInsertDocuments) {
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

TEST_F(MongoDBDocumentTest, FindWithFilter) {
    adapter.insert_document(kCollection, make_doc("f1", "Alice", 1));
    adapter.insert_document(kCollection, make_doc("f2", "Bob",   2));
    adapter.insert_document(kCollection, make_doc("f3", "Alice", 3));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Alice"}};

    auto result = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 2u);
}

TEST_F(MongoDBDocumentTest, FindWithLimit) {
    for (int i = 0; i < 10; ++i) {
        adapter.insert_document(kCollection,
            make_doc("lim" + std::to_string(i), "User", static_cast<int64_t>(i)));
    }

    auto result = adapter.find_documents(kCollection, {}, /*limit=*/3);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 3u);
}

TEST_F(MongoDBDocumentTest, FindInNonExistentCollectionReturnsEmpty) {
    auto result = adapter.find_documents("no_such_collection", {});
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(MongoDBDocumentTest, UpdateDocuments) {
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

TEST_F(MongoDBDocumentTest, UpdateWithEmptyUpdatesReturnsError) {
    adapter.insert_document(kCollection, make_doc("eu1", "Eve", 1));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Eve"}};

    auto result = adapter.update_documents(kCollection, filter, {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(MongoDBDocumentTest, UpdateNonExistentCollectionReturnsZero) {
    std::map<std::string, Scalar> updates;
    updates["x"] = Scalar{int64_t{1}};

    auto result = adapter.update_documents("missing_coll", {}, updates);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 0u);
}

// Error path: operations while disconnected
TEST_F(MongoDBDocumentTest, OperationsWhileDisconnectedReturnError) {
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
// Vector adapter tests
// ---------------------------------------------------------------------------

class MongoDBVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("mongodb://localhost:27017/vectordb");
    }

    MongoDBAdapter adapter;
    static constexpr const char* kCollection = "vectors";
};

TEST_F(MongoDBVectorTest, InsertAndSearchVectors) {
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
    // First result should be closest to (1,0,0) or (1,1,0)
    EXPECT_GT(result.value.value()[0].second, 0.0);
}

TEST_F(MongoDBVectorTest, SearchReturnsSortedByDescendingSimilarity) {
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

TEST_F(MongoDBVectorTest, InsertVectorWithMetadata) {
    Vector v;
    v.data = {0.5f, 0.5f};
    v.metadata["category"] = Scalar{std::string{"A"}};

    auto result = adapter.insert_vector(kCollection, v);
    ASSERT_TRUE(result.is_ok());
}

TEST_F(MongoDBVectorTest, SearchWithMetadataFilter) {
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

TEST_F(MongoDBVectorTest, InsertEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.insert_vector(kCollection, empty);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(MongoDBVectorTest, SearchEmptyVectorReturnsError) {
    Vector empty;
    auto result = adapter.search_vectors(kCollection, empty, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(MongoDBVectorTest, SearchEmptyCollectionReturnsEmpty) {
    Vector query;
    query.data = {1.0f, 0.0f};

    auto result = adapter.search_vectors("empty_col", query, 5);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value().empty());
}

TEST_F(MongoDBVectorTest, BatchInsertVectors) {
    std::vector<Vector> vecs(3);
    vecs[0].data = {1.0f, 0.0f};
    vecs[1].data = {0.0f, 1.0f};
    vecs[2].data = {0.5f, 0.5f};

    auto result = adapter.batch_insert_vectors(kCollection, vecs);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 3u);
}

TEST_F(MongoDBVectorTest, CreateIndexSucceeds) {
    auto result = adapter.create_index(kCollection, 128);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(MongoDBVectorTest, OperationsWhileDisconnectedReturnError) {
    adapter.disconnect();

    Vector v;
    v.data = {1.0f, 0.0f};

    auto r1 = adapter.insert_vector(kCollection, v);
    EXPECT_EQ(r1.error_code, ErrorCode::CONNECTION_ERROR);

    auto r2 = adapter.search_vectors(kCollection, v, 5);
    EXPECT_EQ(r2.error_code, ErrorCode::CONNECTION_ERROR);

    auto r3 = adapter.batch_insert_vectors(kCollection, {v});
    EXPECT_EQ(r3.error_code, ErrorCode::CONNECTION_ERROR);

    auto r4 = adapter.create_index(kCollection, 2);
    EXPECT_EQ(r4.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Transaction tests
// ---------------------------------------------------------------------------

class MongoDBTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("mongodb://localhost:27017/txndb");
    }

    MongoDBAdapter adapter;
};

TEST_F(MongoDBTransactionTest, BeginCommitCycle) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());
    const std::string txn_id = begin.value.value();
    EXPECT_FALSE(txn_id.empty());

    auto commit = adapter.commit_transaction(txn_id);
    EXPECT_TRUE(commit.is_ok());
}

TEST_F(MongoDBTransactionTest, BeginRollbackCycle) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());
    const std::string txn_id = begin.value.value();

    auto rollback = adapter.rollback_transaction(txn_id);
    EXPECT_TRUE(rollback.is_ok());
}

TEST_F(MongoDBTransactionTest, CommitUnknownTransactionReturnsNotFound) {
    auto result = adapter.commit_transaction("no_such_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(MongoDBTransactionTest, RollbackUnknownTransactionReturnsNotFound) {
    auto result = adapter.rollback_transaction("no_such_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(MongoDBTransactionTest, DoubleCommitReturnsError) {
    auto begin = adapter.begin_transaction();
    ASSERT_TRUE(begin.is_ok());
    const std::string txn_id = begin.value.value();

    ASSERT_TRUE(adapter.commit_transaction(txn_id).is_ok());

    auto second_commit = adapter.commit_transaction(txn_id);
    EXPECT_TRUE(second_commit.is_err());
    EXPECT_EQ(second_commit.error_code, ErrorCode::TRANSACTION_ABORTED);
}

TEST_F(MongoDBTransactionTest, MultipleIndependentTransactions) {
    auto t1 = adapter.begin_transaction();
    auto t2 = adapter.begin_transaction();
    ASSERT_TRUE(t1.is_ok());
    ASSERT_TRUE(t2.is_ok());
    EXPECT_NE(t1.value.value(), t2.value.value());

    EXPECT_TRUE(adapter.commit_transaction(t1.value.value()).is_ok());
    EXPECT_TRUE(adapter.rollback_transaction(t2.value.value()).is_ok());
}

TEST_F(MongoDBTransactionTest, OperationsWhileDisconnectedReturnError) {
    adapter.disconnect();

    auto r1 = adapter.begin_transaction();
    EXPECT_EQ(r1.error_code, ErrorCode::CONNECTION_ERROR);

    auto r2 = adapter.commit_transaction("x");
    EXPECT_EQ(r2.error_code, ErrorCode::CONNECTION_ERROR);

    auto r3 = adapter.rollback_transaction("x");
    EXPECT_EQ(r3.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// NOT_IMPLEMENTED paths
// ---------------------------------------------------------------------------

class MongoDBNotImplementedTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("mongodb://localhost:27017/testdb");
    }

    MongoDBAdapter adapter;
};

TEST_F(MongoDBNotImplementedTest, ExecuteQueryNotImplemented) {
    auto result = adapter.execute_query("SELECT 1");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(MongoDBNotImplementedTest, InsertRowNotImplemented) {
    auto result = adapter.insert_row("table", RelationalRow{});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(MongoDBNotImplementedTest, BatchInsertRowsNotImplemented) {
    auto result = adapter.batch_insert("table", {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(MongoDBNotImplementedTest, InsertNodeNotImplemented) {
    auto result = adapter.insert_node(GraphNode{});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(MongoDBNotImplementedTest, InsertEdgeNotImplemented) {
    auto result = adapter.insert_edge(GraphEdge{});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(MongoDBNotImplementedTest, ShortestPathNotImplemented) {
    auto result = adapter.shortest_path("a", "b");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(MongoDBNotImplementedTest, TraverseNotImplemented) {
    auto result = adapter.traverse("a", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(MongoDBNotImplementedTest, ExecuteGraphQueryNotImplemented) {
    auto result = adapter.execute_graph_query("MATCH (n) RETURN n");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// Capability and system info tests
// ---------------------------------------------------------------------------

class MongoDBCapabilityTest : public ::testing::Test {
protected:
    MongoDBAdapter adapter;
};

TEST_F(MongoDBCapabilityTest, SupportedCapabilities) {
    EXPECT_TRUE(adapter.has_capability(Capability::DOCUMENT_STORE));
    EXPECT_TRUE(adapter.has_capability(Capability::FULL_TEXT_SEARCH));
    EXPECT_TRUE(adapter.has_capability(Capability::TRANSACTIONS));
    EXPECT_TRUE(adapter.has_capability(Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(adapter.has_capability(Capability::SECONDARY_INDEXES));
    EXPECT_TRUE(adapter.has_capability(Capability::VECTOR_SEARCH));
}

TEST_F(MongoDBCapabilityTest, UnsupportedCapabilities) {
    EXPECT_FALSE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(adapter.has_capability(Capability::DISTRIBUTED_QUERIES));
    EXPECT_FALSE(adapter.has_capability(Capability::SHARDING));
}

TEST_F(MongoDBCapabilityTest, GetCapabilitiesReturnsNonEmpty) {
    auto caps = adapter.get_capabilities();
    EXPECT_FALSE(caps.empty());
    EXPECT_GE(caps.size(), 6u);
}

TEST_F(MongoDBCapabilityTest, SystemInfoReturnsMongoDBName) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().system_name, "MongoDB");
    EXPECT_FALSE(result.value.value().version.empty());
}

TEST_F(MongoDBCapabilityTest, GetMetricsReturnsOk) {
    auto result = adapter.get_metrics();
    EXPECT_TRUE(result.is_ok());
}

TEST_F(MongoDBCapabilityTest, QueryStatisticsReturnsOk) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// AdapterFactory integration
// ---------------------------------------------------------------------------

TEST(MongoDBFactoryTest, RegisterAndCreate) {
    AdapterFactory::register_adapter("MongoDB",
        []() { return std::make_unique<MongoDBAdapter>(); });

    EXPECT_TRUE(AdapterFactory::is_supported("MongoDB"));

    auto adapter = AdapterFactory::create("MongoDB");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(adapter->is_connected());
}

TEST(MongoDBFactoryTest, CapabilitiesViaFactory) {
    // Ensure MongoDB is registered (may already be from previous test)
    AdapterFactory::register_adapter("MongoDB",
        []() { return std::make_unique<MongoDBAdapter>(); });

    auto adapter = AdapterFactory::create("MongoDB");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(adapter->has_capability(Capability::DOCUMENT_STORE));
    EXPECT_FALSE(adapter->has_capability(Capability::RELATIONAL_QUERIES));
}
