/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_neo4j_adapter.cpp                             ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:22:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     773                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c12588b7a1  2026-02-28  feat(chimera): add Neo4j native graph database adapter ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_neo4j_adapter.cpp
 * @brief Unit tests for Neo4jAdapter (CHIMERA Suite)
 *
 * @details Tests cover connection management, graph operations (primary
 *          capability), ACID transactions, document CRUD, capability
 *          reporting, and NOT_IMPLEMENTED paths for unsupported operations
 *          (relational queries, vector search).
 *
 * All tests run without a live Neo4j server – the adapter operates in
 * its in-process simulation mode.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/neo4j_adapter.hpp"

#include <algorithm>

using namespace chimera;

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------

static GraphNode make_node(const std::string& id,
                            const std::string& label,
                            const std::string& name = "") {
    GraphNode node;
    node.id    = id;
    node.label = label;
    if (!name.empty()) {
        node.properties["name"] = Scalar{name};
    }
    return node;
}

static GraphEdge make_edge(const std::string& id,
                            const std::string& src,
                            const std::string& tgt,
                            const std::string& label,
                            std::optional<double> weight = std::nullopt) {
    GraphEdge edge;
    edge.id        = id;
    edge.source_id = src;
    edge.target_id = tgt;
    edge.label     = label;
    edge.weight    = weight;
    return edge;
}

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

class Neo4jConnectionTest : public ::testing::Test {
protected:
    Neo4jAdapter adapter;
};

TEST_F(Neo4jConnectionTest, InitiallyDisconnected) {
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(Neo4jConnectionTest, ConnectWithBoltUri) {
    auto result = adapter.connect("bolt://localhost:7687");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(Neo4jConnectionTest, ConnectWithNeo4jUri) {
    auto result = adapter.connect("neo4j://localhost:7687");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(Neo4jConnectionTest, ConnectWithNeo4jSecureUri) {
    auto result = adapter.connect("neo4j+s://my-cluster.neo4j.io");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST_F(Neo4jConnectionTest, ConnectWithEmptyStringReturnsError) {
    auto result = adapter.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(Neo4jConnectionTest, ConnectWithHttpSchemeReturnsError) {
    auto result = adapter.connect("http://localhost:7474");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(Neo4jConnectionTest, ConnectWithPostgresSchemeReturnsError) {
    auto result = adapter.connect("postgresql://localhost:5432/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(Neo4jConnectionTest, Disconnect) {
    adapter.connect("bolt://localhost:7687");
    EXPECT_TRUE(adapter.is_connected());

    auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST_F(Neo4jConnectionTest, ConnectWithCredentials) {
    std::map<std::string, std::string> options;
    options["username"] = "neo4j";
    options["password"] = "my-secret-password";
    auto result = adapter.connect("bolt://localhost:7687", options);
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

// ---------------------------------------------------------------------------
// Graph adapter tests (primary capability)
// ---------------------------------------------------------------------------

class Neo4jGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("bolt://localhost:7687");
    }

    Neo4jAdapter adapter;
};

TEST_F(Neo4jGraphTest, InsertNodeReturnsId) {
    auto node = make_node("n1", "Person", "Alice");
    auto result = adapter.insert_node(node);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "n1");
}

TEST_F(Neo4jGraphTest, InsertNodeWithAutoGeneratedId) {
    GraphNode node;
    node.label = "Person";
    node.properties["name"] = Scalar{std::string{"Bob"}};

    auto result = adapter.insert_node(node);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(Neo4jGraphTest, InsertDuplicateNodeReturnsError) {
    auto node = make_node("n1", "Person", "Alice");
    adapter.insert_node(node);

    auto dup = make_node("n1", "Person", "Alice2");
    auto result = adapter.insert_node(dup);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::ALREADY_EXISTS);
}

TEST_F(Neo4jGraphTest, InsertEdgeReturnsId) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));

    auto edge = make_edge("e1", "n1", "n2", "KNOWS", 1.0);
    auto result = adapter.insert_edge(edge);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "e1");
}

TEST_F(Neo4jGraphTest, InsertEdgeWithMissingSourceReturnsError) {
    adapter.insert_node(make_node("n2", "Person", "Bob"));

    auto edge = make_edge("e1", "nonexistent", "n2", "KNOWS");
    auto result = adapter.insert_edge(edge);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(Neo4jGraphTest, InsertEdgeWithMissingTargetReturnsError) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));

    auto edge = make_edge("e1", "n1", "nonexistent", "KNOWS");
    auto result = adapter.insert_edge(edge);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(Neo4jGraphTest, InsertDuplicateEdgeReturnsError) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));

    auto edge = make_edge("e1", "n1", "n2", "KNOWS");
    adapter.insert_edge(edge);

    auto dup = make_edge("e1", "n1", "n2", "KNOWS");
    auto result = adapter.insert_edge(dup);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::ALREADY_EXISTS);
}

TEST_F(Neo4jGraphTest, TraverseFromNode) {
    // n1 -> n2 -> n3
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));
    adapter.insert_node(make_node("n3", "Person", "Carol"));

    adapter.insert_edge(make_edge("e1", "n1", "n2", "KNOWS"));
    adapter.insert_edge(make_edge("e2", "n2", "n3", "KNOWS"));

    auto result = adapter.traverse("n1", 5);
    ASSERT_TRUE(result.is_ok());
    const auto& nodes = result.value.value();
    EXPECT_EQ(nodes.size(), 3u);
}

TEST_F(Neo4jGraphTest, TraverseRespectsMaxDepth) {
    // n1 -> n2 -> n3
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));
    adapter.insert_node(make_node("n3", "Person", "Carol"));

    adapter.insert_edge(make_edge("e1", "n1", "n2", "KNOWS"));
    adapter.insert_edge(make_edge("e2", "n2", "n3", "KNOWS"));

    // Depth 1: only n1 and n2 should be reachable
    auto result = adapter.traverse("n1", 1);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 2u);
}

TEST_F(Neo4jGraphTest, TraverseFiltersEdgeLabels) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));
    adapter.insert_node(make_node("n3", "City", "London"));

    adapter.insert_edge(make_edge("e1", "n1", "n2", "KNOWS"));
    adapter.insert_edge(make_edge("e2", "n1", "n3", "LIVES_IN"));

    // Only follow KNOWS edges
    auto result = adapter.traverse("n1", 5, {"KNOWS"});
    ASSERT_TRUE(result.is_ok());
    const auto& nodes = result.value.value();
    EXPECT_EQ(nodes.size(), 2u);

    bool has_london = false;
    for (const auto& n : nodes) {
        if (n.id == "n3") has_london = true;
    }
    EXPECT_FALSE(has_london);
}

TEST_F(Neo4jGraphTest, TraverseMissingNodeReturnsError) {
    auto result = adapter.traverse("nonexistent", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(Neo4jGraphTest, ShortestPathDirectEdge) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));
    adapter.insert_edge(make_edge("e1", "n1", "n2", "KNOWS", 1.5));

    auto result = adapter.shortest_path("n1", "n2");
    ASSERT_TRUE(result.is_ok());
    const auto& path = result.value.value();
    EXPECT_EQ(path.nodes.size(), 2u);
    EXPECT_EQ(path.edges.size(), 1u);
    EXPECT_DOUBLE_EQ(path.total_weight, 1.5);
}

TEST_F(Neo4jGraphTest, ShortestPathMultiHop) {
    // n1 -> n2 -> n3
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));
    adapter.insert_node(make_node("n3", "Person", "Carol"));

    adapter.insert_edge(make_edge("e1", "n1", "n2", "KNOWS", 1.0));
    adapter.insert_edge(make_edge("e2", "n2", "n3", "KNOWS", 2.0));

    auto result = adapter.shortest_path("n1", "n3");
    ASSERT_TRUE(result.is_ok());
    const auto& path = result.value.value();
    EXPECT_EQ(path.nodes.size(), 3u);
    EXPECT_EQ(path.edges.size(), 2u);
    EXPECT_DOUBLE_EQ(path.total_weight, 3.0);
}

TEST_F(Neo4jGraphTest, ShortestPathSameSourceAndTarget) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));

    auto result = adapter.shortest_path("n1", "n1");
    ASSERT_TRUE(result.is_ok());
    const auto& path = result.value.value();
    EXPECT_EQ(path.nodes.size(), 1u);
    EXPECT_TRUE(path.edges.empty());
}

TEST_F(Neo4jGraphTest, ShortestPathNoRouteReturnsError) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));
    // No edge between n1 and n2

    auto result = adapter.shortest_path("n1", "n2");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(Neo4jGraphTest, ShortestPathMissingSourceReturnsError) {
    adapter.insert_node(make_node("n2", "Person", "Bob"));

    auto result = adapter.shortest_path("nonexistent", "n2");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(Neo4jGraphTest, ShortestPathMissingTargetReturnsError) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));

    auto result = adapter.shortest_path("n1", "nonexistent");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(Neo4jGraphTest, ExecuteGraphQueryReturnsResult) {
    adapter.insert_node(make_node("n1", "Person", "Alice"));
    adapter.insert_node(make_node("n2", "Person", "Bob"));
    adapter.insert_edge(make_edge("e1", "n1", "n2", "KNOWS"));

    auto result = adapter.execute_graph_query(
        "MATCH (n:Person)-[:KNOWS]->(m:Person) RETURN n, m"
    );
    ASSERT_TRUE(result.is_ok());
    const auto& paths = result.value.value();
    EXPECT_FALSE(paths.empty());
    EXPECT_FALSE(paths[0].nodes.empty());
}

TEST_F(Neo4jGraphTest, ExecuteGraphQueryEmptyQueryReturnsError) {
    auto result = adapter.execute_graph_query("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

// ---------------------------------------------------------------------------
// Graph operations while not connected
// ---------------------------------------------------------------------------

class Neo4jGraphDisconnectedTest : public ::testing::Test {
protected:
    Neo4jAdapter adapter;  // not connected
};

TEST_F(Neo4jGraphDisconnectedTest, InsertNodeReturnsConnectionError) {
    auto result = adapter.insert_node(make_node("n1", "Person"));
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jGraphDisconnectedTest, InsertEdgeReturnsConnectionError) {
    auto result = adapter.insert_edge(make_edge("e1", "n1", "n2", "KNOWS"));
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jGraphDisconnectedTest, TraverseReturnsConnectionError) {
    auto result = adapter.traverse("n1", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jGraphDisconnectedTest, ShortestPathReturnsConnectionError) {
    auto result = adapter.shortest_path("n1", "n2");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jGraphDisconnectedTest, ExecuteGraphQueryReturnsConnectionError) {
    auto result = adapter.execute_graph_query("MATCH (n) RETURN n");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

// ---------------------------------------------------------------------------
// Transaction tests
// ---------------------------------------------------------------------------

class Neo4jTransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("bolt://localhost:7687");
    }

    Neo4jAdapter adapter;
};

TEST_F(Neo4jTransactionTest, BeginTransactionReturnsId) {
    auto result = adapter.begin_transaction();
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(Neo4jTransactionTest, CommitTransactionSucceeds) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());

    auto result = adapter.commit_transaction(txn.value.value());
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value());
}

TEST_F(Neo4jTransactionTest, RollbackTransactionSucceeds) {
    auto txn = adapter.begin_transaction();
    ASSERT_TRUE(txn.is_ok());

    auto result = adapter.rollback_transaction(txn.value.value());
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(result.value.value());
}

TEST_F(Neo4jTransactionTest, CommitUnknownTransactionReturnsError) {
    auto result = adapter.commit_transaction("unknown_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(Neo4jTransactionTest, RollbackUnknownTransactionReturnsError) {
    auto result = adapter.rollback_transaction("unknown_txn");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
}

TEST_F(Neo4jTransactionTest, CommitEmptyIdReturnsError) {
    auto result = adapter.commit_transaction("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(Neo4jTransactionTest, RollbackEmptyIdReturnsError) {
    auto result = adapter.rollback_transaction("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(Neo4jTransactionTest, BeginTransactionWhileDisconnectedReturnsError) {
    Neo4jAdapter disconnected;
    auto result = disconnected.begin_transaction();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jTransactionTest, MultipleActiveTransactions) {
    auto txn1 = adapter.begin_transaction();
    auto txn2 = adapter.begin_transaction();
    ASSERT_TRUE(txn1.is_ok());
    ASSERT_TRUE(txn2.is_ok());
    EXPECT_NE(txn1.value.value(), txn2.value.value());

    EXPECT_TRUE(adapter.commit_transaction(txn1.value.value()).is_ok());
    EXPECT_TRUE(adapter.rollback_transaction(txn2.value.value()).is_ok());
}

// ---------------------------------------------------------------------------
// Document adapter tests (node property store)
// ---------------------------------------------------------------------------

class Neo4jDocumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("bolt://localhost:7687");
    }

    Neo4jAdapter adapter;
    static constexpr const char* kCollection = "Person";
};

TEST_F(Neo4jDocumentTest, InsertDocumentReturnsId) {
    auto doc = make_doc("d1", "Alice", 30);
    auto result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), "d1");
}

TEST_F(Neo4jDocumentTest, InsertDocumentWithAutoGeneratedId) {
    Document doc;
    doc.fields["name"] = Scalar{std::string{"Bob"}};

    auto result = adapter.insert_document(kCollection, doc);
    ASSERT_TRUE(result.is_ok());
    EXPECT_FALSE(result.value.value().empty());
}

TEST_F(Neo4jDocumentTest, InsertDuplicateDocumentReturnsError) {
    auto doc = make_doc("d1", "Alice", 30);
    adapter.insert_document(kCollection, doc);

    auto dup = make_doc("d1", "Alice2", 31);
    auto result = adapter.insert_document(kCollection, dup);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::ALREADY_EXISTS);
}

TEST_F(Neo4jDocumentTest, FindDocumentsWithFilter) {
    adapter.insert_document(kCollection, make_doc("d1", "Alice", 30));
    adapter.insert_document(kCollection, make_doc("d2", "Bob", 25));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Alice"}};

    auto result = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 1u);
    EXPECT_EQ(result.value.value()[0].id, "d1");
}

TEST_F(Neo4jDocumentTest, FindDocumentsEmptyFilterReturnsAll) {
    adapter.insert_document(kCollection, make_doc("d1", "Alice", 30));
    adapter.insert_document(kCollection, make_doc("d2", "Bob", 25));

    auto result = adapter.find_documents(kCollection, {});
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 2u);
}

TEST_F(Neo4jDocumentTest, FindDocumentsRespectsLimit) {
    for (int i = 0; i < 5; ++i) {
        adapter.insert_document(
            kCollection,
            make_doc("d" + std::to_string(i), "Name" + std::to_string(i), i)
        );
    }

    auto result = adapter.find_documents(kCollection, {}, 3);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().size(), 3u);
}

TEST_F(Neo4jDocumentTest, UpdateDocumentsSucceeds) {
    adapter.insert_document(kCollection, make_doc("d1", "Alice", 30));

    std::map<std::string, Scalar> filter;
    filter["name"] = Scalar{std::string{"Alice"}};

    std::map<std::string, Scalar> updates;
    updates["value"] = Scalar{int64_t{31}};

    auto result = adapter.update_documents(kCollection, filter, updates);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 1u);

    auto docs = adapter.find_documents(kCollection, filter);
    ASSERT_TRUE(docs.is_ok());
    ASSERT_EQ(docs.value.value().size(), 1u);
    EXPECT_EQ(std::get<int64_t>(docs.value.value()[0].fields.at("value")), 31);
}

TEST_F(Neo4jDocumentTest, UpdateDocumentsEmptyUpdatesReturnsError) {
    auto result = adapter.update_documents(kCollection, {}, {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(Neo4jDocumentTest, BatchInsertDocumentsSucceeds) {
    std::vector<Document> docs;
    docs.push_back(make_doc("d1", "Alice", 30));
    docs.push_back(make_doc("d2", "Bob", 25));
    docs.push_back(make_doc("d3", "Carol", 35));

    auto result = adapter.batch_insert_documents(kCollection, docs);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value(), 3u);
}

// ---------------------------------------------------------------------------
// NOT_IMPLEMENTED paths – relational and vector operations
// ---------------------------------------------------------------------------

class Neo4jUnsupportedTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("bolt://localhost:7687");
    }

    Neo4jAdapter adapter;
};

TEST_F(Neo4jUnsupportedTest, ExecuteQueryReturnsNotImplemented) {
    auto result = adapter.execute_query("SELECT 1");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jUnsupportedTest, InsertRowReturnsNotImplemented) {
    RelationalRow row;
    row.columns["col"] = Scalar{int64_t{1}};
    auto result = adapter.insert_row("table", row);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jUnsupportedTest, BatchInsertRowsReturnsNotImplemented) {
    auto result = adapter.batch_insert("table", {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jUnsupportedTest, InsertVectorReturnsNotImplemented) {
    Vector v;
    v.data = {1.0f, 0.0f};
    auto result = adapter.insert_vector("col", v);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jUnsupportedTest, BatchInsertVectorsReturnsNotImplemented) {
    auto result = adapter.batch_insert_vectors("col", {});
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jUnsupportedTest, SearchVectorsReturnsNotImplemented) {
    Vector q;
    q.data = {1.0f, 0.0f};
    auto result = adapter.search_vectors("col", q, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jUnsupportedTest, CreateIndexReturnsNotImplemented) {
    auto result = adapter.create_index("col", 128);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ---------------------------------------------------------------------------
// Capability and system info tests
// ---------------------------------------------------------------------------

class Neo4jCapabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter.connect("bolt://localhost:7687");
    }

    Neo4jAdapter adapter;
};

TEST_F(Neo4jCapabilityTest, HasGraphTraversalCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
}

TEST_F(Neo4jCapabilityTest, HasTransactionCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::TRANSACTIONS));
}

TEST_F(Neo4jCapabilityTest, HasBatchOperationsCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::BATCH_OPERATIONS));
}

TEST_F(Neo4jCapabilityTest, HasSecondaryIndexesCapability) {
    EXPECT_TRUE(adapter.has_capability(Capability::SECONDARY_INDEXES));
}

TEST_F(Neo4jCapabilityTest, DoesNotHaveVectorSearchCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::VECTOR_SEARCH));
}

TEST_F(Neo4jCapabilityTest, DoesNotHaveRelationalCapability) {
    EXPECT_FALSE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
}

TEST_F(Neo4jCapabilityTest, GetCapabilitiesIsNonEmpty) {
    auto caps = adapter.get_capabilities();
    EXPECT_FALSE(caps.empty());
}

TEST_F(Neo4jCapabilityTest, GetCapabilitiesContainsGraphTraversal) {
    auto caps = adapter.get_capabilities();
    auto it = std::find(caps.begin(), caps.end(), Capability::GRAPH_TRAVERSAL);
    EXPECT_NE(it, caps.end());
}

TEST_F(Neo4jCapabilityTest, GetCapabilitiesContainsTransactions) {
    auto caps = adapter.get_capabilities();
    auto it = std::find(caps.begin(), caps.end(), Capability::TRANSACTIONS);
    EXPECT_NE(it, caps.end());
}

TEST_F(Neo4jCapabilityTest, GetSystemInfoReturnsNeo4j) {
    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value.value().system_name, "Neo4j");
    EXPECT_FALSE(result.value.value().version.empty());
}

TEST_F(Neo4jCapabilityTest, GetSystemInfoDoesNotExposeCredentials) {
    std::map<std::string, std::string> opts;
    opts["username"] = "neo4j";
    opts["password"] = "supersecret";
    Neo4jAdapter secure_adapter;
    secure_adapter.connect("bolt://localhost:7687", opts);

    auto result = secure_adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    const auto& info = result.value.value();
    // Password must not appear in any configuration value
    for (const auto& kv : info.configuration) {
        if (std::holds_alternative<std::string>(kv.second)) {
            EXPECT_EQ(std::get<std::string>(kv.second).find("supersecret"),
                      std::string::npos)
                << "Credential leaked in configuration field: " << kv.first;
        }
    }
}

TEST_F(Neo4jCapabilityTest, GetMetricsSucceeds) {
    auto result = adapter.get_metrics();
    EXPECT_TRUE(result.is_ok());
}

TEST_F(Neo4jCapabilityTest, GetQueryStatisticsSucceeds) {
    auto result = adapter.get_query_statistics();
    EXPECT_TRUE(result.is_ok());
}

// ---------------------------------------------------------------------------
// Factory registration test
// ---------------------------------------------------------------------------

TEST(Neo4jFactoryTest, IsRegisteredInAdapterFactory) {
    EXPECT_TRUE(AdapterFactory::is_supported("Neo4j"));
}

TEST(Neo4jFactoryTest, FactoryCreatesNeo4jAdapter) {
    auto adapter = AdapterFactory::create("Neo4j");
    ASSERT_NE(adapter, nullptr);
    EXPECT_FALSE(adapter->is_connected());
}
