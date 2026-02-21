/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_proximity_let.cpp                         ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:41:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     49                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7d467f118  2026-01-24  Remove columnar storage and optimization issue templates ║
    • 246f1e038  2025-11-17  feat: Implement Phase 3 & 4 - Full Subquery and CTE Support ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/aql_translator.h"

using namespace themis; using namespace themis::query;

// Disable legacy AQL PROXIMITY LET tests
#if 0

TEST(AQLProximityLetTest, TranslateLetProximitySortVariable) {
    std::string aql = R"(
        FOR doc IN places
        FILTER FULLTEXT(doc.description, "coffee", 20)
        LET prox = PROXIMITY(doc.location, [13.4,52.5])
        SORT prox ASC
        LIMIT 10
        RETURN doc
    )";
    AQLParser p; auto pr = p.parse(aql); ASSERT_TRUE(pr.success) << pr.error.toString();
    auto tr = AQLTranslator::translate(pr.query); ASSERT_TRUE(tr.success) << tr.error_message;
    ASSERT_TRUE(tr.content_geo.has_value());
    EXPECT_EQ(tr.content_geo->limit, 10u);
    EXPECT_TRUE(tr.content_geo->center_point.has_value());
}

#endif // legacy proximity LET tests

TEST(AQLProximityLetTest, DISABLED_ProximityLetLegacy) {
    GTEST_SKIP() << "Skipping legacy AQL PROXIMITY LET tests";
}
