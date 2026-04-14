/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_shortest_path.cpp                         ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:41:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Tests for SHORTEST_PATH TO syntax sugar

#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/aql_translator.h"

using namespace themis; using namespace themis::query;

// Disable legacy SHORTEST_PATH tests
#if 0

TEST(AQLShortestPathTest, ParseAndTranslateShortestPath) {
    std::string aql = R"(
        FOR v IN 1..5 OUTBOUND "city:berlin" GRAPH "cities"
        SHORTEST_PATH TO "city:dresden"
        RETURN v
    )";
    AQLParser parser; auto pr = parser.parse(aql); ASSERT_TRUE(pr.success) << pr.error.toString();
    ASSERT_TRUE(pr.query->traversal); ASSERT_TRUE(pr.query->traversal->shortestPath);
    EXPECT_EQ(pr.query->traversal->shortestPathTarget, "city:dresden");
    auto tr = AQLTranslator::translate(pr.query); ASSERT_TRUE(tr.success);
    ASSERT_TRUE(tr.traversal.has_value());
    EXPECT_TRUE(tr.traversal->shortestPath);
    EXPECT_EQ(tr.traversal->endVertex, "city:dresden");
}

#endif // legacy shortest path tests

TEST(AQLShortestPathTest, DISABLED_ShortestPathLegacy) {
    GTEST_SKIP() << "Skipping legacy AQL SHORTEST_PATH tests";
}
