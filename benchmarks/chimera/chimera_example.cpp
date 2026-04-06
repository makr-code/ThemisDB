/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chimera_example.cpp                                ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:04:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     337                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file chimera_example.cpp
 * @brief Example usage of CHIMERA adapter architecture
 * 
 * This example demonstrates how to:
 * 1. Register database adapters
 * 2. Create adapter instances
 * 3. Query system capabilities
 * 4. Perform basic database operations
 * 
 * @copyright MIT License
 */

#include "chimera/database_adapter.hpp"
#include "chimera/themisdb_adapter.hpp"
#include <iostream>
#include <iomanip>

using namespace chimera;

/**
 * @brief Print system capabilities
 */
void print_capabilities(IDatabaseAdapter* adapter) {
    std::cout << "\n=== System Capabilities ===\n";
    
    auto info_result = adapter->get_system_info();
    if (info_result.is_ok()) {
        auto info = info_result.value.value();
        std::cout << "System: " << info.system_name << "\n";
        std::cout << "Version: " << info.version << "\n";
    }
    
    auto caps = adapter->get_capabilities();
    std::cout << "\nSupported features:\n";
    for (const auto& cap : caps) {
        switch (cap) {
            case Capability::RELATIONAL_QUERIES:
                std::cout << "  ✓ Relational Queries\n";
                break;
            case Capability::VECTOR_SEARCH:
                std::cout << "  ✓ Vector Search\n";
                break;
            case Capability::GRAPH_TRAVERSAL:
                std::cout << "  ✓ Graph Traversal\n";
                break;
            case Capability::DOCUMENT_STORE:
                std::cout << "  ✓ Document Store\n";
                break;
            case Capability::FULL_TEXT_SEARCH:
                std::cout << "  ✓ Full-Text Search\n";
                break;
            case Capability::TRANSACTIONS:
                std::cout << "  ✓ ACID Transactions\n";
                break;
            case Capability::DISTRIBUTED_QUERIES:
                std::cout << "  ✓ Distributed Queries\n";
                break;
            case Capability::GEOSPATIAL_QUERIES:
                std::cout << "  ✓ Geospatial Queries\n";
                break;
            case Capability::TIME_SERIES:
                std::cout << "  ✓ Time-Series\n";
                break;
            case Capability::BATCH_OPERATIONS:
                std::cout << "  ✓ Batch Operations\n";
                break;
            case Capability::SECONDARY_INDEXES:
                std::cout << "  ✓ Secondary Indexes\n";
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Example: Relational operations
 */
void example_relational(IDatabaseAdapter* adapter) {
    std::cout << "\n=== Relational Example ===\n";
    
    if (!adapter->has_capability(Capability::RELATIONAL_QUERIES)) {
        std::cout << "Relational queries not supported\n";
        return;
    }
    
    // Insert data
    RelationalRow row;
    row.columns["name"] = Scalar{std::string{"Alice"}};
    row.columns["age"] = Scalar{int64_t{30}};
    row.columns["active"] = Scalar{true};
    
    auto insert_result = adapter->insert_row("users", row);
    if (insert_result.is_ok()) {
        std::cout << "✓ Inserted " << insert_result.value.value() << " row(s)\n";
    } else {
        std::cout << "✗ Insert failed: " << insert_result.error_message << "\n";
    }
    
    // Query data
    auto query_result = adapter->execute_query("SELECT * FROM users WHERE age > 25");
    if (query_result.is_ok()) {
        std::cout << "✓ Query executed successfully\n";
    } else {
        std::cout << "✗ Query failed: " << query_result.error_message << "\n";
    }
}

/**
 * @brief Example: Vector operations
 */
void example_vector(IDatabaseAdapter* adapter) {
    std::cout << "\n=== Vector Example ===\n";
    
    if (!adapter->has_capability(Capability::VECTOR_SEARCH)) {
        std::cout << "Vector search not supported\n";
        return;
    }
    
    // Create vector index
    auto index_result = adapter->create_index("embeddings", 128);
    if (index_result.is_ok()) {
        std::cout << "✓ Created vector index (128 dimensions)\n";
    }
    
    // Insert vector
    Vector vec;
    vec.data.resize(128);
    for (size_t i = 0; i < 128; ++i) {
        vec.data[i] = static_cast<float>(i) / 128.0f;
    }
    vec.metadata["category"] = Scalar{std::string{"test"}};
    
    auto vec_result = adapter->insert_vector("embeddings", vec);
    if (vec_result.is_ok()) {
        std::cout << "✓ Inserted vector with ID: " << vec_result.value.value() << "\n";
    }
    
    // Search for similar vectors
    Vector query_vec;
    query_vec.data.resize(128);
    for (size_t i = 0; i < 128; ++i) {
        query_vec.data[i] = static_cast<float>(i + 1) / 128.0f;
    }
    
    auto search_result = adapter->search_vectors("embeddings", query_vec, 5);
    if (search_result.is_ok()) {
        std::cout << "✓ Vector search completed\n";
        std::cout << "  Found " << search_result.value.value().size() << " similar vectors\n";
    }
}

/**
 * @brief Example: Graph operations
 */
void example_graph(IDatabaseAdapter* adapter) {
    std::cout << "\n=== Graph Example ===\n";
    
    if (!adapter->has_capability(Capability::GRAPH_TRAVERSAL)) {
        std::cout << "Graph traversal not supported\n";
        return;
    }
    
    // Create nodes
    GraphNode alice;
    alice.id = "person_alice";
    alice.label = "Person";
    alice.properties["name"] = Scalar{std::string{"Alice"}};
    alice.properties["age"] = Scalar{int64_t{30}};
    
    GraphNode bob;
    bob.id = "person_bob";
    bob.label = "Person";
    bob.properties["name"] = Scalar{std::string{"Bob"}};
    bob.properties["age"] = Scalar{int64_t{35}};
    
    adapter->insert_node(alice);
    adapter->insert_node(bob);
    std::cout << "✓ Inserted 2 nodes\n";
    
    // Create edge
    GraphEdge friendship;
    friendship.source_id = "person_alice";
    friendship.target_id = "person_bob";
    friendship.label = "KNOWS";
    friendship.weight = 1.0;
    
    adapter->insert_edge(friendship);
    std::cout << "✓ Inserted 1 edge\n";
    
    // Find shortest path
    auto path_result = adapter->shortest_path("person_alice", "person_bob");
    if (path_result.is_ok()) {
        std::cout << "✓ Found shortest path\n";
    }
}

/**
 * @brief Example: Document operations
 */
void example_document(IDatabaseAdapter* adapter) {
    std::cout << "\n=== Document Example ===\n";
    
    if (!adapter->has_capability(Capability::DOCUMENT_STORE)) {
        std::cout << "Document store not supported\n";
        return;
    }
    
    // Create document
    Document doc;
    doc.id = "doc_001";
    doc.fields["title"] = Scalar{std::string{"CHIMERA Documentation"}};
    doc.fields["author"] = Scalar{std::string{"Development Team"}};
    doc.fields["views"] = Scalar{int64_t{1000}};
    doc.fields["published"] = Scalar{true};
    
    auto insert_result = adapter->insert_document("articles", doc);
    if (insert_result.is_ok()) {
        std::cout << "✓ Inserted document with ID: " << insert_result.value.value() << "\n";
    }
    
    // Find documents
    std::map<std::string, Scalar> filter;
    filter["published"] = Scalar{true};
    
    auto find_result = adapter->find_documents("articles", filter, 10);
    if (find_result.is_ok()) {
        std::cout << "✓ Found " << find_result.value.value().size() << " documents\n";
    }
}

/**
 * @brief Example: Transaction operations
 */
void example_transaction(IDatabaseAdapter* adapter) {
    std::cout << "\n=== Transaction Example ===\n";
    
    if (!adapter->has_capability(Capability::TRANSACTIONS)) {
        std::cout << "Transactions not supported\n";
        return;
    }
    
    // Begin transaction
    TransactionOptions opts;
    opts.isolation_level = TransactionOptions::IsolationLevel::SERIALIZABLE;
    opts.timeout = std::chrono::milliseconds(5000);
    
    auto txn_result = adapter->begin_transaction(opts);
    if (txn_result.is_ok()) {
        std::string txn_id = txn_result.value.value();
        std::cout << "✓ Started transaction: " << txn_id << "\n";
        
        // Perform operations...
        
        // Commit
        auto commit_result = adapter->commit_transaction(txn_id);
        if (commit_result.is_ok()) {
            std::cout << "✓ Transaction committed\n";
        }
    }
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "CHIMERA Adapter Architecture - Example Program\n";
    std::cout << "=================================================\n";
    
    // Register adapters
    AdapterFactory::register_adapter("ThemisDB",
        []() { return std::make_unique<ThemisDBAdapter>(); });
    
    // List supported systems
    auto systems = AdapterFactory::get_supported_systems();
    std::cout << "\nSupported database systems:\n";
    for (const auto& sys : systems) {
        std::cout << "  • " << sys << "\n";
    }
    
    // Create adapter
    auto adapter = AdapterFactory::create("ThemisDB");
    if (!adapter) {
        std::cerr << "Failed to create adapter\n";
        return 1;
    }
    
    // Connect
    auto conn_result = adapter->connect("themisdb://localhost:7777");
    if (!conn_result.is_ok()) {
        std::cerr << "Connection failed: " << conn_result.error_message << "\n";
        return 1;
    }
    std::cout << "\n✓ Connected to ThemisDB\n";
    
    // Show capabilities
    print_capabilities(adapter.get());
    
    // Run examples
    example_relational(adapter.get());
    example_vector(adapter.get());
    example_graph(adapter.get());
    example_document(adapter.get());
    example_transaction(adapter.get());
    
    // Disconnect
    adapter->disconnect();
    std::cout << "\n✓ Disconnected\n";
    
    std::cout << "\n=================================================\n";
    std::cout << "Example completed successfully!\n";
    std::cout << "=================================================\n";
    
    return 0;
}
