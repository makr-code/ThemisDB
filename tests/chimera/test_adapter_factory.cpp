/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_adapter_factory.cpp                           ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:22:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     720                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 04a46f63a9  2026-03-12  fix(chimera): address PR review comments on multi-databas... ║
    • 3bd2167e65  2026-03-12  feat(chimera): implement multi-database adapter registrat... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 1aba82430d  2026-02-28  fix(chimera): mask credentials in ThemisDBAdapter::connec... ║
    • 10fd73cb8a  2026-02-28  audit(chimera): fill all gaps identified in Pinecone adap... ║
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
    
    Result<QueryStatistics> get_query_statistics() const override {
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
    Result<SystemInfo> get_system_info() const override {
        SystemInfo info;
        info.system_name = "MockDB";
        info.version = "1.0.0";
        return Result<SystemInfo>::ok(std::move(info));
    }
    
    Result<SystemMetrics> get_metrics() const override {
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
    
    bool has_capability(Capability cap) const override {
        return false;
    }
    
    std::vector<Capability> get_capabilities() const override {
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

// ---------------------------------------------------------------------------
// Security audit: credential handling
// ---------------------------------------------------------------------------

TEST(ThemisDBAdapterSecurityTest, ConnectWithEmptyStringReturnsError) {
    ThemisDBAdapter adapter;
    auto result = adapter.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST(ThemisDBAdapterSecurityTest, ConnectWithInvalidSchemeReturnsError) {
    ThemisDBAdapter adapter;
    auto result = adapter.connect("postgresql://localhost:5432/mydb");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST(ThemisDBAdapterSecurityTest, ConnectWithValidSchemeSucceeds) {
    ThemisDBAdapter adapter;
    auto result = adapter.connect("themisdb://localhost:7777/testdb");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

TEST(ThemisDBAdapterSecurityTest, CredentialsNotExposedInSystemInfo) {
    ThemisDBAdapter adapter;
    adapter.connect("themisdb://admin:s3cr3t@localhost:7777/mydb");

    auto result = adapter.get_system_info();
    ASSERT_TRUE(result.is_ok());
    const auto& info = result.value.value();

    for (const auto& kv : info.build_info) {
        EXPECT_EQ(kv.second.find("s3cr3t"), std::string::npos)
            << "Credential leaked in build_info[" << kv.first << "]";
    }
    for (const auto& kv : info.configuration) {
        if (std::holds_alternative<std::string>(kv.second)) {
            EXPECT_EQ(std::get<std::string>(kv.second).find("s3cr3t"),
                      std::string::npos)
                << "Credential leaked in configuration[" << kv.first << "]";
        }
    }
}

TEST(ThemisDBAdapterSecurityTest, CredentialsNotExposedWithoutUserInfo) {
    ThemisDBAdapter adapter;
    auto result = adapter.connect("themisdb://localhost:7777/cleandb");
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(adapter.is_connected());
}

// ---------------------------------------------------------------------------
// Multi-Database Adapter Registration (v1.3.0)
// ---------------------------------------------------------------------------

/// A mock adapter that reports a configurable set of capabilities.
class CapableAdapter : public MockAdapter {
public:
    explicit CapableAdapter(std::vector<Capability> caps)
        : caps_(std::move(caps)) {}

    bool has_capability(Capability cap) const override {
        for (const auto& c : caps_) {
            if (c == cap) return true;
        }
        return false;
    }

    std::vector<Capability> get_capabilities() const override {
        return caps_;
    }

private:
    std::vector<Capability> caps_;
};

/**
 * @brief Version-specific adapters can be registered with "System:version" keys
 *        and retrieved individually via AdapterFactory::create.
 */
TEST(MultiDatabaseAdapterTest, VersionedAdapterRegistration) {
    AdapterFactory::register_adapter("VersionedDB:1",
        []() { return std::make_unique<MockAdapter>(); });
    AdapterFactory::register_adapter("VersionedDB:2",
        []() { return std::make_unique<MockAdapter>(); });
    AdapterFactory::register_adapter("VersionedDB:3",
        []() { return std::make_unique<MockAdapter>(); });

    EXPECT_TRUE(AdapterFactory::is_supported("VersionedDB:1"));
    EXPECT_TRUE(AdapterFactory::is_supported("VersionedDB:2"));
    EXPECT_TRUE(AdapterFactory::is_supported("VersionedDB:3"));
    EXPECT_FALSE(AdapterFactory::is_supported("VersionedDB:99"));

    auto v1 = AdapterFactory::create("VersionedDB:1");
    auto v2 = AdapterFactory::create("VersionedDB:2");
    auto v3 = AdapterFactory::create("VersionedDB:3");
    EXPECT_NE(v1, nullptr);
    EXPECT_NE(v2, nullptr);
    EXPECT_NE(v3, nullptr);
}

/**
 * @brief create_with_fallback returns the first *registered* candidate in list order.
 *
 * FallbackDB:4 is absent; FallbackDB:3 is the earliest registered candidate so
 * it must be returned – not FallbackDB:2.  The adapters are given distinguishable
 * capability sets so the test can verify *which* instance was returned.
 */
TEST(MultiDatabaseAdapterTest, CreateWithFallbackReturnsFirstRegistered) {
    // FallbackDB:3 supports RELATIONAL_QUERIES; FallbackDB:2 supports VECTOR_SEARCH.
    AdapterFactory::register_adapter("FallbackDB:3",
        []() {
            return std::make_unique<CapableAdapter>(
                std::vector<Capability>{Capability::RELATIONAL_QUERIES});
        });
    AdapterFactory::register_adapter("FallbackDB:2",
        []() {
            return std::make_unique<CapableAdapter>(
                std::vector<Capability>{Capability::VECTOR_SEARCH});
        });

    // FallbackDB:4 is not registered; FallbackDB:3 is first registered – it wins.
    auto adapter = AdapterFactory::create_with_fallback(
        {"FallbackDB:4", "FallbackDB:3", "FallbackDB:2"});
    ASSERT_NE(adapter, nullptr);
    // Verify FallbackDB:3 was selected (RELATIONAL_QUERIES present, VECTOR_SEARCH absent).
    EXPECT_TRUE(adapter->has_capability(Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(adapter->has_capability(Capability::VECTOR_SEARCH));
}

/**
 * @brief create_with_fallback returns nullptr when no candidate is registered.
 */
TEST(MultiDatabaseAdapterTest, CreateWithFallbackReturnsNullptrWhenNoneRegistered) {
    auto adapter = AdapterFactory::create_with_fallback(
        {"UnknownDB:99", "UnknownDB:98"});
    EXPECT_EQ(adapter, nullptr);
}

/**
 * @brief create_with_fallback on an empty candidate list returns nullptr.
 */
TEST(MultiDatabaseAdapterTest, CreateWithFallbackEmptyListReturnsNullptr) {
    auto adapter = AdapterFactory::create_with_fallback({});
    EXPECT_EQ(adapter, nullptr);
}

/**
 * @brief create_with_capabilities returns the first adapter that meets the
 *        required capability set (capability negotiation).
 */
TEST(MultiDatabaseAdapterTest, CreateWithCapabilitiesReturnsFirstQualifying) {
    AdapterFactory::register_adapter("CapDB:low",
        []() {
            return std::make_unique<CapableAdapter>(
                std::vector<Capability>{Capability::RELATIONAL_QUERIES});
        });
    AdapterFactory::register_adapter("CapDB:high",
        []() {
            return std::make_unique<CapableAdapter>(
                std::vector<Capability>{
                    Capability::RELATIONAL_QUERIES,
                    Capability::VECTOR_SEARCH,
                    Capability::TRANSACTIONS});
        });

    // Both are registered; only "CapDB:high" supports VECTOR_SEARCH.
    auto adapter = AdapterFactory::create_with_capabilities(
        {"CapDB:low", "CapDB:high"},
        {Capability::VECTOR_SEARCH});
    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(adapter->has_capability(Capability::VECTOR_SEARCH));
}

/**
 * @brief create_with_capabilities returns nullptr when no candidate meets all
 *        required capabilities.
 */
TEST(MultiDatabaseAdapterTest, CreateWithCapabilitiesReturnsNullptrWhenNoneQualify) {
    AdapterFactory::register_adapter("CapDB:none",
        []() {
            return std::make_unique<CapableAdapter>(std::vector<Capability>{});
        });

    auto adapter = AdapterFactory::create_with_capabilities(
        {"CapDB:none"},
        {Capability::GRAPH_TRAVERSAL});
    EXPECT_EQ(adapter, nullptr);
}

/**
 * @brief create_with_capabilities with an empty required-capability list
 *        behaves like create_with_fallback (first registered wins).
 */
TEST(MultiDatabaseAdapterTest, CreateWithCapabilitiesEmptyRequirementsActsAsFallback) {
    AdapterFactory::register_adapter("CapDB:any",
        []() { return std::make_unique<MockAdapter>(); });

    auto adapter = AdapterFactory::create_with_capabilities(
        {"CapDB:any"}, {});
    EXPECT_NE(adapter, nullptr);
}

/**
 * @brief When multiple candidates satisfy the required capabilities,
 *        create_with_capabilities returns the earliest qualifying candidate.
 */
TEST(MultiDatabaseAdapterTest, CreateWithCapabilitiesPrefersEarlierWhenMultipleQualify) {
    // First candidate: supports VECTOR_SEARCH only.
    AdapterFactory::register_adapter("CapDB:vs_only",
        []() {
            return std::make_unique<CapableAdapter>(
                std::vector<Capability>{Capability::VECTOR_SEARCH});
        });

    // Second candidate: supports VECTOR_SEARCH and TRANSACTIONS.
    AdapterFactory::register_adapter("CapDB:vs_plus_tx",
        []() {
            return std::make_unique<CapableAdapter>(
                std::vector<Capability>{
                    Capability::VECTOR_SEARCH,
                    Capability::TRANSACTIONS});
        });

    auto adapter = AdapterFactory::create_with_capabilities(
        {"CapDB:vs_only", "CapDB:vs_plus_tx"},
        {Capability::VECTOR_SEARCH});

    ASSERT_NE(adapter, nullptr);
    // Both candidates have VECTOR_SEARCH, so this alone doesn't distinguish them.
    EXPECT_TRUE(adapter->has_capability(Capability::VECTOR_SEARCH));
    // Only the second candidate has TRANSACTIONS; lack of this capability
    // proves the first candidate was selected.
    EXPECT_FALSE(adapter->has_capability(Capability::TRANSACTIONS));
}

/**
 * @brief register_adapter with static capability hints allows create_with_capabilities
 *        to negotiate without instantiating non-qualifying adapters.
 */
TEST(MultiDatabaseAdapterTest, CreateWithCapabilitiesUsesStaticHints) {
    AdapterFactory::register_adapter(
        "HintedDB:low",
        []() {
            return std::make_unique<CapableAdapter>(
                std::vector<Capability>{Capability::RELATIONAL_QUERIES});
        },
        {Capability::RELATIONAL_QUERIES});

    AdapterFactory::register_adapter(
        "HintedDB:high",
        []() {
            return std::make_unique<CapableAdapter>(
                std::vector<Capability>{
                    Capability::RELATIONAL_QUERIES,
                    Capability::GRAPH_TRAVERSAL});
        },
        {Capability::RELATIONAL_QUERIES, Capability::GRAPH_TRAVERSAL});

    // Only HintedDB:high satisfies GRAPH_TRAVERSAL.
    auto adapter = AdapterFactory::create_with_capabilities(
        {"HintedDB:low", "HintedDB:high"},
        {Capability::GRAPH_TRAVERSAL});

    ASSERT_NE(adapter, nullptr);
    EXPECT_TRUE(adapter->has_capability(Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(adapter->has_capability(Capability::VECTOR_SEARCH));
}

/**
 * @brief Main test runner
 */

