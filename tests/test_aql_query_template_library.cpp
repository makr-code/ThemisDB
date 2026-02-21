/**
 * @file test_aql_query_template_library.cpp
 * @brief Unit tests for AQLQueryTemplateLibrary
 */

#include <gtest/gtest.h>
#include "aql/aql_query_template_library.h"

using namespace themis::aql;

// ============================================================================
// Fixture
// ============================================================================

class AQLQueryTemplateLibraryTest : public ::testing::Test {
protected:
    AQLQueryTemplateLibrary lib;
};

// ============================================================================
// Built-in template registration
// ============================================================================

TEST_F(AQLQueryTemplateLibraryTest, BuiltinsAreRegistered) {
    EXPECT_FALSE(lib.all().empty());
}

TEST_F(AQLQueryTemplateLibraryTest, SimpleScanTemplateExists) {
    const auto* tmpl = lib.findById("simple_scan");
    ASSERT_NE(tmpl, nullptr);
    EXPECT_EQ(tmpl->id, "simple_scan");
    EXPECT_FALSE(tmpl->template_body.empty());
}

TEST_F(AQLQueryTemplateLibraryTest, AllBuiltinIdsAreUnique) {
    const auto& all = lib.all();
    std::unordered_map<std::string, int> counts;
    for (const auto& t : all) {
        ++counts[t.id];
    }
    for (const auto& [id, cnt] : counts) {
        EXPECT_EQ(cnt, 1) << "duplicate id: " << id;
    }
}

TEST_F(AQLQueryTemplateLibraryTest, AllBuiltinTemplatesHaveParameters) {
    for (const auto& tmpl : lib.all()) {
        EXPECT_FALSE(tmpl.parameters.empty())
            << "template " << tmpl.id << " has no parameters list";
    }
}

// ============================================================================
// Lookup
// ============================================================================

TEST_F(AQLQueryTemplateLibraryTest, FindByIdReturnsNullForUnknown) {
    EXPECT_EQ(lib.findById("no_such_template"), nullptr);
}

TEST_F(AQLQueryTemplateLibraryTest, FindByTagGraphReturnsGraphTemplates) {
    auto results = lib.findByTag("graph");
    EXPECT_GE(results.size(), 1u);
    for (const auto& t : results) {
        bool has_tag = false;
        for (const auto& tag : t.tags) {
            if (tag == "graph") { has_tag = true; break; }
        }
        EXPECT_TRUE(has_tag) << "template " << t.id << " missing 'graph' tag";
    }
}

TEST_F(AQLQueryTemplateLibraryTest, FindByTagCaseInsensitive) {
    auto lower = lib.findByTag("graph");
    auto upper = lib.findByTag("GRAPH");
    EXPECT_EQ(lower.size(), upper.size());
}

TEST_F(AQLQueryTemplateLibraryTest, FindByTagUnknownReturnsEmpty) {
    auto results = lib.findByTag("xyzzy_nonexistent_tag");
    EXPECT_TRUE(results.empty());
}

TEST_F(AQLQueryTemplateLibraryTest, SearchByKeywordFindsMatch) {
    auto results = lib.search("traversal");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AQLQueryTemplateLibraryTest, SearchCaseInsensitive) {
    auto lower = lib.search("vector");
    auto upper = lib.search("VECTOR");
    EXPECT_EQ(lower.size(), upper.size());
}

TEST_F(AQLQueryTemplateLibraryTest, SearchNoMatchReturnsEmpty) {
    auto results = lib.search("xyzzy_no_match_7439");
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Custom registration
// ============================================================================

TEST_F(AQLQueryTemplateLibraryTest, RegisterCustomTemplate) {
    AQLQueryTemplate custom;
    custom.id            = "my_custom";
    custom.name          = "My Custom Query";
    custom.description   = "A custom template for testing";
    custom.template_body = "FOR {{var}} IN {{coll}} RETURN {{var}}";
    custom.tags          = {"custom"};
    custom.parameters    = {"var", "coll"};

    EXPECT_NO_THROW(lib.registerTemplate(custom));

    const auto* found = lib.findById("my_custom");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "My Custom Query");
}

TEST_F(AQLQueryTemplateLibraryTest, RegisterDuplicateIdThrows) {
    AQLQueryTemplate dup;
    dup.id            = "simple_scan"; // already registered
    dup.template_body = "FOR x IN y RETURN x";
    dup.parameters    = {"x"};

    EXPECT_THROW(lib.registerTemplate(dup), std::invalid_argument);
}

TEST_F(AQLQueryTemplateLibraryTest, RegisterEmptyIdThrows) {
    AQLQueryTemplate empty_id;
    empty_id.id            = "";
    empty_id.template_body = "FOR x IN y RETURN x";

    EXPECT_THROW(lib.registerTemplate(empty_id), std::invalid_argument);
}

// ============================================================================
// Instantiation
// ============================================================================

TEST_F(AQLQueryTemplateLibraryTest, InstantiateSimpleScan) {
    std::string aql = lib.instantiate("simple_scan", {
        {"var",        "doc"},
        {"collection", "documents"}
    });
    EXPECT_NE(aql.find("FOR doc IN documents"), std::string::npos);
    EXPECT_NE(aql.find("RETURN doc"), std::string::npos);
}

TEST_F(AQLQueryTemplateLibraryTest, InstantiateFilteredScan) {
    std::string aql = lib.instantiate("filtered_scan", {
        {"var",        "u"},
        {"collection", "users"},
        {"condition",  "u.age > 18"}
    });
    EXPECT_NE(aql.find("FOR u IN users"), std::string::npos);
    EXPECT_NE(aql.find("FILTER u.age > 18"), std::string::npos);
    EXPECT_NE(aql.find("RETURN u"), std::string::npos);
}

TEST_F(AQLQueryTemplateLibraryTest, InstantiateGraphTraversal) {
    std::string aql = lib.instantiate("graph_traversal", {
        {"var",        "v"},
        {"start_node", "users/42"},
        {"edge_coll",  "friends"},
        {"depth_min",  "1"},
        {"depth_max",  "3"},
        {"direction",  "OUTBOUND"}
    });
    EXPECT_NE(aql.find("1..3"), std::string::npos);
    EXPECT_NE(aql.find("OUTBOUND"), std::string::npos);
    EXPECT_NE(aql.find("users/42"), std::string::npos);
    EXPECT_NE(aql.find("friends"), std::string::npos);
}

TEST_F(AQLQueryTemplateLibraryTest, InstantiateReplacesAllOccurrences) {
    // "simple_scan" uses {{var}} twice (in FOR and RETURN)
    std::string aql = lib.instantiate("simple_scan", {
        {"var",        "x"},
        {"collection", "col"}
    });
    // Make sure both occurrences were replaced (no {{var}} left)
    EXPECT_EQ(aql.find("{{var}}"), std::string::npos);
}

TEST_F(AQLQueryTemplateLibraryTest, InstantiateMissingParameterThrows) {
    EXPECT_THROW(
        lib.instantiate("simple_scan", {{"var", "doc"}}),  // missing "collection"
        std::invalid_argument
    );
}

TEST_F(AQLQueryTemplateLibraryTest, InstantiateUnknownIdThrows) {
    EXPECT_THROW(
        lib.instantiate("no_such_id", {{"var", "doc"}}),
        std::invalid_argument
    );
}

TEST_F(AQLQueryTemplateLibraryTest, StaticInstantiateWorksDirectly) {
    AQLQueryTemplate tmpl;
    tmpl.id            = "inline";
    tmpl.template_body = "FOR {{v}} IN {{c}} RETURN {{v}}";
    tmpl.parameters    = {"v", "c"};

    std::string aql = AQLQueryTemplateLibrary::instantiate(tmpl, {{"v", "doc"}, {"c", "items"}});
    EXPECT_NE(aql.find("FOR doc IN items"), std::string::npos);
}

TEST_F(AQLQueryTemplateLibraryTest, InstantiateVectorSimilarity) {
    std::string aql = lib.instantiate("vector_similarity", {
        {"var",          "doc"},
        {"collection",   "embeddings"},
        {"vector_field", "embedding"},
        {"top_k",        "10"}
    });
    EXPECT_NE(aql.find("SIMILARITY"), std::string::npos);
    EXPECT_NE(aql.find("embeddings"), std::string::npos);
    EXPECT_NE(aql.find("10"), std::string::npos);
}

TEST_F(AQLQueryTemplateLibraryTest, InstantiateGroupCount) {
    std::string aql = lib.instantiate("group_count", {
        {"var",         "o"},
        {"collection",  "orders"},
        {"group_var",   "city"},
        {"group_field", "city"}
    });
    EXPECT_NE(aql.find("COLLECT"), std::string::npos);
    EXPECT_NE(aql.find("WITH COUNT INTO count"), std::string::npos);
}

// ============================================================================
// Tags on built-ins
// ============================================================================

TEST_F(AQLQueryTemplateLibraryTest, VectorTemplateHasVectorTag) {
    auto results = lib.findByTag("vector");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AQLQueryTemplateLibraryTest, AggregationTemplatesPresent) {
    auto results = lib.findByTag("aggregation");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AQLQueryTemplateLibraryTest, WriteTemplatesPresent) {
    auto results = lib.findByTag("write");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AQLQueryTemplateLibraryTest, TimeseriesTemplatePresent) {
    const auto* tmpl = lib.findById("timeseries_range");
    ASSERT_NE(tmpl, nullptr);
    EXPECT_NE(tmpl->template_body.find("{{ts_field}}"), std::string::npos);
}
