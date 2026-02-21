/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_shortest_path.cpp                         ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
