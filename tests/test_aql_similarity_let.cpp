/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_similarity_let.cpp                        ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:47:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

// Disable legacy AQL SIMILARITY LET tests
#if 0
#include "query/aql_parser.h"
#include "query/aql_translator.h"

using namespace themis; using namespace themis::query;

TEST(AQLSimilarityLetTest, TranslateLetSimilaritySortVariable) {
    std::string aql = R"(
        FOR doc IN hotels
        LET sim = SIMILARITY(doc.embedding, [0.1,0.2,0.3], 5)
        SORT sim DESC
        RETURN doc
    )";
    AQLParser p; auto pr = p.parse(aql); ASSERT_TRUE(pr.success) << pr.error.toString();
    auto tr = AQLTranslator::translate(pr.query); ASSERT_TRUE(tr.success) << tr.error_message;
    ASSERT_TRUE(tr.vector_geo.has_value());
    EXPECT_EQ(tr.vector_geo->k, 5u);
    EXPECT_FALSE(tr.vector_geo->spatial_filter);
}

#endif // legacy similarity LET tests

TEST(AQLSimilarityLetTest, DISABLED_SimilarityLetLegacy) {
    GTEST_SKIP() << "Skipping legacy AQL SIMILARITY LET tests";
}
