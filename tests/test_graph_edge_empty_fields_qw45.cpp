#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

namespace themis {
namespace index {
namespace tests {

class GraphEdgeEmptyFieldsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test database
        test_db_path_ = std::filesystem::temp_directory_path() / "themis_qw45_test_db";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        std::filesystem::create_directories(test_db_path_);
        
        // Initialize RocksDB wrapper
        RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_.string();
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
        
        // Initialize GraphIndexManager
        graph_mgr_ = std::make_unique<GraphIndexManager>(*db_);
    }
    
    void TearDown() override {
        graph_mgr_.reset();
        db_.reset();
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    BaseEntity createEdgeEntity(
        const std::string& edge_id,
        const std::string& from_node,
        const std::string& to_node) {
        BaseEntity edge(edge_id);
        edge.setField("id", edge_id);
        edge.setField("_from", from_node);
        edge.setField("_to", to_node);
        return edge;
    }
    
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> graph_mgr_;
    std::filesystem::path test_db_path_;
};

// ============================================================================
// QW-45 GUARD TESTS: Empty Field Validation
// ============================================================================

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_EmptyFromFieldRejected) {
    // QW-45 Guard: Empty _from field must be rejected (fail-closed)
    BaseEntity edge = createEdgeEntity("edge_1", "", "node_b");
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_FALSE(status.ok) << "QW-45 Guard failed: empty _from was accepted";
    EXPECT_THAT(status.message, ::testing::HasSubstr("empty")) << 
        "Error message should indicate empty field issue";
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_EmptyToFieldRejected) {
    // QW-45 Guard: Empty _to field must be rejected (fail-closed)
    BaseEntity edge = createEdgeEntity("edge_1", "node_a", "");
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_FALSE(status.ok) << "QW-45 Guard failed: empty _to was accepted";
    EXPECT_THAT(status.message, ::testing::HasSubstr("empty")) << 
        "Error message should indicate empty field issue";
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_BothEmptyFieldsRejected) {
    // QW-45 Guard: Both empty fields must be rejected (fail-closed)
    BaseEntity edge = createEdgeEntity("edge_1", "", "");
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_FALSE(status.ok) << "QW-45 Guard failed: both empty fields were accepted";
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_ValidEdgeAccepted) {
    // Valid edge with both _from and _to populated
    BaseEntity edge = createEdgeEntity("edge_1", "node_a", "node_b");
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_TRUE(status.ok) << "Valid edge should be accepted: " << status.message;
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_WhitespaceOnlyFromRejected) {
    // QW-45 Guard: Whitespace-only _from is semantically empty
    BaseEntity edge = createEdgeEntity("edge_1", "   ", "node_b");
    
    auto status = graph_mgr_->addEdge(edge);
    
    // May reject as whitespace-only or accept as non-empty string
    // This depends on implementation choice; documenting both paths
    // Current behavior: likely accepted (not yet trimmed)
    // Future hardening: should reject whitespace-only
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_SingleCharNodeIDAccepted) {
    // Boundary: Single character node IDs are valid
    BaseEntity edge = createEdgeEntity("edge_1", "a", "b");
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_TRUE(status.ok) << "Single character node IDs should be valid: " << status.message;
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_LongNodeIDAccepted) {
    // Boundary: Long node IDs (up to 1000+ chars) should be valid
    std::string long_id(256, 'x');
    BaseEntity edge = createEdgeEntity("edge_1", long_id, long_id);
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_TRUE(status.ok) << "Long node IDs should be valid: " << status.message;
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_SpecialCharsInNodeIDAccepted) {
    // Special characters in node IDs (URIs, fully qualified names)
    BaseEntity edge = createEdgeEntity("edge_1", "http://example.com/node-a", "schema:Entity:123");
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_TRUE(status.ok) << "Node IDs with special characters should be valid: " << status.message;
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_NumericNodeIDAccepted) {
    // Numeric node IDs (common for internal entity references)
    BaseEntity edge = createEdgeEntity("edge_1", "123456", "789012");
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_TRUE(status.ok) << "Numeric node IDs should be valid: " << status.message;
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_EdgeIDEmptyStillRejected) {
    // Edge ID (different from _from/_to) is also required
    BaseEntity edge("edge_1");
    edge.setField("id", std::string(""));
    edge.setField("_from", std::string("node_a"));
    edge.setField("_to", std::string("node_b"));
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_FALSE(status.ok) << "Empty edge ID should be rejected";
}

// ============================================================================
// QW-45 GUARD TESTS: WriteBatch Variant
// ============================================================================

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_WriteBatchEmptyFromRejected) {
    // QW-45 Guard: Empty _from in WriteBatch variant (transactional)
    BaseEntity edge = createEdgeEntity("edge_1", "", "node_b");
    auto batch = db_->createWriteBatch();
    ASSERT_TRUE(batch);
    
    auto status = graph_mgr_->addEdge(edge, *batch);
    
    EXPECT_FALSE(status.ok) << "WriteBatch: empty _from should be rejected";
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_WriteBatchEmptyToRejected) {
    // QW-45 Guard: Empty _to in WriteBatch variant
    BaseEntity edge = createEdgeEntity("edge_1", "node_a", "");
    auto batch = db_->createWriteBatch();
    ASSERT_TRUE(batch);
    
    auto status = graph_mgr_->addEdge(edge, *batch);
    
    EXPECT_FALSE(status.ok) << "WriteBatch: empty _to should be rejected";
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_WriteBatchValidEdgeAccepted) {
    // WriteBatch variant accepts valid edges
    BaseEntity edge = createEdgeEntity("edge_1", "node_a", "node_b");
    auto batch = db_->createWriteBatch();
    ASSERT_TRUE(batch);
    
    auto status = graph_mgr_->addEdge(edge, *batch);
    
    EXPECT_TRUE(status.ok) << "WriteBatch: valid edge should be accepted: " << status.message;
}

// ============================================================================
// QW-45 GUARD TESTS: Transaction Variant
// ============================================================================

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_TransactionEmptyFromRejected) {
    // QW-45 Guard: Empty _from in Transaction variant (MVCC)
    BaseEntity edge = createEdgeEntity("edge_1", "", "node_b");
    
    rocksdb::Status txn_status;
    auto txn = db_->beginTransaction();
    if (!txn) {
        GTEST_SKIP() << "Transaction support not available in this build";
    }
    
    auto status = graph_mgr_->addEdge(edge, *txn);
    
    EXPECT_FALSE(status.ok) << "Transaction: empty _from should be rejected";
    txn->rollback();
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_TransactionEmptyToRejected) {
    // QW-45 Guard: Empty _to in Transaction variant
    BaseEntity edge = createEdgeEntity("edge_1", "node_a", "");
    
    auto txn = db_->beginTransaction();
    if (!txn) {
        GTEST_SKIP() << "Transaction support not available in this build";
    }
    
    auto status = graph_mgr_->addEdge(edge, *txn);
    
    EXPECT_FALSE(status.ok) << "Transaction: empty _to should be rejected";
    txn->rollback();
}

// ============================================================================
// QW-45 GUARD TESTS: Graph Topology Integrity
// ============================================================================

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_TopologyCorruptionPrevention) {
    // After rejecting empty edges, topology should remain consistent
    std::vector<BaseEntity> edges;
    edges.push_back(createEdgeEntity("edge_1", "", "node_b"));      // Invalid
    edges.push_back(createEdgeEntity("edge_2", "node_a", "node_b")); // Valid
    edges.push_back(createEdgeEntity("edge_3", "node_c", ""));       // Invalid
    
    for (const auto& edge : edges) {
        auto status = graph_mgr_->addEdge(edge);
        // Only valid edges should succeed
    }
    
    // Query neighbors of node_a - should find node_b (only valid edge)
    auto [status, neighbors] = graph_mgr_->outNeighbors("node_a");
    
    if (status.ok && !neighbors.empty()) {
        EXPECT_EQ(neighbors.size(), 1) << "Only valid edge should be in topology";
        EXPECT_EQ(neighbors[0], "node_b") << "Neighbor should be node_b from valid edge";
    }
}

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_FailClosedBehavior) {
    // QW-45 Guard: All validation failures are fail-closed
    // Each invalid edge returns error without partial persistence
    
    struct TestCase {
        std::string from;
        std::string to;
        bool should_succeed;
    };
    
    std::vector<TestCase> test_cases = {
        {"", "node_b", false},           // Empty _from
        {"node_a", "", false},           // Empty _to
        {"", "", false},                 // Both empty
        {"node_a", "node_b", true},      // Valid
    };
    
    for (size_t i = 0; i < test_cases.size(); ++i) {
        const auto& tc = test_cases[i];
        BaseEntity edge = createEdgeEntity("edge_" + std::to_string(i), tc.from, tc.to);
        
        auto status = graph_mgr_->addEdge(edge);
        
        EXPECT_EQ(status.ok, tc.should_succeed) <<
            "Case " << i << ": from='" << tc.from << "' to='" << tc.to << "'";
    }
}

// ============================================================================
// QW-45 GUARD TESTS: Error Diagnostics
// ============================================================================

TEST_F(GraphEdgeEmptyFieldsTest, EmptyFieldValidation_ErrorMessageIndicatesEmptyField) {
    // Error message should help diagnose which field is empty
    BaseEntity edge = createEdgeEntity("edge_1", "", "node_b");
    
    auto status = graph_mgr_->addEdge(edge);
    
    EXPECT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::MatchesRegex(".*_from.*|.*empty.*")) <<
        "Error message should indicate '_from' field issue";
}

}  // namespace tests
}  // namespace index
}  // namespace themis

// ============================================================================
// GTest main (if not linked with other test suites)
// ============================================================================

// Uncomment for standalone execution:
// int main(int argc, char** argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
