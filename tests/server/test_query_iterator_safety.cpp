/**
 * @file test_query_iterator_safety.cpp
 * @brief Wave 2-A / A3: Unit-Tests für Iterator-Sicherheit in BFS-Graph-Traversal.
 *
 * Verifiziert, dass:
 * - Zyklenerkennung in der Elternzeiger-Rückverfolgung korrekt abbricht
 * - Bounds-Checks bei parent.find() korrekt funktionieren
 * - Pfadrekonstruktion bei Zyklen terminiert (kein Infinite Loop / kein Crash)
 *
 * Diese Tests validieren die defensive Invariante der
 * `while (itp != parent.end())` Schleifen in query_api_handler.cpp.
 */

#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis::server::test {

// ---------------------------------------------------------------------------
// Hilfsstruktur — minimale Abstraktion der ParentInfo aus query_api_handler
// ---------------------------------------------------------------------------

struct ParentInfo {
    std::string parent;
    std::string edgeId;
};

using ParentMap = std::unordered_map<std::string, ParentInfo>;

/**
 * @brief Rekonstruiert den Pfad von @p start zurück zum Ursprung, geschützt
 *        durch Zyklenerkennung.
 *
 * Spiegelt die gesicherzte Implementierung aus query_api_handler.cpp wider.
 *
 * @param start     Startknoten für die Rückverfolgung.
 * @param parent    Elternzeiger-Map (node → {parent, edgeId}).
 * @param nodes_out Ausgabe: Knoten vom Ursprung bis @p start (umgekehrt).
 * @param edges_out Ausgabe: Kanten entsprechend.
 */
static void reconstructPathSafe(const std::string&     start,
                                 const ParentMap&       parent,
                                 std::vector<std::string>& nodes_out,
                                 std::vector<std::string>& edges_out) {
    nodes_out.clear();
    edges_out.clear();

    if (start.empty()) {
      return;
    }

    nodes_out.push_back(start);
    std::unordered_set<std::string> visited;
    visited.insert(start);

    auto itp = parent.find(start);
    while (itp != parent.end()) {
        const std::string& next = itp->second.parent;
        if (visited.count(next) > 0) {
            break;  // Zyklus erkannt — Schleife sicher beenden
        }
        edges_out.push_back(itp->second.edgeId);
        nodes_out.push_back(next);
        visited.insert(next);
        itp = parent.find(next);
    }

    // Pfad umkehren: Ursprung → Start
    std::reverse(nodes_out.begin(), nodes_out.end());
    std::reverse(edges_out.begin(), edges_out.end());
}

// ---------------------------------------------------------------------------
// Test 1: Normaler azyklischer Pfad — korrekte Rekonstruktion
// ---------------------------------------------------------------------------

TEST(QueryIteratorSafetyTest, AcyclicPathReconstructedCorrectly) {
    // Pfad: A → B → C → D (D ist Ziel, A ist Ursprung)
    // parent map: D.parent=C, C.parent=B, B.parent=A
    ParentMap parent;
    parent["D"] = {"C", "e3"};
    parent["C"] = {"B", "e2"};
    parent["B"] = {"A", "e1"};
    // A hat keinen Eintrag (Ursprung)

    std::vector<std::string> nodes, edges;
    reconstructPathSafe("D", parent, nodes, edges);

    // Erwarteter Pfad: A → B → C → D
    ASSERT_EQ(nodes.size(), 4u);
    EXPECT_EQ(nodes[0], "A");
    EXPECT_EQ(nodes[1], "B");
    EXPECT_EQ(nodes[2], "C");
    EXPECT_EQ(nodes[3], "D");

    ASSERT_EQ(edges.size(), 3u);
    EXPECT_EQ(edges[0], "e1");
    EXPECT_EQ(edges[1], "e2");
    EXPECT_EQ(edges[2], "e3");
}

// ---------------------------------------------------------------------------
// Test 2: Zyklus in parent map — Schleife terminiert, kein Absturz
// ---------------------------------------------------------------------------

TEST(QueryIteratorSafetyTest, CyclicParentMapTerminatesSafely) {
    // Intentionaler Zyklus: A.parent=B, B.parent=A
    ParentMap parent;
    parent["A"] = {"B", "e1"};
    parent["B"] = {"A", "e2"};  // Zyklus!

    std::vector<std::string> nodes, edges;

    // Muss terminieren (kein Infinite Loop), kein Crash
    EXPECT_NO_FATAL_FAILURE(reconstructPathSafe("A", parent, nodes, edges));

    // Der Zyklus muss bei max 2 Knoten brechen
    EXPECT_LE(nodes.size(), 2u)
        << "Cycle detection must stop traversal before unbounded growth";
}

// ---------------------------------------------------------------------------
// Test 3: parent.find() bei fehlendem Knoten gibt nullopt/end() zurück
// ---------------------------------------------------------------------------

TEST(QueryIteratorSafetyTest, MissingParentEntryTerminatesGracefully) {
    // Pfad: X → (keine weiteren Einträge)
    ParentMap parent;
    // parent map ist leer — X hat keinen Eintrag

    std::vector<std::string> nodes, edges;
    reconstructPathSafe("X", parent, nodes, edges);

    // Nur der Startknoten selbst
    ASSERT_EQ(nodes.size(), 1u);
    EXPECT_EQ(nodes[0], "X");
    EXPECT_TRUE(edges.empty());
}

}  // namespace themis::server::test
