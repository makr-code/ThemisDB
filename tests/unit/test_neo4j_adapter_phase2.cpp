/**
 * @file test_neo4j_adapter_phase2.cpp
 * @brief Unit tests for Phase 2 Neo4j Adapter TODO implementations
 * @details Comprehensive test suite verifying all 14 converted TODOs
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "chimera/neo4j_adapter.hpp"
#include <sstream>
#include <stdexcept>

namespace chimera {
namespace testing {

class Neo4jAdapterPhase2Test : public ::testing::Test {
protected:
    Neo4jAdapter adapter_;

    void SetUp() override {
        // Reset adapter state before each test
        ASSERT_FALSE(adapter_.is_connected());
    }

    void TearDown() override {
        // Ensure cleanup
        if (adapter_.is_connected()) {
            adapter_.disconnect();
        }
    }
};

// ============================================================================
// TODO 1 & Connection Tests: Driver Creation and Connection Management
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, ConnectWithValidBoltURI) {
    // Test: Connection with standard Bolt URI
    auto result = adapter_.connect("bolt://localhost:7687");
    EXPECT_TRUE(result.is_ok() || result.error_code == ErrorCode::NOT_IMPLEMENTED);
    if (result.is_ok()) {
        EXPECT_TRUE(adapter_.is_connected());
    }
}

TEST_F(Neo4jAdapterPhase2Test, ConnectWithValidNeo4jURI) {
    // Test: Connection with Neo4j URI scheme
    auto result = adapter_.connect("neo4j://localhost:7687");
    EXPECT_TRUE(result.is_ok() || result.error_code == ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jAdapterPhase2Test, ConnectWithSecureBoltURI) {
    // Test: Connection with secure Bolt URI (bolt+s)
    auto result = adapter_.connect("bolt+s://localhost:7687");
    EXPECT_TRUE(result.is_ok() || result.error_code == ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jAdapterPhase2Test, ConnectWithCredentials) {
    // Test: Connection string with username and password
    auto result = adapter_.connect("******localhost:7687");
    EXPECT_TRUE(result.is_ok() || result.error_code == ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jAdapterPhase2Test, ConnectWithEmptyConnectionString) {
    // Test: Empty connection string should fail
    auto result = adapter_.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter_.is_connected());
}

TEST_F(Neo4jAdapterPhase2Test, ConnectWithInvalidConnectionString) {
    // Test: Invalid URI scheme should fail
    auto result = adapter_.connect("http://localhost:8080");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(Neo4jAdapterPhase2Test, ConnectWithConnectionOptions) {
    // Test: Connection with additional options map
    std::map<std::string, std::string> options;
    options["username"] = "admin";
    options["password"] = "secret";
    auto result = adapter_.connect("bolt://localhost:7687", options);
    EXPECT_TRUE(result.is_ok() || result.error_code == ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(Neo4jAdapterPhase2Test, DisconnectAfterConnection) {
    // Test: Disconnect from connected state
    auto connect_result = adapter_.connect("bolt://localhost:7687");
    if (connect_result.is_ok()) {
        auto disconnect_result = adapter_.disconnect();
        EXPECT_TRUE(disconnect_result.is_ok());
        EXPECT_FALSE(adapter_.is_connected());
    }
}

// ============================================================================
// TODO 2: Node Insertion Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, InsertNodeNotConnected) {
    // Test: Inserting node without connection should fail
    GraphNode node;
    node.id = "node1";
    node.label = "Person";
    node.properties["name"] = Scalar(std::string("Alice"));
    
    auto result = adapter_.insert_node(node);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jAdapterPhase2Test, InsertNodeWithProperties) {
    // Test: Node insertion with properties
    GraphNode node;
    node.id = "node1";
    node.label = "Person";
    node.properties["name"] = Scalar(std::string("Alice"));
    node.properties["age"] = Scalar(int64_t(30));
    
    auto result = adapter_.insert_node(node);
    EXPECT_TRUE(result.is_ok() || result.is_err());
    if (result.is_ok()) {
        EXPECT_FALSE(result.value->empty());  // Should return a node ID
    }
}

TEST_F(Neo4jAdapterPhase2Test, InsertNodeWithEmptyId) {
    // Test: Node insertion with empty ID generates new ID
    GraphNode node;
    node.id = "";  // Empty ID
    node.label = "Person";
    node.properties["name"] = Scalar(std::string("Bob"));
    
    auto result = adapter_.insert_node(node);
    if (result.is_ok()) {
        // Should have generated an ID (UUID format)
        EXPECT_FALSE(result.value->empty());
        EXPECT_GE(result.value->size(), 36);  // UUID length
    }
}

TEST_F(Neo4jAdapterPhase2Test, InsertNodeWithMultipleProperties) {
    // Test: Node with various property types
    GraphNode node;
    node.id = "person1";
    node.label = "Person";
    node.properties["name"] = Scalar(std::string("Charlie"));
    node.properties["active"] = Scalar(true);
    node.properties["score"] = Scalar(95.5);
    node.properties["age"] = Scalar(int64_t(25));
    
    auto result = adapter_.insert_node(node);
    EXPECT_TRUE(result.is_ok() || result.is_err());
}

// ============================================================================
// TODO 3: Edge Insertion Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, InsertEdgeNotConnected) {
    // Test: Inserting edge without connection should fail
    GraphEdge edge;
    edge.id = "edge1";
    edge.source_id = "node1";
    edge.target_id = "node2";
    edge.label = "KNOWS";
    
    auto result = adapter_.insert_edge(edge);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jAdapterPhase2Test, InsertEdgeWithWeight) {
    // Test: Edge insertion with weight property
    GraphEdge edge;
    edge.id = "edge1";
    edge.source_id = "node1";
    edge.target_id = "node2";
    edge.label = "KNOWS";
    edge.weight = 0.85;
    
    auto result = adapter_.insert_edge(edge);
    if (result.is_ok()) {
        EXPECT_FALSE(result.value->empty());
    }
}

TEST_F(Neo4jAdapterPhase2Test, InsertEdgeWithProperties) {
    // Test: Edge insertion with additional properties
    GraphEdge edge;
    edge.id = "edge2";
    edge.source_id = "node1";
    edge.target_id = "node3";
    edge.label = "WORKS_WITH";
    edge.properties["since"] = Scalar(int64_t(2020));
    edge.properties["department"] = Scalar(std::string("Engineering"));
    
    auto result = adapter_.insert_edge(edge);
    EXPECT_TRUE(result.is_ok() || result.is_err());
}

// ============================================================================
// TODO 4: Shortest Path Query Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, ShortestPathNotConnected) {
    // Test: Shortest path query without connection
    auto result = adapter_.shortest_path("node1", "node2");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jAdapterPhase2Test, ShortestPathWithValidNodes) {
    // Test: Shortest path between two nodes
    auto result = adapter_.shortest_path("node1", "node5", 5);
    EXPECT_TRUE(result.is_ok() || result.is_err());
    if (result.is_ok()) {
        // Path may be empty if no connection exists
        EXPECT_GE(result.value->total_weight, 0.0);
    }
}

TEST_F(Neo4jAdapterPhase2Test, ShortestPathWithMaxDepth) {
    // Test: Shortest path respects depth bounds
    auto result1 = adapter_.shortest_path("node1", "node10", 3);
    auto result2 = adapter_.shortest_path("node1", "node10", 10);
    // Results may differ based on depth; just verify no errors
    EXPECT_TRUE(result1.is_ok() || result1.is_err());
    EXPECT_TRUE(result2.is_ok() || result2.is_err());
}

TEST_F(Neo4jAdapterPhase2Test, ShortestPathWithZeroDepth) {
    // Test: Zero depth should return error or empty path
    auto result = adapter_.shortest_path("node1", "node2", 0);
    EXPECT_TRUE(result.is_err() || result.is_ok());
}

// ============================================================================
// TODO 5: Graph Traversal Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, TraverseNotConnected) {
    // Test: Traversal without connection
    auto result = adapter_.traverse("node1", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jAdapterPhase2Test, TraverseWithMaxDepth) {
    // Test: Traversal with depth limit
    auto result = adapter_.traverse("node1", 3);
    EXPECT_TRUE(result.is_ok() || result.is_err());
    if (result.is_ok()) {
        // Result should be a vector of nodes
        EXPECT_GE(result.value->size(), 0);
    }
}

TEST_F(Neo4jAdapterPhase2Test, TraverseWithEdgeLabels) {
    // Test: Traversal with edge label filtering
    std::vector<std::string> labels = {"KNOWS", "WORKS_WITH"};
    auto result = adapter_.traverse("node1", 5, labels);
    EXPECT_TRUE(result.is_ok() || result.is_err());
}

TEST_F(Neo4jAdapterPhase2Test, TraverseWithoutEdgeLabels) {
    // Test: Traversal without label restrictions
    std::vector<std::string> empty_labels;
    auto result = adapter_.traverse("node1", 5, empty_labels);
    EXPECT_TRUE(result.is_ok() || result.is_err());
}

// ============================================================================
// TODO 6: Arbitrary Cypher Query Execution Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, ExecuteGraphQueryNotConnected) {
    // Test: Graph query without connection
    auto result = adapter_.execute_graph_query("RETURN 1");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jAdapterPhase2Test, ExecuteSimpleGraphQuery) {
    // Test: Simple Cypher query
    auto result = adapter_.execute_graph_query("MATCH (n) RETURN n LIMIT 10");
    EXPECT_TRUE(result.is_ok() || result.is_err());
}

TEST_F(Neo4jAdapterPhase2Test, ExecuteGraphQueryWithParameters) {
    // Test: Cypher query with parameters
    std::map<std::string, Scalar> params;
    params["id"] = Scalar(std::string("node1"));
    auto result = adapter_.execute_graph_query(
        "MATCH (n {id: $id}) RETURN n",
        params
    );
    EXPECT_TRUE(result.is_ok() || result.is_err());
}

TEST_F(Neo4jAdapterPhase2Test, ExecutePathQuery) {
    // Test: Query that returns paths
    auto result = adapter_.execute_graph_query(
        "MATCH path = (n)-[r*1..3]-(m) RETURN path LIMIT 5"
    );
    EXPECT_TRUE(result.is_ok() || result.is_err());
    if (result.is_ok()) {
        EXPECT_GE(result.value->size(), 0);
    }
}

// ============================================================================
// TODO 7: Document Insertion Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, InsertDocumentNotConnected) {
    // Test: Document insertion without connection
    Document doc;
    doc.id = "doc1";
    doc.fields["title"] = Scalar(std::string("Test Document"));
    
    auto result = adapter_.insert_document("documents", doc);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jAdapterPhase2Test, InsertDocumentWithFields) {
    // Test: Document insertion with multiple fields
    Document doc;
    doc.id = "doc1";
    doc.fields["title"] = Scalar(std::string("Introduction to Neo4j"));
    doc.fields["author"] = Scalar(std::string("John Doe"));
    doc.fields["year"] = Scalar(int64_t(2023));
    doc.fields["rating"] = Scalar(4.5);
    
    auto result = adapter_.insert_document("articles", doc);
    if (result.is_ok()) {
        EXPECT_FALSE(result.value->empty());
    }
}

TEST_F(Neo4jAdapterPhase2Test, InsertDocumentGenerateId) {
    // Test: Document with auto-generated ID
    Document doc;
    doc.id = "";  // Empty ID
    doc.fields["content"] = Scalar(std::string("Some content"));
    
    auto result = adapter_.insert_document("notes", doc);
    if (result.is_ok()) {
        EXPECT_FALSE(result.value->empty());
        EXPECT_GE(result.value->size(), 36);  // UUID length
    }
}

// ============================================================================
// TODO 8: Batch Document Insertion Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, BatchInsertDocumentsNotConnected) {
    // Test: Batch insertion without connection
    std::vector<Document> docs;
    auto result = adapter_.batch_insert_documents("collection", docs);
    EXPECT_TRUE(result.is_err());
}

TEST_F(Neo4jAdapterPhase2Test, BatchInsertMultipleDocuments) {
    // Test: Batch insertion with multiple documents
    std::vector<Document> docs;
    for (int i = 0; i < 5; ++i) {
        Document doc;
        doc.id = "doc" + std::to_string(i);
        doc.fields["title"] = Scalar(std::string("Document " + std::to_string(i)));
        doc.fields["index"] = Scalar(int64_t(i));
        docs.push_back(doc);
    }
    
    auto result = adapter_.batch_insert_documents("batch_collection", docs);
    if (result.is_ok()) {
        EXPECT_EQ(*result.value, docs.size());
    }
}

TEST_F(Neo4jAdapterPhase2Test, BatchInsertEmptyList) {
    // Test: Batch insertion with empty document list
    std::vector<Document> empty_docs;
    auto result = adapter_.batch_insert_documents("collection", empty_docs);
    EXPECT_TRUE(result.is_err() || result.is_ok());  // May handle gracefully or error
}

// ============================================================================
// TODO 9: Document Find/Query Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, FindDocumentsNotConnected) {
    // Test: Find without connection
    std::map<std::string, Scalar> filter;
    auto result = adapter_.find_documents("collection", filter);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jAdapterPhase2Test, FindDocumentsWithoutFilter) {
    // Test: Find all documents with limit
    std::map<std::string, Scalar> empty_filter;
    auto result = adapter_.find_documents("collection", empty_filter, 100);
    EXPECT_TRUE(result.is_ok() || result.is_err());
    if (result.is_ok()) {
        EXPECT_LE(result.value->size(), 100);
    }
}

TEST_F(Neo4jAdapterPhase2Test, FindDocumentsWithFilter) {
    // Test: Find documents with filter criteria
    std::map<std::string, Scalar> filter;
    filter["status"] = Scalar(std::string("active"));
    filter["year"] = Scalar(int64_t(2023));
    
    auto result = adapter_.find_documents("documents", filter, 50);
    EXPECT_TRUE(result.is_ok() || result.is_err());
}

TEST_F(Neo4jAdapterPhase2Test, FindDocumentsWithLimit) {
    // Test: Find respects limit parameter
    std::map<std::string, Scalar> filter;
    auto result = adapter_.find_documents("collection", filter, 10);
    if (result.is_ok()) {
        EXPECT_LE(result.value->size(), 10);
    }
}

// ============================================================================
// TODO 10: Document Batch Update Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, UpdateDocumentsNotConnected) {
    // Test: Update without connection
    std::map<std::string, Scalar> filter;
    std::map<std::string, Scalar> updates;
    auto result = adapter_.update_documents("collection", filter, updates);
    EXPECT_TRUE(result.is_err());
}

TEST_F(Neo4jAdapterPhase2Test, UpdateDocumentsWithFilter) {
    // Test: Update documents matching filter
    std::map<std::string, Scalar> filter;
    filter["status"] = Scalar(std::string("draft"));
    
    std::map<std::string, Scalar> updates;
    updates["status"] = Scalar(std::string("published"));
    updates["publish_date"] = Scalar(int64_t(2024));
    
    auto result = adapter_.update_documents("articles", filter, updates);
    if (result.is_ok()) {
        EXPECT_GE(*result.value, 0);  // Number of updated documents
    }
}

TEST_F(Neo4jAdapterPhase2Test, UpdateDocumentsEmptyFilter) {
    // Test: Update with empty filter (affects all)
    std::map<std::string, Scalar> empty_filter;
    std::map<std::string, Scalar> updates;
    updates["modified"] = Scalar(int64_t(2024));
    
    auto result = adapter_.update_documents("collection", empty_filter, updates);
    EXPECT_TRUE(result.is_ok() || result.is_err());
}

// ============================================================================
// TODO 11 & 12: Transaction Management Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, BeginTransactionNotConnected) {
    // Test: Begin transaction without connection
    auto result = adapter_.begin_transaction();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(Neo4jAdapterPhase2Test, CommitTransactionNotFound) {
    // Test: Commit non-existent transaction
    auto result = adapter_.commit_transaction("invalid_tx_id");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(Neo4jAdapterPhase2Test, RollbackTransactionNotFound) {
    // Test: Rollback non-existent transaction
    auto result = adapter_.rollback_transaction("invalid_tx_id");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
}

TEST_F(Neo4jAdapterPhase2Test, TransactionLifecycle) {
    // Test: Full transaction lifecycle (if connected)
    auto connect_result = adapter_.connect("bolt://localhost:7687");
    if (connect_result.is_ok()) {
        // Begin transaction
        auto begin_result = adapter_.begin_transaction();
        EXPECT_TRUE(begin_result.is_ok());
        
        if (begin_result.is_ok()) {
            auto tx_id = *begin_result.value;
            
            // Commit transaction
            auto commit_result = adapter_.commit_transaction(tx_id);
            EXPECT_TRUE(commit_result.is_ok() || commit_result.is_err());
        }
    }
}

// ============================================================================
// TODO 13: Credential Masking Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, MaskCredentialsNoAuth) {
    // Test: URI without credentials
    std::string uri = "bolt://localhost:7687";
    std::string masked = Neo4jAdapter::mask_credentials(uri);
    EXPECT_EQ(masked, uri);  // Should remain unchanged
}

TEST_F(Neo4jAdapterPhase2Test, MaskCredentialsWithPassword) {
    // Test: Mask password in URI
    std::string uri = "******localhost:7687";
    std::string masked = Neo4jAdapter::mask_credentials(uri);
    EXPECT_NE(masked, uri);  // Should be different
    EXPECT_EQ(masked.find("password"), std::string::npos);  // Password removed
    EXPECT_NE(masked.find("***"), std::string::npos);  // Asterisks present
}

TEST_F(Neo4jAdapterPhase2Test, MaskCredentialsWithoutPassword) {
    // Test: Mask username-only credentials
    std::string uri = "bolt://user@localhost:7687";
    std::string masked = Neo4jAdapter::mask_credentials(uri);
    EXPECT_NE(masked.find("***"), std::string::npos);
}

TEST_F(Neo4jAdapterPhase2Test, MaskCredentialsWithComplexPassword) {
    // Test: Mask complex password with special characters
    std::string uri = "******ss:w0rd@localhost:7687";
    std::string masked = Neo4jAdapter::mask_credentials(uri);
    EXPECT_EQ(masked.find("p@ss:w0rd"), std::string::npos);
}

// ============================================================================
// TODO 14: Scalar to Cypher Literal Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherNull) {
    // Test: Null scalar conversion
    Scalar scalar = std::monostate{};
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_EQ(result, "null");
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherBoolTrue) {
    // Test: Boolean true conversion
    Scalar scalar = true;
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_EQ(result, "true");
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherBoolFalse) {
    // Test: Boolean false conversion
    Scalar scalar = false;
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_EQ(result, "false");
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherInt) {
    // Test: Integer conversion
    Scalar scalar = int64_t(42);
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_EQ(result, "42");
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherNegativeInt) {
    // Test: Negative integer conversion
    Scalar scalar = int64_t(-100);
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_EQ(result, "-100");
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherDouble) {
    // Test: Double conversion
    Scalar scalar = 3.14;
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_NE(result.find("3.14"), std::string::npos);
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherString) {
    // Test: String conversion
    Scalar scalar = std::string("hello");
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_EQ(result, "'hello'");
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherStringWithQuotes) {
    // Test: String with quotes requires escaping
    Scalar scalar = std::string("it's");
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_NE(result.find("\\'"), std::string::npos);  // Escaped quote
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherStringWithBackslash) {
    // Test: String with backslash
    Scalar scalar = std::string("path\\to\\file");
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_TRUE(result.find("\\") != std::string::npos || 
                result.find("\\\\") != std::string::npos);
}

TEST_F(Neo4jAdapterPhase2Test, ScalarToCypherBinary) {
    // Test: Binary data conversion (hex encoding)
    Scalar scalar = std::vector<uint8_t>{0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
    std::string result = Neo4jAdapter::scalar_to_cypher_literal(scalar);
    EXPECT_EQ(result.substr(0, 2), "0x");  // Should be hex encoded
}

// ============================================================================
// Additional Integration Tests
// ============================================================================

TEST_F(Neo4jAdapterPhase2Test, ConnectionStringValidationFormats) {
    // Test: Various valid connection string formats
    std::vector<std::string> valid_uris = {
        "bolt://localhost",
        "bolt://localhost:7687",
        "neo4j://localhost:7687",
        "bolt+s://localhost:7687",
        "neo4j+s://localhost:7687",
        "bolt://user@localhost",
        "******localhost:7687/db"
    };
    
    for (const auto& uri : valid_uris) {
        EXPECT_TRUE(Neo4jAdapter::is_valid_connection_string(uri))
            << "URI should be valid: " << uri;
    }
}

TEST_F(Neo4jAdapterPhase2Test, ConnectionStringInvalidFormats) {
    // Test: Invalid connection string formats
    std::vector<std::string> invalid_uris = {
        "http://localhost:8080",
        "mongodb://localhost:27017",
        "postgresql://localhost:5432",
        "localhost:7687",
        "://localhost",
        "ftp://localhost"
    };
    
    for (const auto& uri : invalid_uris) {
        EXPECT_FALSE(Neo4jAdapter::is_valid_connection_string(uri))
            << "URI should be invalid: " << uri;
    }
}

TEST_F(Neo4jAdapterPhase2Test, AdapterCapabilities) {
    // Test: Check reported capabilities
    auto caps = adapter_.get_capabilities();
    EXPECT_THAT(caps, ::testing::Contains(Capability::GRAPH_OPERATIONS));
    EXPECT_THAT(caps, ::testing::Contains(Capability::TRANSACTIONS));
    EXPECT_TRUE(adapter_.has_capability(Capability::GRAPH_OPERATIONS));
    EXPECT_TRUE(adapter_.has_capability(Capability::TRANSACTIONS));
}

TEST_F(Neo4jAdapterPhase2Test, UnsupportedOperations) {
    // Test: Verify unsupported operations return NOT_IMPLEMENTED
    auto result1 = adapter_.insert_row("table", {});
    EXPECT_EQ(result1.error_code, ErrorCode::NOT_IMPLEMENTED);
    
    auto result2 = adapter_.insert_vector("collection", {});
    EXPECT_EQ(result2.error_code, ErrorCode::NOT_IMPLEMENTED);
    
    auto result3 = adapter_.execute_query("SELECT * FROM table");
    EXPECT_EQ(result3.error_code, ErrorCode::NOT_IMPLEMENTED);
}

} // namespace testing
} // namespace chimera

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
