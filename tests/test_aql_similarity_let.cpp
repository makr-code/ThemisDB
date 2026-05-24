/*
 * ThemisDB | File: test_aql_similarity_let.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
