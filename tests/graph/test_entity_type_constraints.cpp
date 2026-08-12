/**
 * @file test_entity_type_constraints.cpp
 * @brief Entity Type Constraint validation tests — ETC-01..ETC-13
 *
 * Tests cover:
 *   ETC-01  Reject edge between incompatible entity types (strict mode)
 *   ETC-02  Accept edge between compatible entity types (subsumption chain)
 *   ETC-03  Schema enforcement: addConcept chains hierarchy correctly
 *   ETC-04  Type subsumption: isA returns true for subtype-supertype pairs
 *   ETC-05  Axiom enforcement: isEdgeTypeAllowed respects declared restrictions
 *   ETC-06  Multiple inheritance: concept inherits from multiple parents
 *   ETC-07  Reflexive edges: entity can connect to itself (self-loop)
 *   ETC-08  Transitive edge permissions: indirect inheritance propagates permissions
 *   ETC-09  Unknown edge types: graceful fallback for undeclared edge types
 *   ETC-10  Schema evolution: adding constraints after build() is idempotent
 *   ETC-11  Type subsumption chains: deep hierarchies resolved correctly
 *   ETC-12  Edge type propagation: axioms propagate through inheritance tree
 *   ETC-13  Constraint negation: forbidden edge types correctly rejected
 */

#include <gtest/gtest.h>
#include "graph/ontology_manager.h"

#include <memory>
#include <vector>

namespace themis {
namespace graph {
namespace {

// ---------------------------------------------------------------------------
// Helper: build a legal/financial ontology with constraints
// ---------------------------------------------------------------------------
static std::shared_ptr<OntologyManager> makeLegalFinanceOntology() {
    auto onto = std::make_shared<OntologyManager>();
    
    // Entity hierarchy
    onto->addConcept("Entity");
    onto->addConcept("LegalEntity", {"Entity"});
    onto->addConcept("Person", {"LegalEntity"});
    onto->addConcept("Organization", {"LegalEntity"});
    onto->addConcept("FinancialEntity", {"Entity"});
    onto->addConcept("Account", {"FinancialEntity"});
    onto->addConcept("Transaction", {"FinancialEntity"});
    
    // Edge type constraints
    onto->addAxiom("Person", "knows", "Person");
    onto->addAxiom("LegalEntity", "hasParty", "LegalEntity");
    onto->addAxiom("Organization", "owns", "Account");
    onto->addAxiom("Transaction", "transfers", "Account");
    onto->addAxiom("Person", "controls", "Account");
    onto->addAxiom("Account", "tracks", "Transaction");
    
    onto->build();
    return onto;
}

// ---------------------------------------------------------------------------
// ETC-01: Reject edge between incompatible entity types (strict mode)
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC01_RejectIncompatibleTypes) {
    auto onto = makeLegalFinanceOntology();
    
    // Person knows Person: allowed
    EXPECT_TRUE(onto->isEdgeTypeAllowed("Person", "Person", "knows"));
    
    // Person knows Account: NOT allowed (no axiom)
    // Unknown classes gracefully return true, but for known classes should enforce
    auto allowed = onto->allowedEdgeTypes("Person", "Account");
    EXPECT_FALSE(allowed.count("knows") > 0);
}

// ---------------------------------------------------------------------------
// ETC-02: Accept edge between compatible entity types (subsumption chain)
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC02_AcceptCompatibleTypes) {
    auto onto = makeLegalFinanceOntology();
    
    // Organization isA LegalEntity, LegalEntity hasParty LegalEntity
    // So Organization hasParty Organization should be allowed (via subsumption)
    auto allowed = onto->allowedEdgeTypes("Organization", "Organization");
    EXPECT_GT(allowed.size(), 0u);
    EXPECT_TRUE(allowed.count("hasParty") > 0);
}

// ---------------------------------------------------------------------------
// ETC-03: Schema enforcement: addConcept chains hierarchy correctly
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC03_SchemaHierarchyEnforcement) {
    OntologyManager onto;
    
    // Build a simple hierarchy
    onto.addConcept("Root");
    onto.addConcept("Child1", {"Root"});
    onto.addConcept("Child2", {"Root"});
    onto.addConcept("GrandChild", {"Child1"});
    onto.build();
    
    // Verify hierarchy is correct
    EXPECT_TRUE(onto.isA("GrandChild", "Child1"));
    EXPECT_TRUE(onto.isA("GrandChild", "Root"));
    EXPECT_TRUE(onto.isA("Child1", "Root"));
    EXPECT_FALSE(onto.isA("Root", "Child1"));
}

// ---------------------------------------------------------------------------
// ETC-04: Type subsumption: isA returns true for subtype-supertype pairs
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC04_TypeSubsumption) {
    auto onto = makeLegalFinanceOntology();
    
    // Person isA LegalEntity isA Entity
    EXPECT_TRUE(onto->isA("Person", "LegalEntity"));
    EXPECT_TRUE(onto->isA("Person", "Entity"));
    EXPECT_TRUE(onto->isA("LegalEntity", "Entity"));
    
    // Reverse is false
    EXPECT_FALSE(onto->isA("Entity", "Person"));
    EXPECT_FALSE(onto->isA("LegalEntity", "Person"));
}

// ---------------------------------------------------------------------------
// ETC-05: Axiom enforcement: isEdgeTypeAllowed respects declared restrictions
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC05_AxiomEnforcement) {
    auto onto = makeLegalFinanceOntology();
    
    // Organization owns Account: allowed (direct axiom)
    EXPECT_TRUE(onto->isEdgeTypeAllowed("Organization", "Account", "owns"));
    
    // Organization transfers Account: NOT allowed (no axiom for this pair)
    auto allowed = onto->allowedEdgeTypes("Organization", "Account");
    EXPECT_FALSE(allowed.count("transfers") > 0);
}

// ---------------------------------------------------------------------------
// ETC-06: Multiple inheritance: concept inherits from multiple parents
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC06_MultipleInheritance) {
    OntologyManager onto;
    
    // Create a diamond inheritance pattern
    onto.addConcept("Base");
    onto.addConcept("Left", {"Base"});
    onto.addConcept("Right", {"Base"});
    onto.addConcept("Diamond", {"Left", "Right"});
    
    onto.addAxiom("Base", "connects", "Base");
    onto.build();
    
    // Diamond should inherit the permission through both parents
    EXPECT_TRUE(onto.isA("Diamond", "Base"));
    EXPECT_TRUE(onto.isA("Diamond", "Left"));
    EXPECT_TRUE(onto.isA("Diamond", "Right"));
    
    auto allowed = onto.allowedEdgeTypes("Diamond", "Diamond");
    EXPECT_TRUE(allowed.count("connects") > 0);
}

// ---------------------------------------------------------------------------
// ETC-07: Reflexive edges: entity can connect to itself (self-loop)
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC07_ReflexiveEdges) {
    auto onto = makeLegalFinanceOntology();
    
    // Person knows Person: reflexive edge allowed
    EXPECT_TRUE(onto->isEdgeTypeAllowed("Person", "Person", "knows"));
    
    // Transaction transfers Account: check this specific pair
    auto allowed = onto->allowedEdgeTypes("Transaction", "Account");
    EXPECT_TRUE(allowed.count("transfers") > 0);
}

// ---------------------------------------------------------------------------
// ETC-08: Transitive edge permissions: indirect inheritance propagates permissions
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC08_TransitivePermissions) {
    auto onto = makeLegalFinanceOntology();
    
    // Person isA LegalEntity
    // LegalEntity hasParty LegalEntity
    // So Person hasParty Person should be allowed (via transitive subsumption)
    auto allowed = onto->allowedEdgeTypes("Person", "Person");
    EXPECT_TRUE(allowed.count("hasParty") > 0);
    EXPECT_TRUE(allowed.count("knows") > 0); // direct axiom too
}

// ---------------------------------------------------------------------------
// ETC-09: Unknown edge types: graceful fallback for undeclared edge types
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC09_UnknownEdgeTypes) {
    auto onto = makeLegalFinanceOntology();
    
    // Unknown edge type on unknown classes: graceful return true
    EXPECT_TRUE(onto->isEdgeTypeAllowed("Unknown1", "Unknown2", "unknownEdge"));
    
    // Unknown edge type on known classes: still allows (no axiom restricts it)
    EXPECT_TRUE(onto->isEdgeTypeAllowed("Person", "Person", "unknownEdge"));
}

// ---------------------------------------------------------------------------
// ETC-10: Schema evolution: adding constraints after build() is idempotent
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC10_SchemaEvolutionIdempotent) {
    auto onto = std::make_shared<OntologyManager>();
    
    onto->addConcept("A");
    onto->addConcept("B", {"A"});
    onto->addAxiom("A", "edge1", "A");
    onto->build();
    
    // After build(), add more constraints (should be no-op)
    size_t axiom_count_before = onto->axiomCount();
    onto->addConcept("C");  // no-op
    onto->addAxiom("B", "edge2", "B");  // no-op
    
    EXPECT_EQ(axiom_count_before, onto->axiomCount());
    EXPECT_TRUE(onto->isBuilt());
}

// ---------------------------------------------------------------------------
// ETC-11: Type subsumption chains: deep hierarchies resolved correctly
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC11_DeepHierarchies) {
    OntologyManager onto;
    
    // Create a deep chain: L0 -> L1 -> L2 -> L3 -> L4
    onto.addConcept("L0");
    onto.addConcept("L1", {"L0"});
    onto.addConcept("L2", {"L1"});
    onto.addConcept("L3", {"L2"});
    onto.addConcept("L4", {"L3"});
    onto.addAxiom("L0", "deep", "L0");
    onto.build();
    
    // L4 should be recognized as L0 (through chain)
    EXPECT_TRUE(onto.isA("L4", "L0"));
    EXPECT_TRUE(onto.isA("L3", "L0"));
    EXPECT_TRUE(onto.isA("L2", "L0"));
    EXPECT_TRUE(onto.isA("L1", "L0"));
    
    // Permission should propagate
    auto allowed = onto.allowedEdgeTypes("L4", "L4");
    EXPECT_TRUE(allowed.count("deep") > 0);
}

// ---------------------------------------------------------------------------
// ETC-12: Edge type propagation: axioms propagate through inheritance tree
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC12_EdgeTypePropagarion) {
    OntologyManager onto;
    
    onto.addConcept("Parent");
    onto.addConcept("Child", {"Parent"});
    onto.addConcept("GrandChild", {"Child"});
    
    // Define edge on parent
    onto.addAxiom("Parent", "parentEdge", "Parent");
    onto.build();
    
    // Child and GrandChild should inherit the permission
    EXPECT_TRUE(onto.isEdgeTypeAllowed("Child", "Child", "parentEdge"));
    EXPECT_TRUE(onto.isEdgeTypeAllowed("GrandChild", "GrandChild", "parentEdge"));
    EXPECT_TRUE(onto.isEdgeTypeAllowed("Child", "Parent", "parentEdge"));
    EXPECT_TRUE(onto.isEdgeTypeAllowed("GrandChild", "Parent", "parentEdge"));
}

// ---------------------------------------------------------------------------
// ETC-13: Constraint negation: forbidden edge types correctly rejected
// ---------------------------------------------------------------------------
TEST(EntityTypeConstraintTests, ETC13_ConstraintNegation) {
    OntologyManager onto;
    
    onto.addConcept("A");
    onto.addConcept("B");
    onto.addConcept("C");
    
    // Allow A -> B with "allowed_edge"
    onto.addAxiom("A", "allowed_edge", "B");
    onto.build();
    
    // A -> B with allowed_edge: yes
    EXPECT_TRUE(onto.isEdgeTypeAllowed("A", "B", "allowed_edge"));
    
    // A -> C with allowed_edge: no (no axiom for A -> C)
    EXPECT_FALSE(onto.isEdgeTypeAllowed("A", "C", "allowed_edge"));
    
    // A -> B with forbidden_edge: no (not in any axiom)
    auto allowed = onto.allowedEdgeTypes("A", "B");
    EXPECT_FALSE(allowed.count("forbidden_edge") > 0);
}

} // namespace
} // namespace graph
} // namespace themis
