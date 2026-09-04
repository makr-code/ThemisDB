/**
 * @file test_ontology_manager.cpp
 * @brief OntologyManager focused tests — OM-01..OM-12
 *
 * Tests cover:
 *   OM-01  loadFromJsonString round-trip equality (toJson → loadFromJson → toJson)
 *   OM-02  isA transitive closure (3-hop hierarchy)
 *   OM-03  allowedEdgeTypes returns correct set for known source/target pair
 *   OM-04  unknown concept → unconstrained (no throw, isA returns false, allowedEdgeTypes empty)
 *   OM-05  thread-safety: 16 concurrent isA calls on shared OntologyManager
 *   OM-06  build() seals the object; addConcept after build() is a no-op
 *   OM-07  isEdgeTypeAllowed returns true for unknown classes (graceful degradation)
 *   OM-08  loadFromYaml (inline string via temp file) produces same result as JSON
 *   OM-09  hasConcept / getConcept introspection
 *   OM-10  isA returns false for unrelated concepts in the same hierarchy
 *   OM-11  allowedEdgeTypes inherits through subclass axioms (indirect inheritance)
 *   OM-12  toYaml round-trip (toYaml → loadFromYaml → same concept/axiom counts)
 */

#include <gtest/gtest.h>
#include "graph/ontology_manager.h"

#include <thread>
#include <vector>
#include <atomic>
#include <fstream>
#include <filesystem>

namespace themis {
namespace graph {
namespace {

// ---------------------------------------------------------------------------
// Helper: build a small legal ontology
// ---------------------------------------------------------------------------
static std::shared_ptr<OntologyManager> makeLegalOntology() {
    auto onto = std::make_shared<OntologyManager>();
    onto->addConcept("Entity");
    onto->addConcept("LegalEntity", {"Entity"});
    onto->addConcept("Person",       {"LegalEntity"});
    onto->addConcept("Organization", {"LegalEntity"});
    onto->addConcept("Case");
    onto->addConcept("Statute");
    onto->addAxiom("LegalEntity", "hasParty",   "LegalEntity");
    onto->addAxiom("Case",        "ruledBy",     "Statute");
    onto->addAxiom("Person",      "knows",       "Person");
    onto->build();
    return onto;
}

static const std::string kLegalJson = R"({
  "concepts": [
    {"id": "Entity"},
    {"id": "LegalEntity", "parents": ["Entity"]},
    {"id": "Person",      "parents": ["LegalEntity"]},
    {"id": "Organization","parents": ["LegalEntity"]},
    {"id": "Case"},
    {"id": "Statute"}
  ],
  "axioms": [
    {"source_class": "LegalEntity", "edge_type": "hasParty",  "target_class": "LegalEntity"},
    {"source_class": "Case",        "edge_type": "ruledBy",   "target_class": "Statute"},
    {"source_class": "Person",      "edge_type": "knows",     "target_class": "Person"}
  ]
})";

static const std::string kLegalYaml = R"(concepts:
  - id: Entity
  - id: LegalEntity
    parents:
      - Entity
  - id: Person
    parents:
      - LegalEntity
  - id: Organization
    parents:
      - LegalEntity
  - id: Case
  - id: Statute
axioms:
  - source_class: LegalEntity
    edge_type: hasParty
    target_class: LegalEntity
  - source_class: Case
    edge_type: ruledBy
    target_class: Statute
  - source_class: Person
    edge_type: knows
    target_class: Person
)";

// ---------------------------------------------------------------------------
// OM-01: loadFromJsonString round-trip equality
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM01_JsonRoundTrip) {
    OntologyManager onto;
    ASSERT_TRUE(onto.loadFromJsonString(kLegalJson));
    onto.build();

    std::string json1 = onto.toJson();

    OntologyManager onto2;
    ASSERT_TRUE(onto2.loadFromJsonString(json1));
    onto2.build();

    EXPECT_EQ(onto.conceptCount(), onto2.conceptCount());
    EXPECT_EQ(onto.axiomCount(),   onto2.axiomCount());
    EXPECT_TRUE(onto2.hasConcept("Person"));
    EXPECT_TRUE(onto2.hasConcept("Statute"));
}

// ---------------------------------------------------------------------------
// OM-02: isA transitive closure — 3-hop hierarchy
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM02_IsATransitiveClosure) {
    auto onto = makeLegalOntology();

    // Direct
    EXPECT_TRUE(onto->isA("Person", "Person"));
    EXPECT_TRUE(onto->isA("Person", "LegalEntity"));
    // Transitive (Person → LegalEntity → Entity)
    EXPECT_TRUE(onto->isA("Person", "Entity"));
    // Organisation also derives from Entity
    EXPECT_TRUE(onto->isA("Organization", "Entity"));
    // No upward relationship
    EXPECT_FALSE(onto->isA("Entity", "Person"));
    EXPECT_FALSE(onto->isA("LegalEntity", "Person"));
}

// ---------------------------------------------------------------------------
// OM-03: allowedEdgeTypes returns correct set
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM03_AllowedEdgeTypes) {
    auto onto = makeLegalOntology();

    // Person isA LegalEntity, so "hasParty" should be allowed P→P
    auto allowed = onto->allowedEdgeTypes("Person", "Person");
    EXPECT_GT(allowed.size(), 0u);
    EXPECT_TRUE(allowed.count("hasParty"));  // inherited via LegalEntity
    EXPECT_TRUE(allowed.count("knows"));     // direct axiom Person→Person

    // Case → Statute only gets "ruledBy"
    auto case_stat = onto->allowedEdgeTypes("Case", "Statute");
    EXPECT_EQ(case_stat.size(), 1u);
    EXPECT_TRUE(case_stat.count("ruledBy"));
}

// ---------------------------------------------------------------------------
// OM-04: unknown concept → unconstrained
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM04_UnknownConceptGracefulDegradation) {
    auto onto = makeLegalOntology();

    // isA with unknown concept must not throw
    EXPECT_NO_THROW({
        bool r = onto->isA("NonExistentConcept", "Entity");
        EXPECT_FALSE(r);
    });

    // allowedEdgeTypes with unknown class returns empty
    auto allowed = onto->allowedEdgeTypes("UnknownClass", "Person");
    EXPECT_TRUE(allowed.empty());
}

// ---------------------------------------------------------------------------
// OM-05: thread-safety — 16 concurrent isA calls
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM05_ThreadSafetyIsA) {
    auto onto = makeLegalOntology();

    constexpr int kThreads = 16;
    constexpr int kIterations = 200;
    std::vector<std::thread> threads;
    std::atomic<int> error_count{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&onto, &error_count, t]() {
            for (int i = 0; i < kIterations; ++i) {
                bool r = (t % 2 == 0)
                    ? onto->isA("Person", "Entity")
                    : onto->isA("Organization", "LegalEntity");
                if (!r) {
                  ++error_count;
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    EXPECT_EQ(error_count.load(), 0);
}

// ---------------------------------------------------------------------------
// OM-06: build() seals; addConcept after build() is no-op
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM06_BuildSeals) {
    auto onto = makeLegalOntology();
    EXPECT_TRUE(onto->isBuilt());
    EXPECT_EQ(onto->conceptCount(), 6u);

    // Adding after build should be ignored
    onto->addConcept("NewConcept");
    EXPECT_EQ(onto->conceptCount(), 6u);  // unchanged
}

// ---------------------------------------------------------------------------
// OM-07: isEdgeTypeAllowed — unknown classes → unconstrained (returns true)
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM07_UnknownClassUnconstrained) {
    auto onto = makeLegalOntology();

    // Unknown source or target class → true (graceful degradation)
    EXPECT_TRUE(onto->isEdgeTypeAllowed("Unknown", "Person", "hasParty"));
    EXPECT_TRUE(onto->isEdgeTypeAllowed("Person",  "Unknown", "hasParty"));
}

// ---------------------------------------------------------------------------
// OM-08: loadFromYaml produces same result as JSON
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM08_YamlLoad) {
    // Write YAML to a temp file
    auto tmp = std::filesystem::temp_directory_path() / "test_legal_ontology.yaml";
    {
        std::ofstream f(tmp);
        f << kLegalYaml;
    }

    OntologyManager onto;
    ASSERT_TRUE(onto.loadFromYaml(tmp.string()));
    onto.build();

    EXPECT_EQ(onto.conceptCount(), 6u);
    EXPECT_EQ(onto.axiomCount(),   3u);
    EXPECT_TRUE(onto.isA("Person", "Entity"));

    std::filesystem::remove(tmp);
}

// ---------------------------------------------------------------------------
// OM-09: hasConcept / getConcept introspection
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM09_Introspection) {
    auto onto = makeLegalOntology();

    EXPECT_TRUE(onto->hasConcept("Person"));
    EXPECT_FALSE(onto->hasConcept("Robot"));

    const auto* node = onto->getConcept("Person");
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->id, "Person");
    ASSERT_EQ(node->parents.size(), 1u);
    EXPECT_EQ(node->parents[0], "LegalEntity");

    EXPECT_EQ(onto->getConcept("Robot"), nullptr);
}

// ---------------------------------------------------------------------------
// OM-10: isA returns false for siblings (unrelated concepts)
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM10_SiblingsNotRelated) {
    auto onto = makeLegalOntology();

    EXPECT_FALSE(onto->isA("Person", "Organization"));
    EXPECT_FALSE(onto->isA("Organization", "Person"));
    EXPECT_FALSE(onto->isA("Case", "Statute"));
    EXPECT_FALSE(onto->isA("Statute", "Case"));
}

// ---------------------------------------------------------------------------
// OM-11: allowedEdgeTypes inherits via sub-class axioms
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM11_InheritedEdgeTypesViaSubclass) {
    auto onto = makeLegalOntology();

    // Organisation isA LegalEntity → hasParty should be allowed O→O
    auto allowed = onto->allowedEdgeTypes("Organization", "Organization");
    EXPECT_TRUE(allowed.count("hasParty"));
    EXPECT_FALSE(allowed.count("knows")); // knows is only Person→Person
}

// ---------------------------------------------------------------------------
// OM-12: toYaml round-trip
// ---------------------------------------------------------------------------
TEST(OntologyManagerFocusedTests, OM12_YamlRoundTrip) {
    auto onto = makeLegalOntology();

    std::string yaml = onto->toYaml();

    // Write to temp file and reload
    auto tmp = std::filesystem::temp_directory_path() / "test_ontology_rt.yaml";
    {
        std::ofstream f(tmp);
        f << yaml;
    }

    OntologyManager onto2;
    ASSERT_TRUE(onto2.loadFromYaml(tmp.string()));
    onto2.build();

    EXPECT_EQ(onto->conceptCount(), onto2.conceptCount());
    EXPECT_EQ(onto->axiomCount(),   onto2.axiomCount());
    EXPECT_TRUE(onto2.hasConcept("Case"));
    EXPECT_TRUE(onto2.isA("Person", "Entity"));

    std::filesystem::remove(tmp);
}

} // namespace
} // namespace graph
} // namespace themis
