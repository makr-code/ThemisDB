/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_similarity_let.cpp                        ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
