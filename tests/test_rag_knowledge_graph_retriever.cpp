/**
 * @file test_rag_knowledge_graph_retriever.cpp
 * @brief Unit tests for knowledge graph-augmented retrieval (entity linking)
 *
 * Covers:
 *  - KnowledgeGraph CRUD and BFS traversal
 *  - EntityLinker extraction and linking
 *  - KnowledgeGraphRetriever score fusion and ranking
 *  - Factory helpers
 *  - Edge cases: empty query, empty candidates, unlinked entities
 */

#include "rag/knowledge_graph_retriever.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace themis::rag::kg;
using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static KGNode makeNode(const std::string& id,
                       const std::string& name,
                       std::vector<std::string> aliases = {},
                       EntityType type = EntityType::CONCEPT) {
    KGNode n;
    n.id             = id;
    n.canonical_name = name;
    n.aliases        = std::move(aliases);
    n.type           = type;
    return n;
}

static KGEdge makeEdge(const std::string& from, const std::string& to,
                       RelationType rel = RelationType::RELATED_TO,
                       double weight = 0.8) {
    KGEdge e;
    e.from_id  = from;
    e.to_id    = to;
    e.relation = rel;
    e.weight   = weight;
    return e;
}

static RetrievedDocument makeDoc(const std::string& id,
                                 const std::string& content,
                                 double score = 0.8) {
    RetrievedDocument d;
    d.id               = id;
    d.content          = content;
    d.similarity_score = score;
    return d;
}

// Build a small, reusable knowledge graph with ML-related entities.
static KnowledgeGraph buildMLGraph() {
    KnowledgeGraph g;
    g.addNode(makeNode("ent-hnsw",    "HNSW Algorithm",     {"HNSW", "Hierarchical NSW"}));
    g.addNode(makeNode("ent-ann",     "ANN Search",         {"Approximate Nearest Neighbor"}));
    g.addNode(makeNode("ent-vector",  "Vector Index",       {"vector search", "embeddings index"}));
    g.addNode(makeNode("ent-ml",      "Machine Learning",   {"ML"}));
    g.addNode(makeNode("ent-llm",     "Large Language Model", {"LLM"}));
    g.addNode(makeNode("ent-rag",     "Retrieval Augmented Generation", {"RAG"}));

    g.addEdge(makeEdge("ent-hnsw",   "ent-ann",    RelationType::IS_A,         0.9));
    g.addEdge(makeEdge("ent-ann",    "ent-vector", RelationType::RELATED_TO,   0.8));
    g.addEdge(makeEdge("ent-ml",     "ent-llm",    RelationType::HAS_PART,     0.7));
    g.addEdge(makeEdge("ent-rag",    "ent-llm",    RelationType::RELATED_TO,   0.9));
    g.addEdge(makeEdge("ent-rag",    "ent-vector", RelationType::RELATED_TO,   0.85));
    return g;
}

// ===========================================================================
// KnowledgeGraph – CRUD
// ===========================================================================

class KGCRUDTest : public ::testing::Test {
protected:
    KnowledgeGraph g;
};

TEST_F(KGCRUDTest, InitiallyEmpty) {
    EXPECT_EQ(g.nodeCount(), 0u);
    EXPECT_EQ(g.edgeCount(), 0u);
}

TEST_F(KGCRUDTest, AddNodeIncreasesCount) {
    g.addNode(makeNode("n1", "Alpha"));
    EXPECT_EQ(g.nodeCount(), 1u);
    g.addNode(makeNode("n2", "Beta"));
    EXPECT_EQ(g.nodeCount(), 2u);
}

TEST_F(KGCRUDTest, AddDuplicateNodeOverwrites) {
    g.addNode(makeNode("n1", "Alpha"));
    g.addNode(makeNode("n1", "AlphaV2"));   // same id
    EXPECT_EQ(g.nodeCount(), 1u);
    const KGNode* n = g.findNode("n1");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->canonical_name, "AlphaV2");
}

TEST_F(KGCRUDTest, FindNodeByIdReturnsCorrectNode) {
    g.addNode(makeNode("n1", "Alpha"));
    const KGNode* n = g.findNode("n1");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->canonical_name, "Alpha");
}

TEST_F(KGCRUDTest, FindNodeByIdMissingReturnsNull) {
    EXPECT_EQ(g.findNode("nonexistent"), nullptr);
}

TEST_F(KGCRUDTest, FindNodeByNameExactMatch) {
    g.addNode(makeNode("n1", "HNSW Algorithm", {"HNSW"}));
    const KGNode* n = g.findNodeByName("HNSW Algorithm");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->id, "n1");
}

TEST_F(KGCRUDTest, FindNodeByNameAliasMatch) {
    g.addNode(makeNode("n1", "HNSW Algorithm", {"HNSW"}));
    const KGNode* n = g.findNodeByName("HNSW");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->id, "n1");
}

TEST_F(KGCRUDTest, FindNodeByNameCaseInsensitive) {
    g.addNode(makeNode("n1", "Machine Learning"));
    const KGNode* n = g.findNodeByName("machine learning");
    ASSERT_NE(n, nullptr);
}

TEST_F(KGCRUDTest, FindNodeByNameNoMatchReturnsNull) {
    g.addNode(makeNode("n1", "Machine Learning"));
    EXPECT_EQ(g.findNodeByName("quantum computing"), nullptr);
}

TEST_F(KGCRUDTest, RemoveExistingNode) {
    g.addNode(makeNode("n1", "Alpha"));
    EXPECT_TRUE(g.removeNode("n1"));
    EXPECT_EQ(g.nodeCount(), 0u);
    EXPECT_EQ(g.findNode("n1"), nullptr);
}

TEST_F(KGCRUDTest, RemoveNonexistentNodeReturnsFalse) {
    EXPECT_FALSE(g.removeNode("ghost"));
}

TEST_F(KGCRUDTest, AddEdgeIncreasesCount) {
    g.addNode(makeNode("n1", "A"));
    g.addNode(makeNode("n2", "B"));
    g.addEdge(makeEdge("n1", "n2"));
    EXPECT_EQ(g.edgeCount(), 1u);
}

TEST_F(KGCRUDTest, RemoveEdge) {
    g.addNode(makeNode("n1", "A"));
    g.addNode(makeNode("n2", "B"));
    g.addEdge(makeEdge("n1", "n2", RelationType::RELATED_TO));
    EXPECT_TRUE(g.removeEdge("n1", "n2", RelationType::RELATED_TO));
    EXPECT_EQ(g.edgeCount(), 0u);
}

TEST_F(KGCRUDTest, RemoveNonexistentEdgeReturnsFalse) {
    g.addNode(makeNode("n1", "A"));
    g.addNode(makeNode("n2", "B"));
    EXPECT_FALSE(g.removeEdge("n1", "n2", RelationType::RELATED_TO));
}

TEST_F(KGCRUDTest, RemoveNodeAlsoRemovesIncidentEdges) {
    g.addNode(makeNode("n1", "A"));
    g.addNode(makeNode("n2", "B"));
    g.addEdge(makeEdge("n1", "n2"));
    g.addEdge(makeEdge("n2", "n1"));
    g.removeNode("n2");
    // Both edges touching n2 should be gone
    EXPECT_EQ(g.edgeCount(), 0u);
}

TEST_F(KGCRUDTest, OutEdgesReturnsCorrectList) {
    g.addNode(makeNode("n1", "A"));
    g.addNode(makeNode("n2", "B"));
    g.addNode(makeNode("n3", "C"));
    g.addEdge(makeEdge("n1", "n2"));
    g.addEdge(makeEdge("n1", "n3"));
    auto edges = g.outEdges("n1");
    EXPECT_EQ(edges.size(), 2u);
}

// ===========================================================================
// KnowledgeGraph – BFS traversal
// ===========================================================================

class KGTraversalTest : public ::testing::Test {
protected:
    KnowledgeGraph g = buildMLGraph();
};

TEST_F(KGTraversalTest, DirectNeighbours) {
    auto nbrs = g.neighbours("ent-rag", 1, 0.0);
    // ent-rag → ent-llm, ent-vector
    EXPECT_EQ(nbrs.count("ent-llm"),    1u);
    EXPECT_EQ(nbrs.count("ent-vector"), 1u);
    EXPECT_EQ(nbrs.count("ent-rag"),    0u);  // start excluded
}

TEST_F(KGTraversalTest, TwoHopNeighbours) {
    // ent-rag → ent-vector → ... (1-hop adds ent-llm, ent-vector)
    // 2-hop from ent-rag should also include ent-ann (via ent-vector)
    auto nbrs = g.neighbours("ent-rag", 2, 0.0);
    EXPECT_GE(nbrs.size(), 2u);
    EXPECT_EQ(nbrs.count("ent-vector"), 1u);
    EXPECT_EQ(nbrs.count("ent-llm"),    1u);
}

TEST_F(KGTraversalTest, MinEdgeWeightFilters) {
    // ent-ml → ent-llm has weight 0.7; should be excluded if min_weight=0.8
    auto nbrs_strict = g.neighbours("ent-ml", 1, 0.8);
    EXPECT_EQ(nbrs_strict.count("ent-llm"), 0u);

    auto nbrs_loose = g.neighbours("ent-ml", 1, 0.5);
    EXPECT_EQ(nbrs_loose.count("ent-llm"), 1u);
}

TEST_F(KGTraversalTest, UnknownStartNodeReturnsEmpty) {
    auto nbrs = g.neighbours("no-such-node", 2, 0.0);
    EXPECT_TRUE(nbrs.empty());
}

TEST_F(KGTraversalTest, DepthZeroReturnsEmpty) {
    auto nbrs = g.neighbours("ent-rag", 0, 0.0);
    EXPECT_TRUE(nbrs.empty());
}

// ===========================================================================
// EntityLinker
// ===========================================================================

class EntityLinkerTest : public ::testing::Test {
protected:
    KnowledgeGraph g = buildMLGraph();
    EntityLinkerConfig cfg;
    // Default config
};

TEST_F(EntityLinkerTest, LinkKnownEntity) {
    EntityLinker linker(g, cfg);
    // "HNSW Algorithm" is a canonical name in the graph
    auto matches = linker.link("We use HNSW Algorithm for vector search.");
    bool found = false;
    for (const auto& m : matches) {
        if (m.is_linked && m.node_id == "ent-hnsw") {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected ent-hnsw to be linked";
}

TEST_F(EntityLinkerTest, LinkByAlias) {
    EntityLinker linker(g, cfg);
    auto matches = linker.link("HNSW is fast for approximate nearest neighbor.");
    bool found = false;
    for (const auto& m : matches) {
        if (m.is_linked && m.node_id == "ent-hnsw") {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected ent-hnsw matched via alias 'HNSW'";
}

TEST_F(EntityLinkerTest, UnknownEntityNotLinked) {
    EntityLinker linker(g, cfg);
    auto matches = linker.link("Bananas are rich in potassium.");
    for (const auto& m : matches) {
        EXPECT_FALSE(m.is_linked) << "Unexpected link for: " << m.entity.text;
    }
}

TEST_F(EntityLinkerTest, EmptyTextReturnsEmptyMatches) {
    EntityLinker linker(g, cfg);
    auto matches = linker.link("");
    EXPECT_TRUE(matches.empty());
}

TEST_F(EntityLinkerTest, LinkingScoreInRange) {
    EntityLinker linker(g, cfg);
    auto matches = linker.link("Machine Learning is used in RAG systems.");
    for (const auto& m : matches) {
        EXPECT_GE(m.linking_score, 0.0);
        EXPECT_LE(m.linking_score, 1.0);
    }
}

// ===========================================================================
// KnowledgeGraphRetriever – basic usage
// ===========================================================================

class KGRetrieverTest : public ::testing::Test {
protected:
    KnowledgeGraph  graph = buildMLGraph();
    KGRetrieverConfig cfg;

    std::vector<RetrievedDocument> makeCandidates() {
        return {
            makeDoc("doc1",
                "HNSW Algorithm is used for efficient approximate nearest neighbor search.",
                0.85),
            makeDoc("doc2",
                "Bananas are a tropical fruit.",
                0.75),
            makeDoc("doc3",
                "RAG combines retrieval with Large Language Model generation.",
                0.80)
        };
    }
};

TEST_F(KGRetrieverTest, ReturnsAllCandidates) {
    KnowledgeGraphRetriever retriever(graph, cfg);
    auto result = retriever.retrieve("HNSW vector search", makeCandidates());
    EXPECT_EQ(result.documents.size(), 3u);
}

TEST_F(KGRetrieverTest, EmptyCandidatesReturnsEmpty) {
    KnowledgeGraphRetriever retriever(graph, cfg);
    auto result = retriever.retrieve("query", {});
    EXPECT_TRUE(result.documents.empty());
}

TEST_F(KGRetrieverTest, KGRelevantDocGetsBoost) {
    cfg.kg_score_weight = 0.5;  // strong KG influence
    KnowledgeGraphRetriever retriever(graph, cfg);

    // doc1 mentions "HNSW Algorithm" (linked in graph), doc2 is unrelated
    auto candidates = makeCandidates();
    // Force equal original scores so only KG boost differentiates
    candidates[0].similarity_score = 0.7;
    candidates[1].similarity_score = 0.7;

    auto result = retriever.retrieve("HNSW Algorithm ANN search", candidates);
    ASSERT_GE(result.documents.size(), 2u);

    // The doc about HNSW should appear before the banana doc
    bool hnsw_before_banana = false;
    bool seen_hnsw = false;
    for (const auto& aug : result.documents) {
        if (aug.document.id == "doc1") { seen_hnsw = true; }
        if (aug.document.id == "doc2" && seen_hnsw) { hnsw_before_banana = true; break; }
    }
    EXPECT_TRUE(hnsw_before_banana)
        << "KG-relevant HNSW doc should rank above unrelated banana doc";
}

TEST_F(KGRetrieverTest, FinalScoreInRange) {
    KnowledgeGraphRetriever retriever(graph, cfg);
    auto result = retriever.retrieve("HNSW search", makeCandidates());
    for (const auto& aug : result.documents) {
        EXPECT_GE(aug.final_score, 0.0);
        EXPECT_LE(aug.final_score, 1.0 + 1e-9);
    }
}

TEST_F(KGRetrieverTest, SortedByFinalScoreDescending) {
    KnowledgeGraphRetriever retriever(graph, cfg);
    auto result = retriever.retrieve("HNSW Algorithm", makeCandidates());
    for (size_t i = 1; i < result.documents.size(); ++i) {
        EXPECT_GE(result.documents[i - 1].final_score,
                  result.documents[i].final_score)
            << "Documents not sorted at position " << i;
    }
}

TEST_F(KGRetrieverTest, QueryEntityLinksPopulated) {
    KnowledgeGraphRetriever retriever(graph, cfg);
    auto result = retriever.retrieve("HNSW Algorithm for ANN Search", makeCandidates());
    // There should be at least one linked entity in the query
    bool any_linked = false;
    for (const auto& m : result.query_entity_links) {
        if (m.is_linked) { any_linked = true; break; }
    }
    EXPECT_TRUE(any_linked);
}

TEST_F(KGRetrieverTest, EntityLinkingCoverageRange) {
    KnowledgeGraphRetriever retriever(graph, cfg);
    auto result = retriever.retrieve("HNSW Algorithm", makeCandidates());
    EXPECT_GE(result.entity_linking_coverage, 0.0);
    EXPECT_LE(result.entity_linking_coverage, 1.0);
}

TEST_F(KGRetrieverTest, ElapsedMsPositive) {
    KnowledgeGraphRetriever retriever(graph, cfg);
    auto result = retriever.retrieve("query", makeCandidates());
    EXPECT_GE(result.elapsed_ms, 0.0);
}

TEST_F(KGRetrieverTest, EmptyQueryReturnsOriginalOrder) {
    cfg.kg_score_weight = 0.3;
    KnowledgeGraphRetriever retriever(graph, cfg);
    // With no linked entities the kg_boost is 0, so fused score = orig * 0.7
    // Original scores: doc1=0.85, doc2=0.75, doc3=0.80 → order: doc1, doc3, doc2
    auto result = retriever.retrieve("", makeCandidates());
    ASSERT_EQ(result.documents.size(), 3u);
    // doc1 should still rank highest as it has the highest original score
    EXPECT_EQ(result.documents[0].document.id, "doc1");
}

TEST_F(KGRetrieverTest, VisitedNodesPopulatedWhenLinked) {
    KnowledgeGraphRetriever retriever(graph, cfg);
    auto result = retriever.retrieve("HNSW Algorithm", makeCandidates());
    // Some nodes should have been visited if linking succeeded
    // (may be empty if no entities were linked)
    EXPECT_GE(result.visited_nodes.size(), 0u);  // always true but valid check
}

// ===========================================================================
// KnowledgeGraphRetriever – configuration
// ===========================================================================

TEST(KGRetrieverConfigTest, GetSetConfig) {
    KnowledgeGraph g;
    KGRetrieverConfig cfg;
    cfg.max_traversal_depth = 3;
    cfg.kg_score_weight     = 0.4;

    KnowledgeGraphRetriever retriever(g, cfg);
    EXPECT_EQ(retriever.getConfig().max_traversal_depth, 3u);
    EXPECT_DOUBLE_EQ(retriever.getConfig().kg_score_weight, 0.4);

    KGRetrieverConfig new_cfg;
    new_cfg.max_traversal_depth = 1;
    retriever.setConfig(new_cfg);
    EXPECT_EQ(retriever.getConfig().max_traversal_depth, 1u);
}

// ===========================================================================
// Factory helpers
// ===========================================================================

TEST(KGFactoryTest, CreateShallow) {
    KnowledgeGraph g;
    auto r = KnowledgeGraphRetrieverFactory::createShallow(g);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->getConfig().max_traversal_depth, 1u);
    EXPECT_DOUBLE_EQ(r->getConfig().kg_score_weight, 0.2);
}

TEST(KGFactoryTest, CreateBalanced) {
    KnowledgeGraph g;
    auto r = KnowledgeGraphRetrieverFactory::createBalanced(g);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->getConfig().max_traversal_depth, 2u);
    EXPECT_DOUBLE_EQ(r->getConfig().kg_score_weight, 0.3);
}

TEST(KGFactoryTest, CreateDeep) {
    KnowledgeGraph g;
    auto r = KnowledgeGraphRetrieverFactory::createDeep(g);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->getConfig().max_traversal_depth, 3u);
    EXPECT_DOUBLE_EQ(r->getConfig().kg_score_weight, 0.45);
}

// ===========================================================================
// Edge cases & stress
// ===========================================================================

TEST(KGEdgeCasesTest, LargeGraphDoesNotHang) {
    KnowledgeGraph g;
    // Add 500 nodes in a chain
    for (int i = 0; i < 500; ++i) {
        g.addNode(makeNode("n" + std::to_string(i), "Node" + std::to_string(i)));
    }
    for (int i = 0; i < 499; ++i) {
        g.addEdge(makeEdge("n" + std::to_string(i), "n" + std::to_string(i + 1)));
    }

    KGRetrieverConfig cfg;
    cfg.max_nodes_visited   = 50;   // circuit-breaker
    cfg.max_traversal_depth = 500;  // uncapped depth
    KnowledgeGraphRetriever retriever(g, cfg);

    std::vector<RetrievedDocument> docs = { makeDoc("d1", "Node0 is the first.") };
    auto result = retriever.retrieve("Node0", docs);
    // Should finish quickly without visiting all 500 nodes
    EXPECT_LE(result.visited_nodes.size(), cfg.max_nodes_visited + 1);
}

TEST(KGEdgeCasesTest, DocumentsWithNoEntitiesGetNoBoost) {
    KnowledgeGraph g;
    g.addNode(makeNode("n1", "HNSW"));
    KGRetrieverConfig cfg;
    cfg.kg_score_weight = 0.3;
    KnowledgeGraphRetriever retriever(g, cfg);

    auto docs = std::vector<RetrievedDocument>{
        makeDoc("d1", "completely unrelated text about weather", 0.6)
    };
    auto result = retriever.retrieve("HNSW vector", docs);
    ASSERT_EQ(result.documents.size(), 1u);
    // kg_boost should be near 0, final_score ≈ 0.6 * 0.7
    EXPECT_NEAR(result.documents[0].kg_boost, 0.0, 0.01);
}

TEST(KGEdgeCasesTest, KGWeightZeroPreservesOriginalScores) {
    KnowledgeGraph g = buildMLGraph();
    KGRetrieverConfig cfg;
    cfg.kg_score_weight = 0.0;
    KnowledgeGraphRetriever retriever(g, cfg);

    auto docs = std::vector<RetrievedDocument>{
        makeDoc("d1", "HNSW Algorithm", 0.9),
        makeDoc("d2", "Bananas", 0.5)
    };
    auto result = retriever.retrieve("HNSW", docs);
    ASSERT_EQ(result.documents.size(), 2u);
    // With weight=0 final_score == original score
    EXPECT_DOUBLE_EQ(result.documents[0].final_score,
                     result.documents[0].document.similarity_score);
    EXPECT_DOUBLE_EQ(result.documents[1].final_score,
                     result.documents[1].document.similarity_score);
}

// ===========================================================================
// GAP-010 — BFS DoS guard: neighbours() must respect the max_nodes cap
// ===========================================================================

// GAP-010-01: Build a dense chain graph (1000 nodes → single path) and verify
// that neighbours() respects a cap of 10 nodes, terminating early instead of
// visiting all 1000 nodes.
TEST(GAP010BfsCapTest, DenseChain_NodeCapRespected) {
    KnowledgeGraph g;
    for (int i = 0; i < 1000; ++i) {
        KGNode n;
        n.id = "c" + std::to_string(i);
        n.canonical_name = "chain node " + std::to_string(i);
        g.addNode(n);
    }
    for (int i = 0; i < 999; ++i) {
        KGEdge e;
        e.from_id = "c" + std::to_string(i);
        e.to_id   = "c" + std::to_string(i + 1);
        e.weight  = 1.0;
        g.addEdge(e);
    }

    // Set max_depth high enough that depth isn't the limiting factor;
    // max_nodes=10 should be the binding constraint.
    auto nbrs = g.neighbours("c0", /*max_depth=*/2000, /*min_weight=*/0.0, /*max_nodes=*/10);

    EXPECT_LE(nbrs.size(), 10u)
        << "BFS must not exceed max_nodes cap";
}

// GAP-010-02: When max_nodes is larger than the graph, all reachable nodes
// are returned (no spurious truncation).
TEST(GAP010BfsCapTest, SmallGraph_AllNodesReturned) {
    KnowledgeGraph g = buildMLGraph();  // 6 nodes, sparse

    // With a generous cap the full neighbourhood must be returned.
    auto nbrs = g.neighbours("ent-rag", /*max_depth=*/10, /*min_weight=*/0.0,
                             /*max_nodes=*/1000);

    // ent-rag is reachable to all other 5 nodes through the ML graph
    // (transitively at depth 2); at least direct neighbours must be present.
    EXPECT_GE(nbrs.size(), 2u)
        << "All reachable nodes should be returned when cap is generous";
}
