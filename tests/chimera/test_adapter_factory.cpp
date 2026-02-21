/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_adapter_factory.cpp                           ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     447                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_adapter_factory.cpp
 * @brief Unit tests for CHIMERA AdapterFactory
 * 
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"

using namespace chimera;

/**
 * @brief Mock adapter for testing
 */
class MockAdapter : public IDatabaseAdapter {
public:
    MockAdapter() = default;
    ~MockAdapter() override = default;
    
    // Minimal implementations for testing
    Result<bool> connect(
        const std::string& connection_string,
        const std::map<std::string, std::string>& options = {}
    ) override {
        return Result<bool>::ok(true);
    }
    
    Result<bool> disconnect() override {
        return Result<bool>::ok(true);
    }
    
    bool is_connected() const override {
        return true;
    }
    
    // IRelationalAdapter
    Result<RelationalTable> execute_query(
        const std::string& query,
        const std::vector<Scalar>& params = {}
    ) override {
        return Result<RelationalTable>::ok(RelationalTable{});
    }
    
    Result<size_t> insert_row(
        const std::string& table_name,
        const RelationalRow& row
    ) override {
        return Result<size_t>::ok(1);
    }
    
    Result<size_t> batch_insert(
        const std::string& table_name,
        const std::vector<RelationalRow>& rows
    ) override {
        return Result<size_t>::ok(rows.size());
    }
    
    Result<QueryStatistics> get_query_statistics() override {
        QueryStatistics stats;
        stats.execution_time = std::chrono::microseconds(0);
        stats.rows_read = 0;
        stats.rows_returned = 0;
        stats.bytes_read = 0;
        return Result<QueryStatistics>::ok(std::move(stats));
    }
    
    // IVectorAdapter
    Result<std::string> insert_vector(
        const std::string& collection,
        const Vector& vector
    ) override {
        return Result<std::string>::ok("vector_001");
    }
    
    Result<size_t> batch_insert_vectors(
        const std::string& collection,
        const std::vector<Vector>& vectors
    ) override {
        return Result<size_t>::ok(vectors.size());
    }
    
    Result<std::vector<std::pair<Vector, double>>> search_vectors(
        const std::string& collection,
        const Vector& query_vector,
        size_t k,
        const std::map<std::string, Scalar>& filters = {}
    ) override {
        return Result<std::vector<std::pair<Vector, double>>>::ok({});
    }
    
    Result<bool> create_index(
        const std::string& collection,
        size_t dimensions,
        const std::map<std::string, Scalar>& index_params = {}
    ) override {
        return Result<bool>::ok(true);
    }
    
    // IGraphAdapter
    Result<std::string> insert_node(const GraphNode& node) override {
        return Result<std::string>::ok("node_001");
    }
    
    Result<std::string> insert_edge(const GraphEdge& edge) override {
        return Result<std::string>::ok("edge_001");
    }
    
    Result<GraphPath> shortest_path(
        const std::string& source_id,
        const std::string& target_id,
        size_t max_depth = 10
    ) override {
        GraphPath path;
        path.total_weight = 0.0;
        return Result<GraphPath>::ok(std::move(path));
    }
    
    Result<std::vector<GraphNode>> traverse(
        const std::string& start_id,
        size_t max_depth,
        const std::vector<std::string>& edge_labels = {}
    ) override {
        return Result<std::vector<GraphNode>>::ok({});
    }
    
    Result<std::vector<GraphPath>> execute_graph_query(
        const std::string& query,
        const std::map<std::string, Scalar>& params = {}
    ) override {
        return Result<std::vector<GraphPath>>::ok({});
    }
    
    // IDocumentAdapter
    Result<std::string> insert_document(
        const std::string& collection,
        const Document& doc
    ) override {
        return Result<std::string>::ok("doc_001");
    }
    
    Result<size_t> batch_insert_documents(
        const std::string& collection,
        const std::vector<Document>& docs
    ) override {
        return Result<size_t>::ok(docs.size());
    }
    
    Result<std::vector<Document>> find_documents(
        const std::string& collection,
        const std::map<std::string, Scalar>& filter,
        size_t limit = 100
    ) override {
        return Result<std::vector<Document>>::ok({});
    }
    
    Result<size_t> update_documents(
        const std::string& collection,
        const std::map<std::string, Scalar>& filter,
        const std::map<std::string, Scalar>& updates
    ) override {
        return Result<size_t>::ok(0);
    }
    
    // ITransactionAdapter
    Result<std::string> begin_transaction(
        const TransactionOptions& options = {}
    ) override {
        return Result<std::string>::ok("txn_001");
    }
    
    Result<bool> commit_transaction(const std::string& transaction_id) override {
        return Result<bool>::ok(true);
    }
    
    Result<bool> rollback_transaction(const std::string& transaction_id) override {
        return Result<bool>::ok(true);
    }
    
    // ISystemInfoAdapter
    Result<SystemInfo> get_system_info() override {
        SystemInfo info;
        info.system_name = "MockDB";
        info.version = "1.0.0";
        return Result<SystemInfo>::ok(std::move(info));
    }
    
    Result<SystemMetrics> get_metrics() override {
        SystemMetrics metrics;
        metrics.memory.total_bytes = 0;
        metrics.memory.used_bytes = 0;
        metrics.memory.available_bytes = 0;
        metrics.storage.total_bytes = 0;
        metrics.storage.used_bytes = 0;
        metrics.storage.available_bytes = 0;
        metrics.cpu.utilization_percent = 0.0;
        metrics.cpu.thread_count = 0;
        return Result<SystemMetrics>::ok(std::move(metrics));
    }
    
    bool has_capability(Capability cap) override {
        return false;
    }
    
    std::vector<Capability> get_capabilities() override {
        return {};
    }
};

/**
 * @brief Test fixture for AdapterFactory tests
 */
class AdapterFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register test adapters
        AdapterFactory::register_adapter("MockDB",
            []() { return std::make_unique<MockAdapter>(); });
        AdapterFactory::register_adapter("ThemisDB",
            []() { return std::make_unique<ThemisDBAdapter>(); });
    }
};

/**
 * @brief Test adapter creation
 */
TEST_F(AdapterFactoryTest, CreateAdapter) {
    auto adapter = AdapterFactory::create("MockDB");
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(adapter->is_connected());
}

/**
 * @brief Test creation of non-existent adapter
 */
TEST_F(AdapterFactoryTest, CreateNonExistentAdapter) {
    auto adapter = AdapterFactory::create("NonExistentDB");
    EXPECT_EQ(adapter, nullptr);
}

/**
 * @brief Test listing supported systems
 */
TEST_F(AdapterFactoryTest, GetSupportedSystems) {
    auto systems = AdapterFactory::get_supported_systems();
    EXPECT_GE(systems.size(), 2);
    
    // Check alphabetical ordering (vendor neutrality)
    for (size_t i = 1; i < systems.size(); ++i) {
        EXPECT_LT(systems[i - 1], systems[i]);
    }
}

/**
 * @brief Test checking system support
 */
TEST_F(AdapterFactoryTest, IsSupported) {
    EXPECT_TRUE(AdapterFactory::is_supported("MockDB"));
    EXPECT_TRUE(AdapterFactory::is_supported("ThemisDB"));
    EXPECT_FALSE(AdapterFactory::is_supported("UnknownDB"));
}

/**
 * @brief Test duplicate registration prevention
 */
TEST_F(AdapterFactoryTest, PreventDuplicateRegistration) {
    bool first_registration = AdapterFactory::register_adapter("TestDB",
        []() { return std::make_unique<MockAdapter>(); });
    EXPECT_TRUE(first_registration);
    
    bool second_registration = AdapterFactory::register_adapter("TestDB",
        []() { return std::make_unique<MockAdapter>(); });
    EXPECT_FALSE(second_registration);
}

/**
 * @brief Test Result type success case
 */
TEST(ResultTest, SuccessCase) {
    auto result = Result<int>::ok(42);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_err());
    ASSERT_TRUE(result.value.has_value());
    EXPECT_EQ(result.value.value(), 42);
    EXPECT_EQ(result.error_code, ErrorCode::SUCCESS);
}

/**
 * @brief Test Result type error case
 */
TEST(ResultTest, ErrorCase) {
    auto result = Result<int>::err(ErrorCode::NOT_FOUND, "Item not found");
    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_err());
    EXPECT_FALSE(result.value.has_value());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_FOUND);
    EXPECT_EQ(result.error_message, "Item not found");
}

/**
 * @brief Test Vector structure
 */
TEST(DataStructuresTest, Vector) {
    Vector vec;
    vec.data = {1.0f, 2.0f, 3.0f, 4.0f};
    vec.metadata["category"] = Scalar{"test"};
    
    EXPECT_EQ(vec.dimensions(), 4);
    EXPECT_EQ(vec.data[0], 1.0f);
    EXPECT_EQ(vec.data[3], 4.0f);
}

/**
 * @brief Test Document structure
 */
TEST(DataStructuresTest, Document) {
    Document doc;
    doc.id = "doc_123";
    doc.fields["title"] = Scalar{"Test Document"};
    doc.fields["count"] = Scalar{int64_t{100}};
    doc.fields["active"] = Scalar{true};
    
    EXPECT_EQ(doc.id, "doc_123");
    EXPECT_EQ(doc.fields.size(), 3);
}

/**
 * @brief Test GraphNode and GraphEdge structures
 */
TEST(DataStructuresTest, Graph) {
    GraphNode node;
    node.id = "node_1";
    node.label = "Person";
    node.properties["name"] = Scalar{"Alice"};
    
    GraphEdge edge;
    edge.id = "edge_1";
    edge.source_id = "node_1";
    edge.target_id = "node_2";
    edge.label = "KNOWS";
    edge.weight = 1.5;
    
    EXPECT_EQ(node.id, "node_1");
    EXPECT_EQ(edge.source_id, "node_1");
    EXPECT_TRUE(edge.weight.has_value());
    EXPECT_EQ(edge.weight.value(), 1.5);
}

/**
 * @brief Test Scalar variant
 */
TEST(DataStructuresTest, Scalar) {
    Scalar null_val = std::monostate{};
    Scalar bool_val = true;
    Scalar int_val = int64_t{42};
    Scalar double_val = 3.14;
    Scalar string_val = std::string{"hello"};
    Scalar binary_val = std::vector<uint8_t>{0x01, 0x02, 0x03};
    
    EXPECT_TRUE(std::holds_alternative<std::monostate>(null_val));
    EXPECT_TRUE(std::holds_alternative<bool>(bool_val));
    EXPECT_TRUE(std::holds_alternative<int64_t>(int_val));
    EXPECT_TRUE(std::holds_alternative<double>(double_val));
    EXPECT_TRUE(std::holds_alternative<std::string>(string_val));
    EXPECT_TRUE(std::holds_alternative<std::vector<uint8_t>>(binary_val));
}

/**
 * @brief Test ThemisDB adapter instantiation
 */
TEST(ThemisDBAdapterTest, Instantiation) {
    ThemisDBAdapter adapter;
    EXPECT_FALSE(adapter.is_connected());
}

/**
 * @brief Test ThemisDB adapter connection
 */
TEST(ThemisDBAdapterTest, Connection) {
    ThemisDBAdapter adapter;
    auto result = adapter.connect("themisdb://localhost:7777");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
    
    auto disconnect_result = adapter.disconnect();
    EXPECT_TRUE(disconnect_result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

/**
 * @brief Test ThemisDB adapter capabilities
 */
TEST(ThemisDBAdapterTest, Capabilities) {
    ThemisDBAdapter adapter;
    
    EXPECT_TRUE(adapter.has_capability(Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(adapter.has_capability(Capability::VECTOR_SEARCH));
    EXPECT_TRUE(adapter.has_capability(Capability::GRAPH_TRAVERSAL));
    
    auto caps = adapter.get_capabilities();
    EXPECT_GT(caps.size(), 0);
}

/**
 * @brief Test ThemisDB adapter system info
 */
TEST(ThemisDBAdapterTest, SystemInfo) {
    ThemisDBAdapter adapter;
    auto result = adapter.get_system_info();
    
    ASSERT_TRUE(result.is_ok());
    auto info = result.value.value();
    EXPECT_EQ(info.system_name, "ThemisDB");
    EXPECT_FALSE(info.version.empty());
}

/**
 * @brief Main test runner
 */

