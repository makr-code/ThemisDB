// Test suite: KGRetrieverReasoningTests
//
// KGR-RAG-01  setReasoner(nullptr) is safe and disables reasoning
// KGR-RAG-02  Without reasoner: retrieve() has no inference chains
// KGR-RAG-03  With reasoner: retrieve() populates inference_chains for linked entities
// KGR-RAG-04  has_reasoning is true only when at least one chain is non-empty
// KGR-RAG-05  Reasoning chain is stored in metadata["reasoning_chain"] when flag set
// KGR-RAG-06  max_inference_hops = 0 disables inference even with a reasoner attached
//

#include <gtest/gtest.h>

#include "rag/knowledge_graph_retriever.h"
#include "graph/knowledge_graph_reasoner.h"

#include <string>
#include <vector>

using namespace themis::rag::kg;
using namespace themis::rag::judge;
using namespace themis::graph;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static KnowledgeGraph makeGraph() {
    KnowledgeGraph g;
    (void)g.addNode({"alice", "alice", {}, EntityType::PERSON});
    (void)g.addNode({"bob",   "bob",   {}, EntityType::PERSON});
    (void)g.addEdge({"alice", "bob", RelationType::RELATED_TO, 0.9});
    return g;
}

static void configureReasoner(KnowledgeGraphReasoner& kgr) {
    // Rule: if A reports_to B and B reports_to C then A indirectly_reports_to C
    const bool added_rule = kgr.addRule({ "transitive_reports_to",
                                          {{"?A","reports_to","?B"}, {"?B","reports_to","?C"}},
                                          {{"?A","indirectly_reports_to","?C"}} });
    EXPECT_TRUE(added_rule);
    kgr.addFact({"alice", "reports_to", "bob"});
    kgr.addFact({"bob",   "reports_to", "carol"});
}

static RetrievedDocument makeDoc(const std::string& id,
                                  const std::string& content,
                                  double score = 0.8) {
    return {id, content, score, {}};
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-RAG-01  setReasoner(nullptr) is safe
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG01_SetReasonerNull) {
    auto g = makeGraph();
    KnowledgeGraphRetriever r(g);
    EXPECT_NO_THROW(r.setReasoner(nullptr));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-RAG-02  Without reasoner: no inference chains in result
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG02_NoChainsWithoutReasoner) {
    auto g = makeGraph();
    KnowledgeGraphRetriever r(g);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "alice and bob", 0.7)
    };
    auto res = r.retrieve("alice", cands);
    EXPECT_TRUE(res.inference_chains.empty());
    EXPECT_FALSE(res.has_reasoning);
    EXPECT_DOUBLE_EQ(res.reasoning_elapsed_ms, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-RAG-03  With reasoner: inference_chains populated for linked entities
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG03_ChainsWithReasoner) {
    auto g   = makeGraph();
    KnowledgeGraphReasoner kgr;
    configureReasoner(kgr);

    KGRetrieverConfig cfg;
    cfg.max_inference_hops              = 3;
    cfg.attach_reasoning_chain_to_metadata = false;
    KnowledgeGraphRetriever r(g, cfg);
    r.setReasoner(&kgr);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "alice reports to bob", 0.9)
    };
    auto res = r.retrieve("Alice", cands);

    // alice maps to the KG node "alice"; reasoner derives indirectly_reports_to carol.
    EXPECT_TRUE(res.has_reasoning);
    EXPECT_FALSE(res.inference_chains.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-RAG-04  has_reasoning is true only when a chain has edges
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG04_HasReasoningFlag) {
    // Use an empty reasoner — no rules, no facts → no derived edges.
    auto g = makeGraph();
    KnowledgeGraphReasoner empty_kgr;

    KGRetrieverConfig cfg;
    cfg.max_inference_hops = 3;
    KnowledgeGraphRetriever r(g, cfg);
    r.setReasoner(&empty_kgr);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "alice", 0.7)
    };
    auto res = r.retrieve("alice", cands);
    EXPECT_FALSE(res.has_reasoning);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-RAG-05  Reasoning chain stored in document metadata
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG05_MetadataReasoningChain) {
    auto g   = makeGraph();
    KnowledgeGraphReasoner kgr;
    configureReasoner(kgr);

    KGRetrieverConfig cfg;
    cfg.max_inference_hops              = 3;
    cfg.attach_reasoning_chain_to_metadata = true;
    KnowledgeGraphRetriever r(g, cfg);
    r.setReasoner(&kgr);

    // Document mentions alice, which links to KG node "alice".
    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "alice reports to bob and carol", 0.9)
    };
    auto res = r.retrieve("alice", cands);

    if (res.has_reasoning) {
        bool found_chain = false;
        for (const auto& aug : res.documents) {
            if (aug.document.metadata.count("reasoning_chain")) {
                found_chain = true;
                EXPECT_FALSE(aug.document.metadata.at("reasoning_chain").empty());
            }
        }
        // At least one document should have the chain metadata when reasoning fired.
        EXPECT_TRUE(found_chain);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-RAG-06  max_inference_hops = 0 disables inference
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG06_ZeroHopsDisablesInference) {
    auto g   = makeGraph();
    KnowledgeGraphReasoner kgr;
    configureReasoner(kgr);

    KGRetrieverConfig cfg;
    cfg.max_inference_hops = 0;  // disable
    KnowledgeGraphRetriever r(g, cfg);
    r.setReasoner(&kgr);

    std::vector<RetrievedDocument> cands = {
        makeDoc("d1", "alice", 0.7)
    };
    auto res = r.retrieve("alice", cands);
    EXPECT_FALSE(res.has_reasoning);
    EXPECT_TRUE(res.inference_chains.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-07  applyLoRAScore() heuristic fallback — score = 1/(1+premises)
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG07_ApplyLoRAScoreHeuristicFallback) {
    KnowledgeGraphReasoner kgr;
    configureReasoner(kgr);

    auto chain = kgr.infer("alice", 2);
    ASSERT_FALSE(chain.empty());

    kgr.applyLoRAScore(chain, "test-adapter");

    // Heuristic: score = 1 / (1 + premises.size()); must be in (0, 1].
    for (const auto& edge : chain.edges) {
        EXPECT_GT(edge.lora_score, 0.0);
        EXPECT_LE(edge.lora_score, 1.0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-08  setLoraScoreFn() — injected function is called per edge
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG08_InjectedLoraScoreFnCalled) {
    KnowledgeGraphReasoner kgr;
    configureReasoner(kgr);

    int call_count = 0;
    const double kFixedScore = 0.77;
    kgr.setLoraScoreFn([&call_count, kFixedScore](
            std::string_view /*adapter_id*/,
            const InferenceEdge& /*edge*/) -> double {
        ++call_count;
        return kFixedScore;
    });

    auto chain = kgr.infer("alice", 2);
    ASSERT_FALSE(chain.empty());

    const std::size_t edge_count = chain.edges.size();
    call_count = 0;
    kgr.applyLoRAScore(chain, "test-adapter");

    EXPECT_EQ(static_cast<std::size_t>(call_count), edge_count);
    for (const auto& edge : chain.edges) {
        EXPECT_DOUBLE_EQ(edge.lora_score, kFixedScore);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-09  setLoraScoreFn() — edges below min_lora_score are filtered out
// ─────────────────────────────────────────────────────────────────────────────
TEST(KGRetrieverReasoningTests, KGRRAG09_LoraScoreFilterApplied) {
    KnowledgeGraphReasoner kgr;
    // Rule with min_lora_score = 0.9 — injected backend returns 0.5 → filtered
    const bool added_rule = kgr.addRule({ "strict_rule",
                                          {{"?A","likes","?B"}},
                                          {{"?A","loves","?B"}},
                                          /*lora_adapter=*/"test-adapter",
                                          /*min_lora_score=*/0.9 });
    EXPECT_TRUE(added_rule);
    kgr.addFact({"alice", "likes", "bob"});

    kgr.setLoraScoreFn([](std::string_view, const InferenceEdge&) -> double {
        return 0.5; // below threshold
    });

    auto chain = kgr.infer("alice", 1);
    kgr.applyLoRAScore(chain, "test-adapter");

    // All edges from strict_rule should be filtered out (score 0.5 < 0.9).
    for (const auto& edge : chain.edges) {
        EXPECT_NE(edge.rule_id, "strict_rule");
    }
}
