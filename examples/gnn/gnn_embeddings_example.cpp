/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gnn_embeddings_example.cpp                         ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     347                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file gnn_embeddings_example.cpp
 * @brief Example: Using GNN Embeddings for Graph Similarity Search
 * 
 * This example demonstrates how to use the GNNEmbeddingManager to:
 * - Register GNN models
 * - Generate node embeddings
 * - Find similar nodes based on embeddings
 * - Use embeddings for recommendations
 * 
 * Scenario: Social network with user profiles
 * Use case: Find similar users for friend recommendations
 */

#include "index/gnn_embeddings.h"
#include "index/property_graph.h"
#include "index/vector_index_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <iostream>
#include <vector>
#include <string>

using namespace themis;

/**
 * Create a sample social network graph.
 * 
 * Structure:
 *   Alice (age: 25, interests: tech, music)
 *     -> FOLLOWS -> Bob
 *     -> FOLLOWS -> Charlie
 *   
 *   Bob (age: 30, interests: sports, music)
 *     -> FOLLOWS -> Alice
 *     -> FOLLOWS -> David
 *   
 *   Charlie (age: 28, interests: tech, art)
 *     -> FOLLOWS -> Alice
 *     -> FOLLOWS -> David
 *   
 *   David (age: 35, interests: sports, tech)
 *     -> FOLLOWS -> Bob
 *     -> FOLLOWS -> Charlie
 */
void createSampleGraph(
    PropertyGraphManager& pgm,
    const std::string& graph_id
) {
    std::cout << "\n[1] Creating sample social network graph..." << std::endl;
    
    // Create nodes (users)
    std::vector<std::pair<std::string, BaseEntity>> users = {
        {"alice", BaseEntity("alice")},
        {"bob", BaseEntity("bob")},
        {"charlie", BaseEntity("charlie")},
        {"david", BaseEntity("david")}
    };
    
    // Set user attributes
    users[0].second.setField("name", "Alice");
    users[0].second.setField("age", 25);
    users[0].second.setField("interests", "tech,music");
    users[0].second.setField("activity_score", 0.85);
    
    users[1].second.setField("name", "Bob");
    users[1].second.setField("age", 30);
    users[1].second.setField("interests", "sports,music");
    users[1].second.setField("activity_score", 0.65);
    
    users[2].second.setField("name", "Charlie");
    users[2].second.setField("age", 28);
    users[2].second.setField("interests", "tech,art");
    users[2].second.setField("activity_score", 0.90);
    
    users[3].second.setField("name", "David");
    users[3].second.setField("age", 35);
    users[3].second.setField("interests", "sports,tech");
    users[3].second.setField("activity_score", 0.70);
    
    // Add nodes to graph
    for (const auto& [id, entity] : users) {
        auto st = pgm.addNode(id, "Person", graph_id);
        if (!st.ok) {
            std::cerr << "Failed to add node " << id << ": " << st.message << std::endl;
        } else {
            std::cout << "  ✓ Added user: " << entity.getFieldAsString("name").value_or(id) << std::endl;
        }
    }
    
    // Create edges (follows relationships)
    std::vector<std::tuple<std::string, std::string, std::string>> edges = {
        {"alice", "bob", "FOLLOWS"},
        {"alice", "charlie", "FOLLOWS"},
        {"bob", "alice", "FOLLOWS"},
        {"bob", "david", "FOLLOWS"},
        {"charlie", "alice", "FOLLOWS"},
        {"charlie", "david", "FOLLOWS"},
        {"david", "bob", "FOLLOWS"},
        {"david", "charlie", "FOLLOWS"}
    };
    
    for (const auto& [from, to, type] : edges) {
        auto st = pgm.addEdge(from, to, type, graph_id);
        if (!st.ok) {
            std::cerr << "Failed to add edge " << from << " -> " << to << std::endl;
        }
    }
    
    std::cout << "  ✓ Created " << edges.size() << " relationships" << std::endl;
    std::cout << "  Graph: " << users.size() << " nodes, " << edges.size() << " edges" << std::endl;
}

/**
 * Demonstrate GNN model registration and embedding generation.
 */
void demonstrateEmbeddingGeneration(
    GNNEmbeddingManager& gnn,
    const std::string& graph_id
) {
    std::cout << "\n[2] Registering GNN model..." << std::endl;
    
    // Register a GraphSAGE model (MVP: uses simple feature-based embeddings)
    auto st = gnn.registerModel(
        "social_graphsage",  // model name
        "graphsage",         // model type
        128,                 // embedding dimension
        "{}"                 // config (empty for MVP)
    );
    
    if (!st.ok) {
        std::cerr << "Failed to register model: " << st.message << std::endl;
        return;
    }
    
    std::cout << "  ✓ Registered model: social_graphsage (GraphSAGE, 128-dim)" << std::endl;
    
    // Generate embeddings for all Person nodes
    std::cout << "\n[3] Generating node embeddings..." << std::endl;
    
    st = gnn.generateNodeEmbeddings(
        graph_id,           // target graph
        "Person",           // node label
        "social_graphsage"  // model name
    );
    
    if (!st.ok) {
        std::cerr << "Failed to generate embeddings: " << st.message << std::endl;
        return;
    }
    
    std::cout << "  ✓ Generated embeddings for all Person nodes" << std::endl;
    
    // Retrieve and display an embedding
    auto [st2, emb_info] = gnn.getNodeEmbedding("alice", graph_id, "social_graphsage");
    if (st2.ok) {
        std::cout << "  ✓ Alice's embedding: " << emb_info.embedding.size() << "-dimensional vector" << std::endl;
        std::cout << "    First 5 values: [";
        for (size_t i = 0; i < std::min<size_t>(5, emb_info.embedding.size()); ++i) {
            std::cout << emb_info.embedding[i];
            if (i < 4) {
              std::cout << ", ";
            }
        }
        std::cout << ", ...]" << std::endl;
    }
}

/**
 * Demonstrate similarity search using embeddings.
 */
void demonstrateSimilaritySearch(
    GNNEmbeddingManager& gnn,
    const std::string& graph_id
) {
    std::cout << "\n[4] Finding similar users (friend recommendations)..." << std::endl;
    
    // Find users similar to Alice
    auto [st, similar] = gnn.findSimilarNodes(
        "alice",            // query node
        graph_id,           // target graph
        3,                  // top-3 results
        "social_graphsage"  // model name
    );
    
    if (!st.ok) {
        std::cerr << "Failed to find similar nodes: " << st.message << std::endl;
        return;
    }
    
    std::cout << "  Users most similar to Alice:" << std::endl;
    for (const auto& result : similar) {
        std::cout << "    - " << result.entity_id 
                  << " (similarity: " << result.similarity << ")" << std::endl;
    }
    
    // Find users similar to Bob
    auto [st2, similar2] = gnn.findSimilarNodes("bob", graph_id, 3, "social_graphsage");
    if (st2.ok) {
        std::cout << "\n  Users most similar to Bob:" << std::endl;
        for (const auto& result : similar2) {
            std::cout << "    - " << result.entity_id 
                      << " (similarity: " << result.similarity << ")" << std::endl;
        }
    }
}

/**
 * Demonstrate incremental embedding updates.
 */
void demonstrateIncrementalUpdate(
    GNNEmbeddingManager& gnn,
    PropertyGraphManager& pgm,
    const std::string& graph_id
) {
    std::cout << "\n[5] Demonstrating incremental embedding update..." << std::endl;
    
    // Add a new user
    BaseEntity newUser("eve");
    newUser.setField("name", "Eve");
    newUser.setField("age", 27);
    newUser.setField("interests", "tech,music,art");
    newUser.setField("activity_score", 0.88);
    
    auto st = pgm.addNode("eve", "Person", graph_id);
    if (!st.ok) {
        std::cerr << "Failed to add new user: " << st.message << std::endl;
        return;
    }
    
    std::cout << "  ✓ Added new user: Eve" << std::endl;
    
    // Add relationships
    pgm.addEdge("eve", "alice", "FOLLOWS", graph_id);
    pgm.addEdge("eve", "charlie", "FOLLOWS", graph_id);
    pgm.addEdge("alice", "eve", "FOLLOWS", graph_id);
    
    std::cout << "  ✓ Created relationships for Eve" << std::endl;
    
    // Update embedding for the new user
    st = gnn.updateNodeEmbedding("eve", graph_id, "social_graphsage");
    if (!st.ok) {
        std::cerr << "Failed to update embedding: " << st.message << std::endl;
        return;
    }
    
    std::cout << "  ✓ Generated embedding for Eve" << std::endl;
    
    // Find similar users for Eve
    auto [st2, similar] = gnn.findSimilarNodes("eve", graph_id, 3, "social_graphsage");
    if (st2.ok) {
        std::cout << "\n  Users most similar to Eve:" << std::endl;
        for (const auto& result : similar) {
            std::cout << "    - " << result.entity_id 
                      << " (similarity: " << result.similarity << ")" << std::endl;
        }
        std::cout << "\n  → Recommendation: Eve should connect with these users!" << std::endl;
    }
}

/**
 * Display statistics about embeddings.
 */
void displayStatistics(GNNEmbeddingManager& gnn) {
    std::cout << "\n[6] Embedding statistics..." << std::endl;
    
    auto [st, stats] = gnn.getStats();
    if (!st.ok) {
        std::cerr << "Failed to get statistics: " << st.message << std::endl;
        return;
    }
    
    std::cout << "  Total node embeddings: " << stats.total_node_embeddings << std::endl;
    std::cout << "  Total edge embeddings: " << stats.total_edge_embeddings << std::endl;
    
    std::cout << "\n  Embeddings per model:" << std::endl;
    for (const auto& [model, count] : stats.embeddings_per_model) {
        std::cout << "    " << model << ": " << count << std::endl;
    }
    
    std::cout << "\n  Embeddings per graph:" << std::endl;
    for (const auto& [graph, count] : stats.embeddings_per_graph) {
        std::cout << "    " << graph << ": " << count << std::endl;
    }
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "GNN Embeddings Example for ThemisDB" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        // Initialize database
        RocksDBWrapper db;
        auto st = db.open("./gnn_example_db");
        if (!st.ok) {
            std::cerr << "Failed to open database: " << st.message << std::endl;
            return 1;
        }
        
        // Initialize managers
        PropertyGraphManager pgm(db);
        VectorIndexManager vim(db);
        GNNEmbeddingManager gnn(db, pgm, vim);
        
        const std::string graph_id = "social_network";
        
        // Run demonstrations
        createSampleGraph(pgm, graph_id);
        demonstrateEmbeddingGeneration(gnn, graph_id);
        demonstrateSimilaritySearch(gnn, graph_id);
        demonstrateIncrementalUpdate(gnn, pgm, graph_id);
        displayStatistics(gnn);
        
        std::cout << "\n=====================================" << std::endl;
        std::cout << "Example complete!" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        // Cleanup
        db.close();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
