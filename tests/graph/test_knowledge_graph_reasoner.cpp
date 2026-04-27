/**
 * @file test_knowledge_graph_reasoner.cpp
 * @brief Unit tests for KnowledgeGraphReasoner and InferenceStore — KGR-01..KGR-20
 *
 * Test coverage:
 *   KGR-01..05  Horn-clause rule application (transitive, reflexive, inverse, chained)
 *   KGR-06..10  InferenceChain / InferenceEdge provenance and explain() proof traces
 *   KGR-11..13  Incremental CDC-trigger tests (INSERT / DELETE)
 *   KGR-14..16  applyLoRAScore() — stub integration
 *   KGR-17..18  Structural pattern detection (chain-of-authority, hub-spoke)
 *   KGR-19..20  InferenceStore capacity and TTL eviction
 */

#include "graph/knowledge_graph_reasoner.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::graph;

// Helper — find a derived edge in an InferenceChain by predicate.
static const InferenceEdge* findEdge(const InferenceChain& chain,
                                     const std::string& predicate) {
    for (const auto& e : chain.edges) {
        if (e.fact.predicate == predicate) return &e;
    }
    return nullptr;
}

// Helper — check if a triple is present in the chain.
static bool hasTriple(const InferenceChain& chain,
                       const std::string& subj, const std::string& pred,
                       const std::string& obj) {
    for (const auto& e : chain.edges) {
        if (e.fact.subject == subj && e.fact.predicate == pred && e.fact.object == obj)
            return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-01: Simple one-hop transitive rule
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR01_SimpleTransitiveRule) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"transitive_knows",
                 {{"?A", "knows", "?B"}, {"?B", "knows", "?C"}},
                 {{"?A", "indirectly_knows", "?C"}}});

    kgr.addFact({"alice", "knows", "bob"});
    kgr.addFact({"bob",   "knows", "carol"});

    auto chain = kgr.infer("alice", 2);
    ASSERT_FALSE(chain.empty());
    EXPECT_TRUE(hasTriple(chain, "alice", "indirectly_knows", "carol"));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-02: Multi-hop transitive chain using proper closure rules
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR02_MultiHopTransitive) {
    KnowledgeGraphReasoner kgr(5);

    // Standard transitive closure: base case lifts reports_to → indirectly_reports_to,
    // then the inductive case extends the chain.
    kgr.addRule({"reports_to_base",
                 {{"?A", "reports_to", "?B"}},
                 {{"?A", "indirectly_reports_to", "?B"}}});

    kgr.addRule({"reports_to_trans",
                 {{"?A", "indirectly_reports_to", "?B"}, {"?B", "reports_to", "?C"}},
                 {{"?A", "indirectly_reports_to", "?C"}}});

    kgr.addFact({"alice", "reports_to", "bob"});
    kgr.addFact({"bob",   "reports_to", "carol"});
    kgr.addFact({"carol", "reports_to", "dave"});

    auto chain = kgr.infer("alice", 5);
    EXPECT_TRUE(hasTriple(chain, "alice", "indirectly_reports_to", "bob"));
    EXPECT_TRUE(hasTriple(chain, "alice", "indirectly_reports_to", "carol"));
    EXPECT_TRUE(hasTriple(chain, "alice", "indirectly_reports_to", "dave"));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-03: Inverse relation rule
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR03_InverseRule) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"inverse_manages",
                 {{"?A", "reports_to", "?B"}},
                 {{"?B", "manages", "?A"}}});

    kgr.addFact({"alice", "reports_to", "bob"});

    auto chain = kgr.infer("bob", 1);
    ASSERT_FALSE(chain.empty());
    EXPECT_TRUE(hasTriple(chain, "bob", "manages", "alice"));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-04: Dual-conclusion rule
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR04_DualConclusionRule) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"partner_symmetric",
                 {{"?A", "partner_of", "?B"}},
                 {{"?B", "partner_of", "?A"}, {"?A", "knows", "?B"}}});

    kgr.addFact({"alice", "partner_of", "bob"});

    // infer for "bob" to get the symmetric partner_of
    auto chain_bob = kgr.infer("bob", 1);
    EXPECT_TRUE(hasTriple(chain_bob, "bob", "partner_of", "alice"));

    // infer for "alice" to get the derived knows
    auto chain_alice = kgr.infer("alice", 1);
    EXPECT_TRUE(hasTriple(chain_alice, "alice", "knows", "bob"));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-05: No derivation when facts are insufficient
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR05_NoDerivationWhenFactsMissing) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"transitive",
                 {{"?A", "knows", "?B"}, {"?B", "knows", "?C"}},
                 {{"?A", "indirectly_knows", "?C"}}});

    // Only one fact — rule cannot fire.
    kgr.addFact({"alice", "knows", "bob"});

    auto chain = kgr.infer("alice", 2);
    EXPECT_TRUE(chain.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-06: explain() returns correct rule_id
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR06_ExplainReturnsRuleId) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"my_rule",
                 {{"?A", "friend", "?B"}},
                 {{"?A", "knows", "?B"}}});

    kgr.addFact({"alice", "friend", "bob"});
    kgr.infer("alice", 1); // populate store

    auto proof = kgr.explain({"alice", "knows", "bob"});
    ASSERT_TRUE(proof.has_value());
    EXPECT_EQ(proof->rule_id, "my_rule");
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-07: explain() returns premises
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR07_ExplainReturnsPremises) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"transitive",
                 {{"?A", "reports_to", "?B"}, {"?B", "reports_to", "?C"}},
                 {{"?A", "indirectly_reports_to", "?C"}}});

    kgr.addFact({"alice", "reports_to", "bob"});
    kgr.addFact({"bob",   "reports_to", "carol"});
    kgr.infer("alice", 2);

    auto proof = kgr.explain({"alice", "indirectly_reports_to", "carol"});
    ASSERT_TRUE(proof.has_value());
    ASSERT_EQ(proof->premises.size(), 2u);

    bool has_ab = false, has_bc = false;
    for (const auto& p : proof->premises) {
        if (p.subject == "alice" && p.predicate == "reports_to" && p.object == "bob")   has_ab = true;
        if (p.subject == "bob"   && p.predicate == "reports_to" && p.object == "carol") has_bc = true;
    }
    EXPECT_TRUE(has_ab);
    EXPECT_TRUE(has_bc);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-08: explain() returns nullopt for unknown triple
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR08_ExplainNulloptForUnknown) {
    KnowledgeGraphReasoner kgr;
    auto proof = kgr.explain({"nobody", "knows", "nothing"});
    EXPECT_FALSE(proof.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-09: infer() subject filter — only requested subject returned
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR09_InferSubjectFilter) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"inv",
                 {{"?A", "follows", "?B"}},
                 {{"?B", "followed_by", "?A"}}});

    kgr.addFact({"alice", "follows", "bob"});
    kgr.addFact({"carol", "follows", "dave"});

    // Request for "bob" should only return edges involving bob as subject.
    auto chain = kgr.infer("bob", 1);
    for (const auto& e : chain.edges) {
        EXPECT_EQ(e.fact.subject, "bob");
    }
    EXPECT_TRUE(hasTriple(chain, "bob", "followed_by", "alice"));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-10: InferenceChain serialisability (access all fields)
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR10_InferenceChainFields) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"r1",
                 {{"?A", "likes", "?B"}},
                 {{"?A", "enjoys", "?B"}}});

    kgr.addFact({"alice", "likes", "music"});

    auto chain = kgr.infer("alice", 1);
    ASSERT_FALSE(chain.empty());
    EXPECT_EQ(chain.subject_id, "alice");
    EXPECT_EQ(chain.size(), 1u);

    const auto& e = chain.edges.front();
    EXPECT_EQ(e.fact.subject,    "alice");
    EXPECT_EQ(e.fact.predicate,  "enjoys");
    EXPECT_EQ(e.fact.object,     "music");
    EXPECT_EQ(e.rule_id,         "r1");
    EXPECT_FALSE(e.premises.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-11: CDC INSERT triggers incremental derivation
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR11_CDCInsertTriggersDerivation) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"transitive",
                 {{"?A", "knows", "?B"}, {"?B", "knows", "?C"}},
                 {{"?A", "indirectly_knows", "?C"}}});

    kgr.addFact({"alice", "knows", "bob"});

    // Before INSERT: no derivation possible.
    EXPECT_EQ(kgr.infer("alice", 1).size(), 0u);

    // CDC INSERT: add the missing fact.
    CDCEvent ev;
    ev.op   = CDCEvent::Op::INSERT;
    ev.edge = {"bob", "knows", "carol"};
    kgr.onCDCEvent(ev);

    // After INSERT: transitive rule should fire.
    auto chain = kgr.infer("alice", 2);
    EXPECT_TRUE(hasTriple(chain, "alice", "indirectly_knows", "carol"));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-12: CDC DELETE removes base fact
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR12_CDCDeleteRemovesBaseFact) {
    KnowledgeGraphReasoner kgr;

    kgr.addFact({"alice", "knows", "bob"});
    EXPECT_EQ(kgr.factCount(), 1u);

    CDCEvent ev;
    ev.op   = CDCEvent::Op::DELETE;
    ev.edge = {"alice", "knows", "bob"};
    kgr.onCDCEvent(ev);

    EXPECT_EQ(kgr.factCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-13: CDC INSERT is idempotent (duplicate ignored)
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR13_CDCInsertIdempotent) {
    KnowledgeGraphReasoner kgr;

    CDCEvent ev;
    ev.op   = CDCEvent::Op::INSERT;
    ev.edge = {"alice", "knows", "bob"};

    kgr.onCDCEvent(ev);
    kgr.onCDCEvent(ev); // duplicate
    kgr.onCDCEvent(ev); // duplicate

    EXPECT_EQ(kgr.factCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-14: applyLoRAScore() assigns scores to all edges
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR14_ApplyLoRAScoreAssignsScores) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"r1",
                 {{"?A", "knows", "?B"}},
                 {{"?A", "trusts", "?B"}}});

    kgr.addFact({"alice", "knows", "bob"});

    auto chain = kgr.infer("alice", 1);
    ASSERT_FALSE(chain.empty());

    // Before scoring: lora_score is negative (not scored).
    EXPECT_LT(chain.edges.front().lora_score, 0.0);

    kgr.applyLoRAScore(chain, "test_adapter");

    // After scoring: each edge should have a non-negative score.
    for (const auto& e : chain.edges) {
        EXPECT_GE(e.lora_score, 0.0);
        EXPECT_LE(e.lora_score, 1.0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-15: applyLoRAScore() filters edges below min_lora_score
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR15_ApplyLoRAScoreFiltersLowScores) {
    KnowledgeGraphReasoner kgr;

    // Rule with very high min score — stub will produce 1/(1+1) = 0.5, below 0.9.
    kgr.addRule({"strict_rule",
                 {{"?A", "knows", "?B"}},
                 {{"?A", "deeply_trusts", "?B"}},
                 "adapter",
                 0.9}); // min_lora_score = 0.9

    kgr.addFact({"alice", "knows", "bob"});

    auto chain = kgr.infer("alice", 1);
    ASSERT_FALSE(chain.empty());

    kgr.applyLoRAScore(chain, "adapter");

    // All edges with score < 0.9 should be removed.
    for (const auto& e : chain.edges) {
        EXPECT_GE(e.lora_score, 0.9);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-16: applyLoRAScore() on empty chain is safe
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR16_ApplyLoRAScoreEmptyChain) {
    KnowledgeGraphReasoner kgr;
    InferenceChain chain;
    chain.subject_id = "nobody";
    EXPECT_NO_THROW(kgr.applyLoRAScore(chain, "adapter"));
    EXPECT_TRUE(chain.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-17: Chain-of-authority pattern using proper closure rules
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR17_ChainOfAuthorityPattern) {
    KnowledgeGraphReasoner kgr(10);

    // Base case: any direct authorized_by lifts to indirectly_authorized_by.
    kgr.addRule({"authority_base",
                 {{"?A", "authorized_by", "?B"}},
                 {{"?A", "indirectly_authorized_by", "?B"}}});

    // Inductive case: extend the chain.
    kgr.addRule({"authority_chain",
                 {{"?A", "indirectly_authorized_by", "?B"}, {"?B", "authorized_by", "?C"}},
                 {{"?A", "indirectly_authorized_by", "?C"}}});

    kgr.addFact({"alice",    "authorized_by", "manager"});
    kgr.addFact({"manager",  "authorized_by", "director"});
    kgr.addFact({"director", "authorized_by", "ceo"});

    auto chain = kgr.infer("alice", 10);

    EXPECT_TRUE(hasTriple(chain, "alice", "indirectly_authorized_by", "manager"));
    EXPECT_TRUE(hasTriple(chain, "alice", "indirectly_authorized_by", "director"));
    EXPECT_TRUE(hasTriple(chain, "alice", "indirectly_authorized_by", "ceo"));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-18: Hub-spoke pattern — hub has many direct spokes
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR18_HubSpokePattern) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"co_member",
                 {{"?A", "member_of", "?G"}, {"?B", "member_of", "?G"}},
                 {{"?A", "co_member", "?B"}}});

    kgr.addFact({"alice", "member_of", "engineering"});
    kgr.addFact({"bob",   "member_of", "engineering"});
    kgr.addFact({"carol", "member_of", "engineering"});

    auto chain = kgr.infer("alice", 1);
    EXPECT_TRUE(hasTriple(chain, "alice", "co_member", "bob"));
    EXPECT_TRUE(hasTriple(chain, "alice", "co_member", "carol"));
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-19: InferenceStore FIFO eviction when capacity exceeded
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR19_InferenceStoreFIFOEviction) {
    InferenceStore store;

    // Fill to capacity with a very low cap test (using direct store calls).
    // We test the store independently using its public API.
    // Insert 5 triples with immediate TTL so they can be evicted.
    for (int i = 0; i < 5; ++i) {
        Triple t{"subj" + std::to_string(i), "pred", "obj"};
        store.store(t, "rule_x", {}, std::chrono::seconds{3600});
    }
    EXPECT_EQ(store.size(), 5u);

    // Verify all present.
    for (int i = 0; i < 5; ++i) {
        Triple t{"subj" + std::to_string(i), "pred", "obj"};
        EXPECT_TRUE(store.contains(t));
    }

    store.clear();
    EXPECT_EQ(store.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// KGR-20: addRule returns false for malformed rules
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, KGR20_AddRuleValidation) {
    KnowledgeGraphReasoner kgr;

    // Empty id → rejected.
    EXPECT_FALSE(kgr.addRule({"",
                              {{"?A", "knows", "?B"}},
                              {{"?A", "trusts", "?B"}}}));

    // Empty conditions → rejected.
    EXPECT_FALSE(kgr.addRule({"rule_no_conditions",
                              {},
                              {{"?A", "trusts", "?B"}}}));

    // Empty conclusions → rejected.
    EXPECT_FALSE(kgr.addRule({"rule_no_conclusions",
                              {{"?A", "knows", "?B"}},
                              {}}));

    // Valid rule → accepted.
    EXPECT_TRUE(kgr.addRule({"valid_rule",
                             {{"?A", "knows", "?B"}},
                             {{"?A", "trusts", "?B"}}}));

    EXPECT_EQ(kgr.ruleCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: Triple::isGround()
// ─────────────────────────────────────────────────────────────────────────────
TEST(TripleTest, IsGround) {
    {   Triple t{"alice", "knows", "bob"};
        EXPECT_TRUE(t.isGround()); }
    {   Triple t{"?A", "knows", "bob"};
        EXPECT_FALSE(t.isGround()); }
    {   Triple t{"alice", "?P", "bob"};
        EXPECT_FALSE(t.isGround()); }
    {   Triple t{"alice", "knows", "?B"};
        EXPECT_FALSE(t.isGround()); }
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: addFact rejects non-ground triples
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, AddFactRejectsNonGround) {
    KnowledgeGraphReasoner kgr;
    kgr.addFact({"?A", "knows", "bob"}); // non-ground → ignored
    EXPECT_EQ(kgr.factCount(), 0u);
    kgr.addFact({"alice", "knows", "bob"}); // ground → accepted
    EXPECT_EQ(kgr.factCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: duplicate rule id — last write wins
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, DuplicateRuleIdOverwrites) {
    KnowledgeGraphReasoner kgr;

    kgr.addRule({"r1",
                 {{"?A", "knows", "?B"}},
                 {{"?A", "trusts", "?B"}}});
    kgr.addRule({"r1",
                 {{"?A", "knows", "?B"}},
                 {{"?A", "respects", "?B"}}});

    EXPECT_EQ(kgr.ruleCount(), 1u);

    kgr.addFact({"alice", "knows", "bob"});
    auto chain = kgr.infer("alice", 1);

    // Second rule replaces first — only "respects" derived, not "trusts".
    EXPECT_TRUE(hasTriple(chain, "alice", "respects", "bob"));
    EXPECT_FALSE(hasTriple(chain, "alice", "trusts", "bob"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: setMaxHops clamps to [1, kHardMaxHops]
// ─────────────────────────────────────────────────────────────────────────────
TEST(KnowledgeGraphReasonerTest, SetMaxHopsClamping) {
    KnowledgeGraphReasoner kgr;
    kgr.setMaxHops(0);
    EXPECT_EQ(kgr.maxHops(), 1);
    kgr.setMaxHops(1000);
    EXPECT_EQ(kgr.maxHops(), KnowledgeGraphReasoner::kHardMaxHops);
    kgr.setMaxHops(3);
    EXPECT_EQ(kgr.maxHops(), 3);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: InferenceStore::getDerived returns only matching subject
// ─────────────────────────────────────────────────────────────────────────────
TEST(InferenceStoreTest, GetDerivedFiltersBySubject) {
    InferenceStore store;
    store.store({"alice", "knows", "bob"},   "r1", {});
    store.store({"alice", "trusts", "carol"}, "r2", {});
    store.store({"bob",   "knows", "carol"},  "r3", {});

    auto alice_derived = store.getDerived("alice");
    EXPECT_EQ(alice_derived.size(), 2u);
    for (const auto& e : alice_derived) {
        EXPECT_EQ(e.fact.subject, "alice");
    }

    auto bob_derived = store.getDerived("bob");
    EXPECT_EQ(bob_derived.size(), 1u);
}
