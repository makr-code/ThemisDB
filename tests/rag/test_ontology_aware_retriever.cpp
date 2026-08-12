// Test suite: OntologyAwareRetrieverTests
//
// OAR-01  OntologyRetrieverConfig default values are sensible
// OAR-02  Factory::createShallow produces expected config
// OAR-03  Factory::createBalanced produces expected config
// OAR-04  Factory::createDeep produces expected config
// OAR-05  retrieve() returns empty when no candidates supplied
// OAR-06  retrieve() with no ontology concept hits degrades gracefully
// OAR-07  Entity expansion adds superclass concept IDs for known concepts
// OAR-08  Edge-type filtering flag disables ontology checks when false
//

#include <gtest/gtest.h>

#include "rag/ontology_aware_retriever.h"
#include "rag/knowledge_graph_retriever.h"
#include "graph/ontology_manager.h"

#include <string>
#include <vector>

using namespace themis::rag;
using namespace themis::rag::kg;
using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static KnowledgeGraph makeGraph() {
    KnowledgeGraph g;
    g.addNode({"ent-1", "HNSW Algorithm", {"HNSW"}, EntityType::CONCEPT});
    g.addNode({"ent-2", "Vector Index",   {},        EntityType::CONCEPT});
    g.addEdge({"ent-1", "ent-2", RelationType::RELATED_TO, 0.9});
    return g;
}

static void configureOntology(themis::graph::OntologyManager& om) {
    om.addConcept("Entity");
    om.addConcept("Concept",      {"Entity"});
    om.addConcept("TechConcept",  {"Concept"});
    om.addAxiom("Concept", "RELATED_TO", "Concept");
    om.build();
}

static RetrievedDocument makeDoc(const std::string& id,
                                  const std::string& content,
                                  double score = 0.8) {
    return {id, content, score, {}};
}

// ─────────────────────────────────────────────────────────────────────────────
// OAR-01  OntologyRetrieverConfig defaults
// ─────────────────────────────────────────────────────────────────────────────
TEST(OntologyAwareRetrieverTests, OAR01_DefaultConfig) {
    OntologyRetrieverConfig cfg;
    EXPECT_EQ(cfg.max_traversal_depth,      2u);
    EXPECT_DOUBLE_EQ(cfg.kg_score_weight,   0.3);
    EXPECT_GE(cfg.max_superclass_expansion, 1u);
    EXPECT_TRUE(cfg.filter_by_allowed_edge_types);
}

// ─────────────────────────────────────────────────────────────────────────────
// OAR-02  Factory::createShallow
// ─────────────────────────────────────────────────────────────────────────────
TEST(OntologyAwareRetrieverTests, OAR02_FactoryShallow) {
    auto g = makeGraph();
    themis::graph::OntologyManager o; configureOntology(o);
    auto r = OntologyAwareRetrieverFactory::createShallow(g, o);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->config().max_traversal_depth, 1u);
    EXPECT_LT(r->config().kg_score_weight, 0.25);
    EXPECT_FALSE(r->config().filter_by_allowed_edge_types);
}

// ─────────────────────────────────────────────────────────────────────────────
// OAR-03  Factory::createBalanced
// ─────────────────────────────────────────────────────────────────────────────
TEST(OntologyAwareRetrieverTests, OAR03_FactoryBalanced) {
    auto g = makeGraph();
    themis::graph::OntologyManager o; configureOntology(o);
    auto r = OntologyAwareRetrieverFactory::createBalanced(g, o);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->config().max_traversal_depth, 2u);
    EXPECT_DOUBLE_EQ(r->config().kg_score_weight, 0.3);
    EXPECT_TRUE(r->config().filter_by_allowed_edge_types);
}

// ─────────────────────────────────────────────────────────────────────────────
// OAR-04  Factory::createDeep
// ─────────────────────────────────────────────────────────────────────────────
TEST(OntologyAwareRetrieverTests, OAR04_FactoryDeep) {
    auto g = makeGraph();
    themis::graph::OntologyManager o; configureOntology(o);
    auto r = OntologyAwareRetrieverFactory::createDeep(g, o);
    ASSERT_NE(r, nullptr);
    EXPECT_GE(r->config().max_traversal_depth, 3u);
    EXPECT_GT(r->config().kg_score_weight, 0.3);
    EXPECT_TRUE(r->config().filter_by_allowed_edge_types);
}

// ─────────────────────────────────────────────────────────────────────────────
// OAR-05  retrieve() with no candidates returns empty document list
// ─────────────────────────────────────────────────────────────────────────────
TEST(OntologyAwareRetrieverTests, OAR05_EmptyCandidates) {
    auto g = makeGraph();
    themis::graph::OntologyManager o; configureOntology(o);
    OntologyAwareRetriever r(g, o);

    auto res = r.retrieve("HNSW search", {});
    EXPECT_TRUE(res.documents.empty());
    EXPECT_GE(res.elapsed_ms, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// OAR-06  retrieve() with unlinked query degrades gracefully
// ─────────────────────────────────────────────────────────────────────────────
TEST(OntologyAwareRetrieverTests, OAR06_UnknownEntityDegradation) {
    auto g = makeGraph();
    themis::graph::OntologyManager o; configureOntology(o);
    OntologyAwareRetriever r(g, o);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "unrelated document", 0.5)
    };
    auto res = r.retrieve("zzz_no_entity_match_xyz", cands);

    ASSERT_EQ(res.documents.size(), 1u);
    // Graceful fallback: document is still returned (possibly with reduced score).
    EXPECT_GE(res.documents[0].final_score, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// OAR-07  expanded_concepts includes superclass ancestors
// ─────────────────────────────────────────────────────────────────────────────
TEST(OntologyAwareRetrieverTests, OAR07_ConceptExpansion) {
    // Build a graph where the linked node has type CONCEPT.
    KnowledgeGraph g;
    g.addNode({"tech-1", "HNSW", {}, EntityType::CONCEPT});

    themis::graph::OntologyManager om;
    om.addConcept("Entity");
    om.addConcept("Concept",     {"Entity"});
    om.addConcept("TechConcept", {"Concept"});
    om.build();

    OntologyRetrieverConfig cfg;
    cfg.max_superclass_expansion = 5;
    OntologyAwareRetriever r(g, om, cfg);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "HNSW vector index", 0.9)
    };
    auto res = r.retrieve("HNSW vector index", cands);
    // expanded_concepts should contain at least the entity type name of CONCEPT.
    EXPECT_FALSE(res.expanded_concepts.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// OAR-08  filter_by_allowed_edge_types = false → edges always allowed
// ─────────────────────────────────────────────────────────────────────────────
TEST(OntologyAwareRetrieverTests, OAR08_FilterDisabled) {
    auto g = makeGraph();
    themis::graph::OntologyManager om; // Not built — no axioms.

    OntologyRetrieverConfig cfg;
    cfg.filter_by_allowed_edge_types = false;
    OntologyAwareRetriever r(g, om, cfg);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "HNSW vector search", 0.8)
    };
    // Should not throw even with an unbuilt ontology.
    EXPECT_NO_THROW({ auto res = r.retrieve("HNSW", cands); });
}

