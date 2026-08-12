// Tests for GNN Embedding Manager

#include <gtest/gtest.h>
#include "index/gnn_embeddings.h"
#include "index/property_graph.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <cmath>
#include <filesystem>

using namespace themis;

class GNNEmbeddingTest : public ::testing::Test {
protected:
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<PropertyGraphManager> pgm;
    std::unique_ptr<VectorIndexManager> vim;
    std::unique_ptr<GNNEmbeddingManager> gem;
    std::string testDbPath = "data/themis_gnn_test";
    
    void SetUp() override {
        // Clean up existing test database
        if (std::filesystem::exists(testDbPath)) {
            std::filesystem::remove_all(testDbPath);
        }
        
        // Initialize components
        RocksDBWrapper::Config config;
        config.db_path = testDbPath;
        db = std::make_unique<RocksDBWrapper>(config);
        db->open();
        
        pgm = std::make_unique<PropertyGraphManager>(*db);
        vim = std::make_unique<VectorIndexManager>(*db);
        vim->init("embedding", 64);  // 64-dim embeddings
        
        gem = std::make_unique<GNNEmbeddingManager>(*db, *pgm, *vim);
        
        // Register default model
        gem->registerModel("test_model", "feature_based", 64);
    }
    
    void TearDown() override {
        gem.reset();
        vim.reset();
        pgm.reset();
        db.reset();
        
        if (std::filesystem::exists(testDbPath)) {
            std::filesystem::remove_all(testDbPath);
        }
    }
    
    void createTestGraph() {
        // Create Person nodes with features
        BaseEntity person1("person1");
        person1.setField("id", std::string("person1"));  // Required by PropertyGraphManager
        person1.setField("age", 30);
        person1.setField("score", 85.5);
        
        BaseEntity person2("person2");
        person2.setField("id", std::string("person2"));
        person2.setField("age", 25);
        person2.setField("score", 90.0);
        
        BaseEntity person3("person3");
        person3.setField("id", std::string("person3"));
        person3.setField("age", 35);
        person3.setField("score", 78.0);
        
        // Add nodes to property graph
        pgm->addNode(person1, "g1");
        pgm->addNodeLabel("person1", "Person", "g1");
        
        pgm->addNode(person2, "g1");
        pgm->addNodeLabel("person2", "Person", "g1");
        
        pgm->addNode(person3, "g1");
        pgm->addNodeLabel("person3", "Person", "g1");
        
        // Create relationship edges (with _from/_to fields)
        BaseEntity edge1("edge1");
        edge1.setField("id", std::string("edge1"));  // Required
        edge1.setField("_from", std::string("person1"));
        edge1.setField("_to", std::string("person2"));
        edge1.setField("_type", std::string("knows"));
        
        BaseEntity edge2("edge2");
        edge2.setField("id", std::string("edge2"));
        edge2.setField("_from", std::string("person2"));
        edge2.setField("_to", std::string("person3"));
        edge2.setField("_type", std::string("knows"));
        
        pgm->addEdge(edge1, "g1");
        pgm->addEdge(edge2, "g1");
    }
};

TEST_F(GNNEmbeddingTest, RegisterModel) {
    auto st = gem->registerModel("gnn_model", "GraphSAGE", 128, "{\"layers\": 2}");
    EXPECT_TRUE(st.ok);
    
    auto [status_models, models] = gem->listModels();
    EXPECT_TRUE(status_models.ok);
    EXPECT_GE(models.size(), 2);  // test_model + gnn_model
    
    auto [status_model_info, modelInfo] = gem->getModelInfo("gnn_model");
    EXPECT_TRUE(status_model_info.ok);
    EXPECT_EQ(modelInfo.name, "gnn_model");
    EXPECT_EQ(modelInfo.type, "GraphSAGE");
    EXPECT_EQ(modelInfo.embedding_dim, 128);
}

TEST_F(GNNEmbeddingTest, GenerateNodeEmbeddings) {
    createTestGraph();
    
    // Generate embeddings for all Person nodes
    auto st = gem->generateNodeEmbeddings("g1", "Person", "test_model");
    EXPECT_TRUE(st.ok);
    
    // Verify embeddings were created
    auto [status_embedding, embInfo] = gem->getNodeEmbedding("person1", "g1", "test_model");
    EXPECT_TRUE(status_embedding.ok);
    EXPECT_EQ(embInfo.entity_id, "person1");
    EXPECT_EQ(embInfo.entity_type, "node");
    EXPECT_EQ(embInfo.model_name, "test_model");
    EXPECT_EQ(embInfo.embedding.size(), 64);  // test_model dim
}

TEST_F(GNNEmbeddingTest, UpdateNodeEmbedding) {
    createTestGraph();
    
    // Update single node embedding
    auto st = gem->updateNodeEmbedding("person1", "g1", "test_model");
    EXPECT_TRUE(st.ok);
    
    // Verify embedding
    auto [status_embedding, embInfo] = gem->getNodeEmbedding("person1", "g1", "test_model");
    EXPECT_TRUE(status_embedding.ok);
    EXPECT_FALSE(embInfo.embedding.empty());
    
    // Verify embedding is normalized
    float norm = 0.0f;
    for (float val : embInfo.embedding) {
        norm += val * val;
    }
    EXPECT_NEAR(std::sqrt(norm), 1.0f, 0.01f);
}

TEST_F(GNNEmbeddingTest, GenerateEdgeEmbeddings) {
    createTestGraph();
    
    // Generate embeddings for all "knows" edges
    auto st = gem->generateEdgeEmbeddings("g1", "knows", "test_model");
    EXPECT_TRUE(st.ok);
    
    // Verify edge embeddings
    auto [status_edges, edges] = pgm->getEdgesByType("knows", "g1");
    EXPECT_TRUE(status_edges.ok);
    EXPECT_GE(edges.size(), 2);
    
    if (!edges.empty()) {
        auto [status_edge_embedding, embInfo] = gem->getEdgeEmbedding(edges[0].edgeId, "g1", "test_model");
        EXPECT_TRUE(status_edge_embedding.ok);
        EXPECT_EQ(embInfo.entity_type, "edge");
    }
}

TEST_F(GNNEmbeddingTest, FindSimilarNodes) {
    createTestGraph();
    
    // Generate embeddings
    gem->generateNodeEmbeddings("g1", "Person", "test_model");
    
    // Find similar nodes to person1
    auto [status_similar, similar] = gem->findSimilarNodes("person1", "g1", 2, "test_model");
    EXPECT_TRUE(status_similar.ok);
    
    // Should find person2 and person3 (excluding person1 itself)
    EXPECT_LE(similar.size(), 2);
    
    // Verify similarity scores are in valid range
    for (const auto& res : similar) {
        EXPECT_GE(res.similarity, 0.0f);
        EXPECT_LE(res.similarity, 1.0f);
        EXPECT_NE(res.entity_id, "person1");  // Should not include query node
    }
}

TEST_F(GNNEmbeddingTest, FindSimilarEdges) {
    createTestGraph();
    
    // Generate edge embeddings
    gem->generateEdgeEmbeddings("g1", "knows", "test_model");
    
    // Get first edge
    auto [status_edges, edges] = pgm->getEdgesByType("knows", "g1");
    ASSERT_TRUE(status_edges.ok);
    ASSERT_GE(edges.size(), 1);
    
    std::string queryEdgeId = edges[0].edgeId;
    
    // Find similar edges
    auto [status_similar, similar] = gem->findSimilarEdges(queryEdgeId, "g1", 1, "test_model");
    EXPECT_TRUE(status_similar.ok);
    
    // Should find other edges (excluding query edge)
    for (const auto& res : similar) {
        EXPECT_NE(res.entity_id, queryEdgeId);
    }
}

TEST_F(GNNEmbeddingTest, GenerateGraphEmbedding) {
    createTestGraph();
    
    // Generate node embeddings first
    gem->generateNodeEmbeddings("g1", "Person", "test_model");
    
    // Generate graph-level embedding with mean pooling
    auto [status_graph_emb, graphEmb] = gem->generateGraphEmbedding("g1", "test_model", "mean");
    EXPECT_TRUE(status_graph_emb.ok);
    EXPECT_EQ(graphEmb.size(), 64);
    
    // Test sum pooling
    auto [status_graph_emb_sum, graphEmbSum] = gem->generateGraphEmbedding("g1", "test_model", "sum");
    EXPECT_TRUE(status_graph_emb_sum.ok);
    
    // Sum should be larger than mean (3 nodes)
    float sumNorm = 0.0f, meanNorm = 0.0f;
    for (size_t i = 0; i < graphEmb.size(); ++i) {
        sumNorm += graphEmbSum[i] * graphEmbSum[i];
        meanNorm += graphEmb[i] * graphEmb[i];
    }
    EXPECT_GT(std::sqrt(sumNorm), std::sqrt(meanNorm));
}

TEST_F(GNNEmbeddingTest, BatchOperations) {
    createTestGraph();
    
    // Generate embeddings in batch
    std::vector<std::string> node_pks = {"person1", "person2", "person3"};
    auto st = gem->generateNodeEmbeddingsBatch(node_pks, "g1", "test_model", 2);
    EXPECT_TRUE(st.ok);
    
    // Verify all embeddings created
    for (const auto& pk : node_pks) {
        auto [status_embedding, embInfo] = gem->getNodeEmbedding(pk, "g1", "test_model");
        EXPECT_TRUE(status_embedding.ok) << " Failed for node: " << pk;
        [[maybe_unused]] auto& embInfoRef = embInfo;
    }
}

TEST_F(GNNEmbeddingTest, GetStats) {
    createTestGraph();
    
    // Generate embeddings
    gem->generateNodeEmbeddings("g1", "Person", "test_model");
    gem->generateEdgeEmbeddings("g1", "knows", "test_model");
    
    // Get stats
    auto [status_stats, stats] = gem->getStats();
    EXPECT_TRUE(status_stats.ok);
    EXPECT_EQ(stats.total_node_embeddings, 3);
    EXPECT_GE(stats.total_edge_embeddings, 2);
    EXPECT_GT(stats.embeddings_per_model["test_model"], 0);
    EXPECT_GT(stats.embeddings_per_graph["g1"], 0);
}

TEST_F(GNNEmbeddingTest, MultiGraphIsolation) {
    // Create nodes in two different graphs
    BaseEntity node1("node1");
    node1.setField("id", std::string("node1"));
    node1.setField("value", 100);
    BaseEntity node2("node2");
    node2.setField("id", std::string("node2"));
    node2.setField("value", 200);
    
    pgm->addNode(node1, "g1");
    pgm->addNodeLabel("node1", "Type1", "g1");
    
    pgm->addNode(node2, "g2");
    pgm->addNodeLabel("node2", "Type1", "g2");
    
    // Generate embeddings for both graphs
    gem->updateNodeEmbedding("node1", "g1", "test_model");
    gem->updateNodeEmbedding("node2", "g2", "test_model");
    
    // Verify isolation: similar search in g1 should not find node2
    auto [status_similar, similar] = gem->findSimilarNodes("node1", "g1", 10, "test_model");
    EXPECT_TRUE(status_similar.ok);
    
    for (const auto& res : similar) {
        EXPECT_EQ(res.graph_id, "g1");
        EXPECT_NE(res.entity_id, "node2");  // node2 is in g2, not g1
    }
}

TEST_F(GNNEmbeddingTest, FeatureExtraction) {
    // Create node with various field types
    BaseEntity node("test_node");
    node.setField("id", std::string("test_node"));
    node.setField("int_field", 42);
    node.setField("double_field", 3.14);
    node.setField("string_field", "test");
    
    pgm->addNode(node, "g1");
    pgm->addNodeLabel("test_node", "Test", "g1");
    
    // Generate embedding with specific feature fields
    std::vector<std::string> features = {"int_field", "double_field"};
    auto st = gem->updateNodeEmbedding("test_node", "g1", "test_model", features);
    EXPECT_TRUE(st.ok);
    
    // Verify embedding was created
    auto [status_embedding, embInfo] = gem->getNodeEmbedding("test_node", "g1", "test_model");
    EXPECT_TRUE(status_embedding.ok);
    EXPECT_FALSE(embInfo.embedding.empty());
}

TEST_F(GNNEmbeddingTest, MultiModelSupport) {
    createTestGraph();
    
    // Register multiple models
    gem->registerModel("model_64", "feature", 64);
    gem->registerModel("model_128", "feature", 128);
    
    // Generate embeddings with different models
    gem->updateNodeEmbedding("person1", "g1", "model_64");
    gem->updateNodeEmbedding("person1", "g1", "model_128");
    
    // Verify different embeddings exist
    auto [status_emb64, emb64] = gem->getNodeEmbedding("person1", "g1", "model_64");
    auto [status_emb128, emb128] = gem->getNodeEmbedding("person1", "g1", "model_128");

    EXPECT_TRUE(status_emb64.ok);
    EXPECT_TRUE(status_emb128.ok);
    EXPECT_EQ(emb64.embedding.size(), 64);
    EXPECT_EQ(emb128.embedding.size(), 128);
}

TEST_F(GNNEmbeddingTest, ErrorHandling) {
    // Test non-existent node
    auto st1 = gem->updateNodeEmbedding("nonexistent", "g1", "test_model");
    EXPECT_FALSE(st1.ok);
    
    // Test non-registered model
    createTestGraph();
    auto st2 = gem->updateNodeEmbedding("person1", "g1", "nonexistent_model");
    EXPECT_FALSE(st2.ok);
    
    // Test getting non-existent embedding
    auto [status_embedding, embedding] = gem->getNodeEmbedding("person1", "g1", "test_model");
    EXPECT_FALSE(status_embedding.ok);  // No embedding generated yet
}

TEST_F(GNNEmbeddingTest, MultiHopNeighborAggregation) {
    // Create a chain graph: person1 -> person2 -> person3 -> person4
    BaseEntity person1("person1");
    person1.setField("id", std::string("person1"));
    person1.setField("value", 100);
    
    BaseEntity person2("person2");
    person2.setField("id", std::string("person2"));
    person2.setField("value", 200);
    
    BaseEntity person3("person3");
    person3.setField("id", std::string("person3"));
    person3.setField("value", 300);
    
    BaseEntity person4("person4");
    person4.setField("id", std::string("person4"));
    person4.setField("value", 400);
    
    // Add nodes
    pgm->addNode(person1, "g1");
    pgm->addNodeLabel("person1", "Person", "g1");
    
    pgm->addNode(person2, "g1");
    pgm->addNodeLabel("person2", "Person", "g1");
    
    pgm->addNode(person3, "g1");
    pgm->addNodeLabel("person3", "Person", "g1");
    
    pgm->addNode(person4, "g1");
    pgm->addNodeLabel("person4", "Person", "g1");
    
    // Create edges forming a chain
    BaseEntity edge1("edge1");
    edge1.setField("id", std::string("edge1"));
    edge1.setField("_from", std::string("person1"));
    edge1.setField("_to", std::string("person2"));
    edge1.setField("_type", std::string("knows"));
    
    BaseEntity edge2("edge2");
    edge2.setField("id", std::string("edge2"));
    edge2.setField("_from", std::string("person2"));
    edge2.setField("_to", std::string("person3"));
    edge2.setField("_type", std::string("knows"));
    
    BaseEntity edge3("edge3");
    edge3.setField("id", std::string("edge3"));
    edge3.setField("_from", std::string("person3"));
    edge3.setField("_to", std::string("person4"));
    edge3.setField("_type", std::string("knows"));
    
    pgm->addEdge(edge1, "g1");
    pgm->addEdge(edge2, "g1");
    pgm->addEdge(edge3, "g1");
    
    // Generate embedding for person1 (should aggregate neighbors at different hops)
    auto st = gem->updateNodeEmbedding("person1", "g1", "test_model");
    EXPECT_TRUE(st.ok);
    
    // Verify embedding was created and is valid
    auto [status_embedding, embInfo] = gem->getNodeEmbedding("person1", "g1", "test_model");
    EXPECT_TRUE(status_embedding.ok);
    EXPECT_FALSE(embInfo.embedding.empty());
    EXPECT_EQ(embInfo.embedding.size(), 64);
    
    // Verify embedding is normalized
    float norm = 0.0f;
    for (float val : embInfo.embedding) {
        norm += val * val;
    }
    EXPECT_NEAR(std::sqrt(norm), 1.0f, 0.01f);
}

TEST_F(GNNEmbeddingTest, NeighborAggregationImpactsEmbedding) {
    // Create two nodes with different neighborhoods
    BaseEntity node1("node1");
    node1.setField("id", std::string("node1"));
    node1.setField("value", 100);
    
    BaseEntity node2("node2");
    node2.setField("id", std::string("node2"));
    node2.setField("value", 100);  // Same features as node1
    
    BaseEntity neighbor1("neighbor1");
    neighbor1.setField("id", std::string("neighbor1"));
    neighbor1.setField("value", 200);
    
    BaseEntity neighbor2("neighbor2");
    neighbor2.setField("id", std::string("neighbor2"));
    neighbor2.setField("value", 300);  // Different value
    
    // Add nodes
    pgm->addNode(node1, "g1");
    pgm->addNode(node2, "g1");
    pgm->addNode(neighbor1, "g1");
    pgm->addNode(neighbor2, "g1");
    
    pgm->addNodeLabel("node1", "TestNode", "g1");
    pgm->addNodeLabel("node2", "TestNode", "g1");
    
    // node1 connects to neighbor1
    BaseEntity edge1("edge1");
    edge1.setField("id", std::string("edge1"));
    edge1.setField("_from", std::string("node1"));
    edge1.setField("_to", std::string("neighbor1"));
    pgm->addEdge(edge1, "g1");
    
    // node2 connects to neighbor2 (different neighbor)
    BaseEntity edge2("edge2");
    edge2.setField("id", std::string("edge2"));
    edge2.setField("_from", std::string("node2"));
    edge2.setField("_to", std::string("neighbor2"));
    pgm->addEdge(edge2, "g1");
    
    // Generate embeddings
    gem->updateNodeEmbedding("node1", "g1", "test_model");
    gem->updateNodeEmbedding("node2", "g1", "test_model");
    
    // Get embeddings
    auto [status1, emb1] = gem->getNodeEmbedding("node1", "g1", "test_model");
    auto [status2, emb2] = gem->getNodeEmbedding("node2", "g1", "test_model");
    
    ASSERT_TRUE(status1.ok);
    ASSERT_TRUE(status2.ok);
    ASSERT_EQ(emb1.embedding.size(), emb2.embedding.size());
    
    // Embeddings should be different because neighborhoods are different
    // (even though node features are the same)
    float similarity = 0.0f;
    for (size_t i = 0; i < emb1.embedding.size(); ++i) {
        similarity += emb1.embedding[i] * emb2.embedding[i];
    }
    
    // Low threshold (0.5): embeddings should be reasonably similar since nodes have same features
    constexpr float SIMILARITY_LOW_THRESHOLD = 0.5f;
    EXPECT_GT(similarity, SIMILARITY_LOW_THRESHOLD);

    // The embeddings must not be bit-for-bit identical even though cosine similarity may be high:
    // different neighborhoods produce small but non-zero structural differences (dims 1-3).
    float dist_sq = 0.0f;
    for (size_t i = 0; i < emb1.embedding.size(); ++i) {
        float d = emb1.embedding[i] - emb2.embedding[i];
        dist_sq += d * d;
    }
    EXPECT_GT(dist_sq, 1e-9f)
        << "Embeddings must differ: different neighborhoods should produce distinct structural signals";
}

TEST_F(GNNEmbeddingTest, AggregationStrategies_ProduceDifferentEmbeddings) {
    createTestGraph();
    
    // Register models with different aggregation strategies
    gem->registerModel("model_mean", "feature", 64);
    gem->setAggregationStrategy("model_mean", GNNEmbeddingManager::AggregationStrategy::MEAN_POOLING);
    
    gem->registerModel("model_max", "feature", 64);
    gem->setAggregationStrategy("model_max", GNNEmbeddingManager::AggregationStrategy::MAX_POOLING);
    
    gem->registerModel("model_sum", "feature", 64);
    gem->setAggregationStrategy("model_sum", GNNEmbeddingManager::AggregationStrategy::SUM_POOLING);
    
    // Generate embeddings with different strategies
    gem->updateNodeEmbedding("person1", "g1", "model_mean");
    gem->updateNodeEmbedding("person1", "g1", "model_max");
    gem->updateNodeEmbedding("person1", "g1", "model_sum");
    
    // Get embeddings
    auto [status_mean, emb_mean] = gem->getNodeEmbedding("person1", "g1", "model_mean");
    auto [status_max, emb_max] = gem->getNodeEmbedding("person1", "g1", "model_max");
    auto [status_sum, emb_sum] = gem->getNodeEmbedding("person1", "g1", "model_sum");
    
    ASSERT_TRUE(status_mean.ok);
    ASSERT_TRUE(status_max.ok);
    ASSERT_TRUE(status_sum.ok);
    
    // Embeddings should be different due to different aggregation strategies
    // Compare mean vs max
    float sim_mean_max = 0.0f;
    for (size_t i = 0; i < emb_mean.embedding.size(); ++i) {
        sim_mean_max += emb_mean.embedding[i] * emb_max.embedding[i];
    }
    
    // Compare mean vs sum
    float sim_mean_sum = 0.0f;
    for (size_t i = 0; i < emb_mean.embedding.size(); ++i) {
        sim_mean_sum += emb_mean.embedding[i] * emb_sum.embedding[i];
    }
    
    // Low threshold (0.5): embeddings should still be reasonably similar
    constexpr float AGGREGATION_SIMILARITY_LOW_THRESHOLD = 0.5f;
    EXPECT_GT(sim_mean_max, AGGREGATION_SIMILARITY_LOW_THRESHOLD);
    EXPECT_GT(sim_mean_sum, AGGREGATION_SIMILARITY_LOW_THRESHOLD);

    // Embeddings from different aggregation strategies must not be identical:
    // each strategy injects a distinct signal at dim[3] (MEAN=0.10, MAX=0.20, SUM=0.30).
    float dist_mean_max_sq = 0.0f;
    float dist_mean_sum_sq = 0.0f;
    for (size_t i = 0; i < emb_mean.embedding.size(); ++i) {
        float d1 = emb_mean.embedding[i] - emb_max.embedding[i];
        float d2 = emb_mean.embedding[i] - emb_sum.embedding[i];
        dist_mean_max_sq += d1 * d1;
        dist_mean_sum_sq += d2 * d2;
    }
    EXPECT_GT(dist_mean_max_sq, 1e-9f)
        << "MEAN and MAX pooling embeddings must differ (different strategy signals)";
    EXPECT_GT(dist_mean_sum_sq, 1e-9f)
        << "MEAN and SUM pooling embeddings must differ (different strategy signals)";
}
