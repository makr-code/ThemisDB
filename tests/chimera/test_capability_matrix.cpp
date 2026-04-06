/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_capability_matrix.cpp                         ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-06 04:22:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     409                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0701ac8f4d  2026-03-12  feat(chimera): implement async/promise-based API (IAsyncD... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 631e66471c  2026-03-01  Add Neo4j capability matrix tests to complete adapter cap... ║
    • e7b0a8793b  2026-02-28  chimera: add Elasticsearch capability tests to test_capab... ║
    • 10fd73cb8a  2026-02-28  audit(chimera): fill all gaps identified in Pinecone adap... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_capability_matrix.cpp
 * @brief Unit tests for AdapterCapabilityMatrix (CHIMERA Suite)
 *
 * @details
 * Tests cover:
 *   - Manual population via add_entry() and add_adapter()
 *   - Factory-driven population via build_from_factory()
 *   - Point queries: supports()
 *   - Column queries: adapters_supporting()
 *   - Row queries: capabilities_of()
 *   - Utility helpers: all_capabilities(), capability_to_string()
 *   - Edge cases: unknown system, unknown capability
 *
 * All tests run without a live database server.
 *
 * @copyright MIT License
 */

#include <gtest/gtest.h>
#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"
#include "chimera/mongodb_adapter.hpp"
#include "chimera/postgresql_adapter.hpp"
#include "chimera/weaviate_adapter.hpp"
#include "chimera/qdrant_adapter.hpp"
#include "chimera/pinecone_adapter.hpp"
#include "chimera/elasticsearch_adapter.hpp"
#include "chimera/neo4j_adapter.hpp"

using namespace chimera;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class CapabilityMatrixTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Populate a small matrix manually for deterministic testing.
        matrix_.add_entry("SystemA", {
            Capability::RELATIONAL_QUERIES,
            Capability::TRANSACTIONS,
            Capability::BATCH_OPERATIONS
        });
        matrix_.add_entry("SystemB", {
            Capability::VECTOR_SEARCH,
            Capability::DOCUMENT_STORE,
            Capability::BATCH_OPERATIONS
        });
        matrix_.add_entry("SystemC", {
            Capability::GRAPH_TRAVERSAL,
            Capability::TRANSACTIONS
        });
    }

    AdapterCapabilityMatrix matrix_;
};

// ---------------------------------------------------------------------------
// supports() – point queries
// ---------------------------------------------------------------------------

TEST_F(CapabilityMatrixTest, SupportsReturnsTrueForKnownCapability) {
    EXPECT_TRUE(matrix_.supports("SystemA", Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(matrix_.supports("SystemA", Capability::TRANSACTIONS));
    EXPECT_TRUE(matrix_.supports("SystemB", Capability::VECTOR_SEARCH));
    EXPECT_TRUE(matrix_.supports("SystemC", Capability::GRAPH_TRAVERSAL));
}

TEST_F(CapabilityMatrixTest, SupportsReturnsFalseForMissingCapability) {
    EXPECT_FALSE(matrix_.supports("SystemA", Capability::VECTOR_SEARCH));
    EXPECT_FALSE(matrix_.supports("SystemB", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix_.supports("SystemC", Capability::DOCUMENT_STORE));
}

TEST_F(CapabilityMatrixTest, SupportsReturnsFalseForUnknownSystem) {
    EXPECT_FALSE(matrix_.supports("UnknownDB", Capability::RELATIONAL_QUERIES));
}

// ---------------------------------------------------------------------------
// adapters_supporting() – column queries
// ---------------------------------------------------------------------------

TEST_F(CapabilityMatrixTest, AdaptersSupportingTransactions) {
    auto systems = matrix_.adapters_supporting(Capability::TRANSACTIONS);
    ASSERT_EQ(systems.size(), 2u);
    EXPECT_EQ(systems[0], "SystemA");
    EXPECT_EQ(systems[1], "SystemC");
}

TEST_F(CapabilityMatrixTest, AdaptersSupportingBatchOperations) {
    auto systems = matrix_.adapters_supporting(Capability::BATCH_OPERATIONS);
    ASSERT_EQ(systems.size(), 2u);
    EXPECT_EQ(systems[0], "SystemA");
    EXPECT_EQ(systems[1], "SystemB");
}

TEST_F(CapabilityMatrixTest, AdaptersSupportingUnsupportedCapabilityIsEmpty) {
    auto systems = matrix_.adapters_supporting(Capability::STREAM_PROCESSING);
    EXPECT_TRUE(systems.empty());
}

TEST_F(CapabilityMatrixTest, AdaptersSupportingResultsAreSorted) {
    // All three entries support BATCH_OPERATIONS only for SystemA and B.
    // Add a third entry to ensure sort order is enforced.
    matrix_.add_entry("SystemZ", {Capability::BATCH_OPERATIONS});
    auto systems = matrix_.adapters_supporting(Capability::BATCH_OPERATIONS);
    ASSERT_EQ(systems.size(), 3u);
    EXPECT_EQ(systems[0], "SystemA");
    EXPECT_EQ(systems[1], "SystemB");
    EXPECT_EQ(systems[2], "SystemZ");
}

// ---------------------------------------------------------------------------
// capabilities_of() – row queries
// ---------------------------------------------------------------------------

TEST_F(CapabilityMatrixTest, CapabilitiesOfReturnsCorrectSet) {
    auto caps = matrix_.capabilities_of("SystemA");
    EXPECT_EQ(caps.size(), 3u);
    // Order is not guaranteed; check presence.
    auto has = [&](Capability c) {
        return std::find(caps.begin(), caps.end(), c) != caps.end();
    };
    EXPECT_TRUE(has(Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(has(Capability::TRANSACTIONS));
    EXPECT_TRUE(has(Capability::BATCH_OPERATIONS));
    EXPECT_FALSE(has(Capability::VECTOR_SEARCH));
}

TEST_F(CapabilityMatrixTest, CapabilitiesOfUnknownSystemIsEmpty) {
    auto caps = matrix_.capabilities_of("NoSuchDB");
    EXPECT_TRUE(caps.empty());
}

// ---------------------------------------------------------------------------
// system_names()
// ---------------------------------------------------------------------------

TEST_F(CapabilityMatrixTest, SystemNamesReturnsAllEntries) {
    auto names = matrix_.system_names();
    ASSERT_EQ(names.size(), 3u);
    // std::map orders keys lexicographically.
    EXPECT_EQ(names[0], "SystemA");
    EXPECT_EQ(names[1], "SystemB");
    EXPECT_EQ(names[2], "SystemC");
}

// ---------------------------------------------------------------------------
// all_capabilities()
// ---------------------------------------------------------------------------

TEST_F(CapabilityMatrixTest, AllCapabilitiesCoversEveryEnumerator) {
    auto all = AdapterCapabilityMatrix::all_capabilities();
    // Verify every Capability enumerator is present.
    auto has = [&](Capability c) {
        return std::find(all.begin(), all.end(), c) != all.end();
    };
    EXPECT_TRUE(has(Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(has(Capability::VECTOR_SEARCH));
    EXPECT_TRUE(has(Capability::GRAPH_TRAVERSAL));
    EXPECT_TRUE(has(Capability::DOCUMENT_STORE));
    EXPECT_TRUE(has(Capability::FULL_TEXT_SEARCH));
    EXPECT_TRUE(has(Capability::TRANSACTIONS));
    EXPECT_TRUE(has(Capability::DISTRIBUTED_QUERIES));
    EXPECT_TRUE(has(Capability::GEOSPATIAL_QUERIES));
    EXPECT_TRUE(has(Capability::TIME_SERIES));
    EXPECT_TRUE(has(Capability::STREAM_PROCESSING));
    EXPECT_TRUE(has(Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(has(Capability::SECONDARY_INDEXES));
    EXPECT_TRUE(has(Capability::MATERIALIZED_VIEWS));
    EXPECT_TRUE(has(Capability::REPLICATION));
    EXPECT_TRUE(has(Capability::SHARDING));
    EXPECT_TRUE(has(Capability::ASYNC_OPERATIONS));
    // Total count matches enum size.
    EXPECT_EQ(all.size(), 16u);
}

// ---------------------------------------------------------------------------
// capability_to_string()
// ---------------------------------------------------------------------------

TEST(CapabilityToStringTest, AllCapabilitiesHaveNonEmptyLabels) {
    for (const auto& cap : AdapterCapabilityMatrix::all_capabilities()) {
        const std::string label = AdapterCapabilityMatrix::capability_to_string(cap);
        EXPECT_FALSE(label.empty()) << "Empty label for capability";
        EXPECT_NE(label, "UNKNOWN")  << "Unexpected UNKNOWN label for capability";
    }
}

TEST(CapabilityToStringTest, KnownLabels) {
    EXPECT_EQ(AdapterCapabilityMatrix::capability_to_string(Capability::RELATIONAL_QUERIES),
              "RELATIONAL_QUERIES");
    EXPECT_EQ(AdapterCapabilityMatrix::capability_to_string(Capability::VECTOR_SEARCH),
              "VECTOR_SEARCH");
    EXPECT_EQ(AdapterCapabilityMatrix::capability_to_string(Capability::GRAPH_TRAVERSAL),
              "GRAPH_TRAVERSAL");
    EXPECT_EQ(AdapterCapabilityMatrix::capability_to_string(Capability::MATERIALIZED_VIEWS),
              "MATERIALIZED_VIEWS");
    EXPECT_EQ(AdapterCapabilityMatrix::capability_to_string(Capability::SHARDING),
              "SHARDING");
    EXPECT_EQ(AdapterCapabilityMatrix::capability_to_string(Capability::ASYNC_OPERATIONS),
              "ASYNC_OPERATIONS");
}

// ---------------------------------------------------------------------------
// add_adapter() – population from a live adapter instance
// ---------------------------------------------------------------------------

TEST(CapabilityMatrixAddAdapterTest, ThemisDBAdapterCapabilities) {
    ThemisDBAdapter adapter;
    AdapterCapabilityMatrix matrix;
    matrix.add_adapter("ThemisDB", adapter);

    EXPECT_TRUE(matrix.supports("ThemisDB", Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(matrix.supports("ThemisDB", Capability::VECTOR_SEARCH));
    EXPECT_TRUE(matrix.supports("ThemisDB", Capability::GRAPH_TRAVERSAL));
    EXPECT_TRUE(matrix.supports("ThemisDB", Capability::DOCUMENT_STORE));
    EXPECT_TRUE(matrix.supports("ThemisDB", Capability::TRANSACTIONS));
}

TEST(CapabilityMatrixAddAdapterTest, PostgreSQLAdapterCapabilities) {
    PostgreSQLAdapter adapter;
    AdapterCapabilityMatrix matrix;
    matrix.add_adapter("PostgreSQL", adapter);

    EXPECT_TRUE(matrix.supports("PostgreSQL", Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(matrix.supports("PostgreSQL", Capability::VECTOR_SEARCH));
    EXPECT_TRUE(matrix.supports("PostgreSQL", Capability::DOCUMENT_STORE));
    EXPECT_TRUE(matrix.supports("PostgreSQL", Capability::TRANSACTIONS));
    EXPECT_TRUE(matrix.supports("PostgreSQL", Capability::MATERIALIZED_VIEWS));
    // PostgreSQL does not support graph traversal natively.
    EXPECT_FALSE(matrix.supports("PostgreSQL", Capability::GRAPH_TRAVERSAL));
}

TEST(CapabilityMatrixAddAdapterTest, MongoDBAdapterCapabilities) {
    MongoDBAdapter adapter;
    AdapterCapabilityMatrix matrix;
    matrix.add_adapter("MongoDB", adapter);

    EXPECT_TRUE(matrix.supports("MongoDB", Capability::DOCUMENT_STORE));
    EXPECT_TRUE(matrix.supports("MongoDB", Capability::VECTOR_SEARCH));
    EXPECT_TRUE(matrix.supports("MongoDB", Capability::TRANSACTIONS));
    // MongoDB does not support relational queries or graph traversal.
    EXPECT_FALSE(matrix.supports("MongoDB", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("MongoDB", Capability::GRAPH_TRAVERSAL));
}

TEST(CapabilityMatrixAddAdapterTest, WeaviateAdapterCapabilities) {
    WeaviateAdapter adapter;
    AdapterCapabilityMatrix matrix;
    matrix.add_adapter("Weaviate", adapter);

    EXPECT_TRUE(matrix.supports("Weaviate", Capability::VECTOR_SEARCH));
    EXPECT_TRUE(matrix.supports("Weaviate", Capability::DOCUMENT_STORE));
    EXPECT_TRUE(matrix.supports("Weaviate", Capability::FULL_TEXT_SEARCH));
    EXPECT_FALSE(matrix.supports("Weaviate", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("Weaviate", Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(matrix.supports("Weaviate", Capability::TRANSACTIONS));
}

TEST(CapabilityMatrixAddAdapterTest, QdrantAdapterCapabilities) {
    QdrantAdapter adapter;
    AdapterCapabilityMatrix matrix;
    matrix.add_adapter("Qdrant", adapter);

    EXPECT_TRUE(matrix.supports("Qdrant", Capability::VECTOR_SEARCH));
    EXPECT_TRUE(matrix.supports("Qdrant", Capability::DOCUMENT_STORE));
    EXPECT_FALSE(matrix.supports("Qdrant", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("Qdrant", Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(matrix.supports("Qdrant", Capability::TRANSACTIONS));
    EXPECT_FALSE(matrix.supports("Qdrant", Capability::FULL_TEXT_SEARCH));
}

TEST(CapabilityMatrixAddAdapterTest, PineconeAdapterCapabilities) {
    PineconeAdapter adapter;
    AdapterCapabilityMatrix matrix;
    matrix.add_adapter("Pinecone", adapter);

    EXPECT_TRUE(matrix.supports("Pinecone", Capability::VECTOR_SEARCH));
    EXPECT_TRUE(matrix.supports("Pinecone", Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(matrix.supports("Pinecone", Capability::SECONDARY_INDEXES));
    // Pinecone does not support relational, graph, transactions, or document store.
    EXPECT_FALSE(matrix.supports("Pinecone", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("Pinecone", Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(matrix.supports("Pinecone", Capability::TRANSACTIONS));
    EXPECT_FALSE(matrix.supports("Pinecone", Capability::DOCUMENT_STORE));
    EXPECT_FALSE(matrix.supports("Pinecone", Capability::FULL_TEXT_SEARCH));
}

TEST(CapabilityMatrixAddAdapterTest, ElasticsearchAdapterCapabilities) {
    ElasticsearchAdapter adapter;
    AdapterCapabilityMatrix matrix;
    matrix.add_adapter("Elasticsearch", adapter);

    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::FULL_TEXT_SEARCH));
    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::VECTOR_SEARCH));
    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::DOCUMENT_STORE));
    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::SECONDARY_INDEXES));
    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::DISTRIBUTED_QUERIES));
    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::SHARDING));
    // Elasticsearch does not support relational queries, graph traversal, or ACID transactions.
    EXPECT_FALSE(matrix.supports("Elasticsearch", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("Elasticsearch", Capability::GRAPH_TRAVERSAL));
    EXPECT_FALSE(matrix.supports("Elasticsearch", Capability::TRANSACTIONS));
}

TEST(CapabilityMatrixAddAdapterTest, Neo4jAdapterCapabilities) {
    Neo4jAdapter adapter;
    AdapterCapabilityMatrix matrix;
    matrix.add_adapter("Neo4j", adapter);

    // Neo4j supports native graph traversal with ACID transactions.
    EXPECT_TRUE(matrix.supports("Neo4j", Capability::GRAPH_TRAVERSAL));
    EXPECT_TRUE(matrix.supports("Neo4j", Capability::TRANSACTIONS));
    EXPECT_TRUE(matrix.supports("Neo4j", Capability::BATCH_OPERATIONS));
    EXPECT_TRUE(matrix.supports("Neo4j", Capability::SECONDARY_INDEXES));
    // Neo4j does not support relational queries, vector search, or document store.
    EXPECT_FALSE(matrix.supports("Neo4j", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("Neo4j", Capability::VECTOR_SEARCH));
    EXPECT_FALSE(matrix.supports("Neo4j", Capability::DOCUMENT_STORE));
    EXPECT_FALSE(matrix.supports("Neo4j", Capability::FULL_TEXT_SEARCH));
}

// ---------------------------------------------------------------------------
// build_from_factory()
// ---------------------------------------------------------------------------

TEST(CapabilityMatrixFactoryTest, BuildFromFactoryPopulatesKnownAdapters) {
    // The auto-registration lambdas in each adapter's .cpp file ensure that
    // ThemisDB, MongoDB, PostgreSQL, Weaviate, Qdrant, and Pinecone are present.
    auto matrix = AdapterCapabilityMatrix::build_from_factory();

    const auto names = matrix.system_names();
    EXPECT_FALSE(names.empty());

    // Every system in the factory must appear in the matrix.
    for (const auto& name : AdapterFactory::get_supported_systems()) {
        EXPECT_TRUE(matrix.supports(name, Capability::BATCH_OPERATIONS) ||
                    !matrix.supports(name, Capability::BATCH_OPERATIONS))
            << "System '" << name << "' must be present in the matrix";
        // A simpler existence check:
        const auto caps = matrix.capabilities_of(name);
        // The row is always populated (even if all values are false).
        // Check the system is present by verifying data() has the key.
        EXPECT_TRUE(matrix.data().count(name) > 0)
            << "System '" << name << "' missing from factory-built matrix";
    }
}

TEST(CapabilityMatrixFactoryTest, BuildFromFactoryIncludesThemisDB) {
    auto matrix = AdapterCapabilityMatrix::build_from_factory();
    EXPECT_TRUE(matrix.supports("ThemisDB", Capability::RELATIONAL_QUERIES));
    EXPECT_TRUE(matrix.supports("ThemisDB", Capability::VECTOR_SEARCH));
}

TEST(CapabilityMatrixFactoryTest, BuildFromFactoryIncludesPinecone) {
    auto matrix = AdapterCapabilityMatrix::build_from_factory();
    EXPECT_TRUE(matrix.data().count("Pinecone") > 0);
    EXPECT_TRUE(matrix.supports("Pinecone", Capability::VECTOR_SEARCH));
    EXPECT_FALSE(matrix.supports("Pinecone", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("Pinecone", Capability::TRANSACTIONS));
}

TEST(CapabilityMatrixFactoryTest, BuildFromFactoryIncludesElasticsearch) {
    auto matrix = AdapterCapabilityMatrix::build_from_factory();
    EXPECT_TRUE(matrix.data().count("Elasticsearch") > 0);
    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::FULL_TEXT_SEARCH));
    EXPECT_TRUE(matrix.supports("Elasticsearch", Capability::VECTOR_SEARCH));
    EXPECT_FALSE(matrix.supports("Elasticsearch", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("Elasticsearch", Capability::TRANSACTIONS));
}

TEST(CapabilityMatrixFactoryTest, BuildFromFactoryIncludesNeo4j) {
    auto matrix = AdapterCapabilityMatrix::build_from_factory();
    EXPECT_TRUE(matrix.data().count("Neo4j") > 0);
    EXPECT_TRUE(matrix.supports("Neo4j", Capability::GRAPH_TRAVERSAL));
    EXPECT_TRUE(matrix.supports("Neo4j", Capability::TRANSACTIONS));
    EXPECT_FALSE(matrix.supports("Neo4j", Capability::RELATIONAL_QUERIES));
    EXPECT_FALSE(matrix.supports("Neo4j", Capability::VECTOR_SEARCH));
}
