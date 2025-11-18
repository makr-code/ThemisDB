#include "gtest/gtest.h"
#include "query/aql_parser.h"
#include "query/aql_translator.h"

using namespace themis;
using namespace themis::query;

TEST(AqlTraversalConstraints, ParseAndTranslateConstraints) {
    AQLParser p;
    auto res = p.parse(
        "FOR v IN 1..3 OUTBOUND 'u/1' TYPE 'friend' GRAPH 'g' "
        "NO_BACKTRACK EDGE_LABEL_WHITELIST ['friend','colleague'] NODE_LABEL_BLACKLIST ['Bot'] RETURN v"
    );
    ASSERT_TRUE(res.success);
    ASSERT_TRUE(res.query);
    ASSERT_TRUE(res.query->traversal);
    auto t = res.query->traversal;
    EXPECT_TRUE(t->noBacktrack);
    ASSERT_EQ(t->edgeLabelWhitelist.size(), 2u);
    EXPECT_EQ(t->edgeLabelWhitelist[0], "friend");
    EXPECT_EQ(t->edgeLabelWhitelist[1], "colleague");
    ASSERT_EQ(t->nodeLabelBlacklist.size(), 1u);
    EXPECT_EQ(t->nodeLabelBlacklist[0], "Bot");

    auto tr = AQLTranslator::translate(res.query);
    ASSERT_TRUE(tr.success);
    ASSERT_TRUE(tr.traversal.has_value());
    auto tv = tr.traversal.value();
    EXPECT_TRUE(tv.noBacktrack);
    ASSERT_EQ(tv.edgeLabelWhitelist.size(), 2u);
    ASSERT_EQ(tv.nodeLabelBlacklist.size(), 1u);
}
