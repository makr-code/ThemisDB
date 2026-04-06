/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_usage.cpp                                  ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:03:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     315                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0 OR MIT
// Copyright (c) 2026 CHIMERA Suite Contributors
//
// Example: Basic CRUD operations with CHIMERA adapters

#include "chimera/database_adapter.hpp"
// Include specific adapter implementations as needed:
// #include "chimera/themisdb_adapter.hpp"
// #include "chimera/postgresql_adapter.hpp"
// #include "chimera/weaviate_adapter.hpp"

#include <iostream>
#include <memory>

using namespace chimera;

int main() {
    // =========================================================================
    // Example 1: Basic Connection and CRUD
    // =========================================================================
    
    std::cout << "=== Example 1: Basic CRUD Operations ===" << std::endl;
    
    // Create adapter (replace with actual adapter factory)
    // auto adapter = adapters::create_postgresql_adapter();
    std::unique_ptr<DatabaseAdapter> adapter; // Placeholder
    
    // Configure connection
    ConnectionConfig config;
    config.host = "localhost";
    config.port = 5432;
    config.database = "example_db";
    config.username = "user";
    config.password = "password";
    config.timeout_ms = 30000;
    config.pool_size = 10;
    
    // Connect
    auto status = adapter->connect(config);
    if (!status.ok()) {
        std::cerr << "Connection failed: " << status.message << std::endl;
        return 1;
    }
    std::cout << "Connected successfully" << std::endl;
    
    // Create a collection
    status = adapter->create_collection("users");
    if (!status.ok()) {
        std::cerr << "Create collection failed: " << status.message << std::endl;
    }
    
    // Insert a document
    Document user;
    user["id"] = std::string("user_001");
    user["name"] = std::string("Alice Smith");
    user["age"] = int64_t(30);
    user["email"] = std::string("alice@example.com");
    user["active"] = true;
    
    status = adapter->insert("users", user);
    if (status.ok()) {
        std::cout << "Inserted user successfully" << std::endl;
    } else {
        std::cerr << "Insert failed: " << status.message << std::endl;
    }
    
    // Find by ID
    Document found_user;
    status = adapter->find_by_id("users", "user_001", found_user);
    if (status.ok()) {
        std::cout << "Found user: " 
                  << std::get<std::string>(found_user["name"]) << std::endl;
    }
    
    // Update
    Document filter;
    filter["id"] = std::string("user_001");
    
    Document update;
    update["age"] = int64_t(31);
    
    size_t updated_count = 0;
    status = adapter->update("users", filter, update, updated_count);
    if (status.ok()) {
        std::cout << "Updated " << updated_count << " document(s)" << std::endl;
    }
    
    // Delete
    size_t deleted_count = 0;
    status = adapter->remove("users", filter, deleted_count);
    if (status.ok()) {
        std::cout << "Deleted " << deleted_count << " document(s)" << std::endl;
    }
    
    // =========================================================================
    // Example 2: Batch Operations
    // =========================================================================
    
    std::cout << "\n=== Example 2: Batch Operations ===" << std::endl;
    
    // Prepare multiple documents
    std::vector<Document> users;
    for (int i = 0; i < 100; ++i) {
        Document user;
        user["id"] = std::string("user_") + std::to_string(i);
        user["name"] = std::string("User ") + std::to_string(i);
        user["age"] = int64_t(20 + (i % 50));
        users.push_back(user);
    }
    
    // Batch insert
    status = adapter->insert_batch("users", users);
    if (status.ok()) {
        std::cout << "Batch inserted " << users.size() << " users" << std::endl;
    }
    
    // Count documents
    size_t count = 0;
    status = adapter->count("users", {}, count);
    if (status.ok()) {
        std::cout << "Total users: " << count << std::endl;
    }
    
    // =========================================================================
    // Example 3: Query with Options
    // =========================================================================
    
    std::cout << "\n=== Example 3: Query with Options ===" << std::endl;
    
    QueryOptions options;
    options.limit = 10;
    options.offset = 5;
    options.projection = {"id", "name", "age"};
    options.timeout_ms = 5000;
    
    ResultSet results;
    status = adapter->find("users", {}, results, options);
    if (status.ok()) {
        std::cout << "Found " << results.size() << " users (limited)" << std::endl;
        for (const auto& user : results) {
            std::cout << "  - " << std::get<std::string>(user.at("name")) 
                      << " (age " << std::get<int64_t>(user.at("age")) << ")"
                      << std::endl;
        }
    }
    
    // =========================================================================
    // Example 4: Vector Search (if supported)
    // =========================================================================
    
    std::cout << "\n=== Example 4: Vector Search ===" << std::endl;
    
    if (adapter->supports_vector_search()) {
        std::cout << "Vector search is supported" << std::endl;
        
        // Create a collection for vectors
        adapter->create_collection("embeddings");
        
        // Insert documents with vectors
        Document doc1;
        doc1["id"] = std::string("doc_1");
        doc1["text"] = std::string("Machine learning is fascinating");
        doc1["embedding"] = std::vector<float>{0.1f, 0.2f, 0.3f, 0.4f};
        adapter->insert("embeddings", doc1);
        
        Document doc2;
        doc2["id"] = std::string("doc_2");
        doc2["text"] = std::string("Deep learning advances AI");
        doc2["embedding"] = std::vector<float>{0.15f, 0.25f, 0.35f, 0.45f};
        adapter->insert("embeddings", doc2);
        
        // Perform vector search
        VectorSearchParams search_params;
        search_params.query_vector = {0.12f, 0.22f, 0.32f, 0.42f};
        search_params.k = 5;
        search_params.metric = "cosine";
        
        ResultSet vector_results;
        status = adapter->vector_search(
            "embeddings", 
            "embedding", 
            search_params, 
            vector_results
        );
        
        if (status.ok()) {
            std::cout << "Found " << vector_results.size() 
                      << " similar documents:" << std::endl;
            for (const auto& doc : vector_results) {
                std::cout << "  - " << std::get<std::string>(doc.at("text")) 
                          << std::endl;
            }
        }
    } else {
        std::cout << "Vector search not supported by this adapter" << std::endl;
    }
    
    // =========================================================================
    // Example 5: Transactions (if supported)
    // =========================================================================
    
    std::cout << "\n=== Example 5: Transactions ===" << std::endl;
    
    if (adapter->supports_transactions()) {
        std::cout << "Transactions are supported" << std::endl;
        
        // Begin transaction
        std::unique_ptr<Transaction> txn;
        status = adapter->begin_transaction(txn, IsolationLevel::READ_COMMITTED);
        
        if (status.ok() && txn) {
            std::cout << "Transaction started" << std::endl;
            
            // Perform operations within transaction
            Document txn_user;
            txn_user["id"] = std::string("txn_user_1");
            txn_user["name"] = std::string("Transaction User");
            
            QueryOptions txn_options;
            txn_options.transaction = txn.get();
            
            adapter->insert("users", txn_user, txn_options);
            
            // Commit transaction
            status = txn->commit();
            if (status.ok()) {
                std::cout << "Transaction committed successfully" << std::endl;
            }
        }
    } else {
        std::cout << "Transactions not supported by this adapter" << std::endl;
    }
    
    // =========================================================================
    // Example 6: Native Queries
    // =========================================================================
    
    std::cout << "\n=== Example 6: Native Queries ===" << std::endl;
    
    // Execute native query (SQL, AQL, Cypher, etc. depending on database)
    std::string query = "SELECT name, age FROM users WHERE age > 25 LIMIT 10";
    
    ResultSet query_results;
    status = adapter->execute_query(query, query_results);
    if (status.ok()) {
        std::cout << "Query returned " << query_results.size() 
                  << " results" << std::endl;
    }
    
    // Execute parameterized query
    std::string param_query = "SELECT * FROM users WHERE age > $1 AND age < $2";
    std::map<std::string, Value> params;
    params["1"] = int64_t(25);
    params["2"] = int64_t(40);
    
    status = adapter->execute_query_params(param_query, params, query_results);
    if (status.ok()) {
        std::cout << "Parameterized query returned " << query_results.size() 
                  << " results" << std::endl;
    }
    
    // =========================================================================
    // Example 7: Metadata and Capabilities
    // =========================================================================
    
    std::cout << "\n=== Example 7: Metadata ===" << std::endl;
    
    std::cout << "Adapter: " << adapter->get_adapter_info() << std::endl;
    std::cout << "Database version: " << adapter->get_database_version() << std::endl;
    
    auto capabilities = adapter->get_capabilities();
    std::cout << "Capabilities:" << std::endl;
    for (const auto& cap : capabilities) {
        std::cout << "  - " << cap << std::endl;
    }
    
    // =========================================================================
    // Cleanup
    // =========================================================================
    
    std::cout << "\n=== Cleanup ===" << std::endl;
    
    // Drop test collection
    adapter->drop_collection("users");
    adapter->drop_collection("embeddings");
    
    // Disconnect
    adapter->disconnect();
    std::cout << "Disconnected" << std::endl;
    
    return 0;
}
