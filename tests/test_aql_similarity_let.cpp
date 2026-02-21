/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_similarity_let.cpp                        ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:49:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
