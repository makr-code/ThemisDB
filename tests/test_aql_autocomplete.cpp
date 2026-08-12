/**
 * @file test_aql_autocomplete.cpp
 * @brief Unit tests for AQLAutoComplete (LSP-compatible auto-complete engine)
 */

#include <gtest/gtest.h>
#include "aql/aql_autocomplete.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace themis::aql;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool hasLabel(const std::vector<CompletionItem>& items,
                     const std::string& label) {
    return std::any_of(items.begin(), items.end(),
        [&](const CompletionItem& i) { return i.label == label; });
}

static bool hasKind(const std::vector<CompletionItem>& items,
                    const std::string& label,
                    CompletionItemKind kind) {
    return std::any_of(items.begin(), items.end(),
        [&](const CompletionItem& i) {
            return i.label == label && i.kind == kind;
        });
}

// Build a context with cursor at end of text
static CompletionContext makeCtx(const std::string& text,
                                 const std::string& schema = "") {
    CompletionContext ctx;
    ctx.query_text    = text;
    ctx.cursor_offset = text.size();
    ctx.schema_context = schema;
    return ctx;
}

// Build a context with cursor at an explicit offset
static CompletionContext makeCtxAt(const std::string& text,
                                   std::size_t offset,
                                   const std::string& schema = "") {
    CompletionContext ctx;
    ctx.query_text    = text;
    ctx.cursor_offset = offset;
    ctx.schema_context = schema;
    return ctx;
}

// ============================================================================
// allKeywords / allFunctions
// ============================================================================

class AQLAutoCompleteMetaTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteMetaTest, AllKeywordsNonEmpty) {
    auto kws = ac.allKeywords();
    EXPECT_FALSE(kws.empty());
}

TEST_F(AQLAutoCompleteMetaTest, AllKeywordsContainCoreClauses) {
    auto kws = ac.allKeywords();
    for (const auto& kw : {"FOR", "FILTER", "RETURN", "SORT", "LIMIT", "LET", "COLLECT"}) {
        EXPECT_NE(std::find(kws.begin(), kws.end(), kw), kws.end())
            << "Expected keyword: " << kw;
    }
}

TEST_F(AQLAutoCompleteMetaTest, AllKeywordsContainLLMExtensions) {
    auto kws = ac.allKeywords();
    for (const auto& kw : {"LLM", "INFER", "RAG", "EMBED"}) {
        EXPECT_NE(std::find(kws.begin(), kws.end(), kw), kws.end())
            << "Expected LLM keyword: " << kw;
    }
}

TEST_F(AQLAutoCompleteMetaTest, AllFunctionsNonEmpty) {
    auto fns = ac.allFunctions();
    EXPECT_FALSE(fns.empty());
}

TEST_F(AQLAutoCompleteMetaTest, AllFunctionsContainCommonOnes) {
    auto fns = ac.allFunctions();
    for (const auto& fn : {"COUNT", "SUM", "MIN", "MAX", "AVG", "LENGTH",
                            "CONCAT", "CONTAINS", "SIMILARITY", "ST_DISTANCE"}) {
        EXPECT_NE(std::find(fns.begin(), fns.end(), fn), fns.end())
            << "Expected function: " << fn;
    }
}

// ============================================================================
// Empty / trivial inputs
// ============================================================================

class AQLAutoCompleteBasicTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteBasicTest, EmptyQueryReturnsKeywords) {
    auto items = ac.complete(makeCtx(""));
    EXPECT_FALSE(items.empty());
    EXPECT_TRUE(hasLabel(items, "FOR"));
    EXPECT_TRUE(hasLabel(items, "RETURN"));
}

TEST_F(AQLAutoCompleteBasicTest, EmptyQueryCursorAtNpos) {
    CompletionContext ctx;
    ctx.query_text    = "";
    ctx.cursor_offset = std::string::npos;
    auto items = ac.complete(ctx);
    EXPECT_FALSE(items.empty());
}

TEST_F(AQLAutoCompleteBasicTest, NoPrefix_AllCandidatesReturned) {
    // A space at end means prefix is "" → all candidates returned
    auto items = ac.complete(makeCtx("FOR u IN users "));
    EXPECT_FALSE(items.empty());
    EXPECT_TRUE(hasLabel(items, "FILTER"));
    EXPECT_TRUE(hasLabel(items, "RETURN"));
    EXPECT_TRUE(hasLabel(items, "SORT"));
}

// ============================================================================
// Prefix filtering
// ============================================================================

class AQLAutoCompletePrefixTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompletePrefixTest, PrefixF_ContainsFilterAndFor) {
    auto items = ac.complete(makeCtx("F"));
    EXPECT_TRUE(hasLabel(items, "FOR"));
    EXPECT_TRUE(hasLabel(items, "FILTER"));
}

TEST_F(AQLAutoCompletePrefixTest, PrefixFI_ContainsFilter_NotFor) {
    auto items = ac.complete(makeCtx("FI"));
    EXPECT_TRUE(hasLabel(items,  "FILTER"));
    EXPECT_FALSE(hasLabel(items, "FOR"));
}

TEST_F(AQLAutoCompletePrefixTest, PrefixRET_ContainsReturn) {
    auto items = ac.complete(makeCtx("FOR u IN users RET"));
    EXPECT_TRUE(hasLabel(items, "RETURN"));
    EXPECT_FALSE(hasLabel(items, "FOR"));
}

TEST_F(AQLAutoCompletePrefixTest, PrefixSO_ContainsSort) {
    auto items = ac.complete(makeCtx("FOR u IN users SO"));
    EXPECT_TRUE(hasLabel(items, "SORT"));
}

TEST_F(AQLAutoCompletePrefixTest, PrefixLI_ContainsLimit) {
    auto items = ac.complete(makeCtx("FOR u IN users LI"));
    EXPECT_TRUE(hasLabel(items, "LIMIT"));
}

TEST_F(AQLAutoCompletePrefixTest, PrefixCOUN_ContainsCountFunction) {
    auto items = ac.complete(makeCtx("FOR u IN users RETURN COU"));
    // COUNT is a function; prefix "COU" should match it
    bool found = false;
    for (const auto& i : items) {
        if (i.label == "COUNT") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(AQLAutoCompletePrefixTest, CaseInsensitivePrefix_Lowercase) {
    // "for" (lower) should still match FOR, FILTER etc.
    auto items = ac.complete(makeCtx("fo"));
    EXPECT_TRUE(hasLabel(items, "FOR"));
}

TEST_F(AQLAutoCompletePrefixTest, ZeroMatchPrefix_ReturnsEmpty) {
    // "ZZZZ" shouldn't match any known keyword/function
    auto items = ac.complete(makeCtx("ZZZZ"));
    EXPECT_TRUE(items.empty());
}

// ============================================================================
// Variable completions
// ============================================================================

class AQLAutoCompleteVariableTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteVariableTest, ForDeclaredVar_AppearAsVariable) {
    auto items = ac.complete(makeCtx("FOR user IN users RETURN use"));
    EXPECT_TRUE(hasLabel(items, "user"));
    EXPECT_TRUE(hasKind(items,  "user", CompletionItemKind::Variable));
}

TEST_F(AQLAutoCompleteVariableTest, LetDeclaredVar_AppearAsVariable) {
    auto items = ac.complete(makeCtx("FOR u IN users LET total = u.age RETURN tot"));
    EXPECT_TRUE(hasLabel(items, "total"));
    EXPECT_TRUE(hasKind(items,  "total", CompletionItemKind::Variable));
}

TEST_F(AQLAutoCompleteVariableTest, MultipleForVars_BothPresent) {
    const std::string q = "FOR u IN users FOR o IN orders RETURN ";
    auto items = ac.complete(makeCtx(q));
    EXPECT_TRUE(hasLabel(items, "u"));
    EXPECT_TRUE(hasLabel(items, "o"));
}

TEST_F(AQLAutoCompleteVariableTest, VarBeforeCursor_PrefixFilter) {
    // cursor right after "u" → only variables matching "u" are returned
    const std::string q = "FOR user IN users FOR order IN orders RETURN u";
    auto items = ac.complete(makeCtx(q));
    EXPECT_TRUE(hasLabel(items, "user"));
    // "order" does not start with "u" — should not appear
    EXPECT_FALSE(hasLabel(items, "order"));
}

TEST_F(AQLAutoCompleteVariableTest, CollectGroupVar_AppearAsVariable) {
    // COLLECT dep = u.department creates variable 'dep'
    const std::string q = "FOR u IN users COLLECT dep = u.department RETURN d";
    auto items = ac.complete(makeCtx(q));
    EXPECT_TRUE(hasLabel(items, "dep"));
    EXPECT_TRUE(hasKind(items, "dep", CompletionItemKind::Variable));
}

TEST_F(AQLAutoCompleteVariableTest, CollectIntoVar_AppearAsVariable) {
    // COLLECT ... INTO grp creates variable 'grp'
    const std::string q = "FOR u IN users COLLECT dep = u.department INTO grp RETURN g";
    auto items = ac.complete(makeCtx(q));
    EXPECT_TRUE(hasLabel(items, "grp"));
    EXPECT_TRUE(hasKind(items, "grp", CompletionItemKind::Variable));
}

TEST_F(AQLAutoCompleteVariableTest, CollectWithCountInto_AppearAsVariable) {
    // COLLECT WITH COUNT INTO cnt creates variable 'cnt'
    const std::string q = "FOR u IN users COLLECT WITH COUNT INTO cnt RETURN c";
    auto items = ac.complete(makeCtx(q));
    EXPECT_TRUE(hasLabel(items, "cnt"));
    EXPECT_TRUE(hasKind(items, "cnt", CompletionItemKind::Variable));
}

// ============================================================================
// Dot / attribute completion
// ============================================================================

class AQLAutoCompleteDotTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteDotTest, DotAfterKnownVar_ReturnsFields) {
    const std::string schema = "collection: users(id, name, age, email)";
    const std::string q      = "FOR u IN users RETURN u.";
    auto items = ac.complete(makeCtx(q, schema));
    EXPECT_TRUE(hasLabel(items, "id"));
    EXPECT_TRUE(hasLabel(items, "name"));
    EXPECT_TRUE(hasLabel(items, "age"));
    EXPECT_TRUE(hasLabel(items, "email"));
}

TEST_F(AQLAutoCompleteDotTest, DotAfterKnownVar_FieldKind) {
    const std::string schema = "collection: users(name, age)";
    const std::string q      = "FOR u IN users RETURN u.";
    auto items = ac.complete(makeCtx(q, schema));
    EXPECT_TRUE(hasKind(items, "name", CompletionItemKind::Field));
    EXPECT_TRUE(hasKind(items, "age",  CompletionItemKind::Field));
}

TEST_F(AQLAutoCompleteDotTest, DotAfterKnownVar_WithPartialFieldPrefix) {
    const std::string schema = "collection: users(id, name, age)";
    const std::string q      = "FOR u IN users RETURN u.na";
    auto items = ac.complete(makeCtx(q, schema));
    EXPECT_TRUE(hasLabel(items,  "name"));
    EXPECT_FALSE(hasLabel(items, "id"));
    EXPECT_FALSE(hasLabel(items, "age"));
}

TEST_F(AQLAutoCompleteDotTest, DotWithNoSchema_ReturnsEmpty) {
    const std::string q = "FOR u IN users FILTER u.";
    // No schema → no attribute suggestions (and variable "u" may appear)
    auto items = ac.complete(makeCtx(q));
    // Should not crash; result may be empty or contain variable names
    SUCCEED();
    // No field-kind items without schema
    bool has_field = std::any_of(items.begin(), items.end(), [](const CompletionItem& i){
        return i.kind == CompletionItemKind::Field;
    });
    EXPECT_FALSE(has_field);
}

TEST_F(AQLAutoCompleteDotTest, DotAfterUnknownVar_UnionOfAllFields) {
    // "x" is not declared in any FOR, but schema has fields → union returned
    const std::string schema = "collection: orders(id, total)";
    const std::string q      = "FOR o IN orders RETURN x.";
    auto items = ac.complete(makeCtx(q, schema));
    // x is not bound to 'orders' (FOR o IN orders binds 'o', not 'x')
    // Implementation returns union of all schema fields
    EXPECT_TRUE(hasLabel(items, "id"));
    EXPECT_TRUE(hasLabel(items, "total"));
}

// ============================================================================
// Schema parsing
// ============================================================================

class AQLAutoCompleteSchemaTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteSchemaTest, CollectionPrefixFormat) {
    const std::string schema = "collection: users(id, name), orders(id, amount)";
    const std::string q      = "FOR u IN users FILTER u.";
    auto items = ac.complete(makeCtx(q, schema));
    EXPECT_TRUE(hasLabel(items, "id"));
    EXPECT_TRUE(hasLabel(items, "name"));
    // 'amount' belongs to 'orders', not 'users'
    EXPECT_FALSE(hasLabel(items, "amount"));
}

TEST_F(AQLAutoCompleteSchemaTest, PlainCollectionNameSchema) {
    // Simplified format without fields — engine should not crash and should
    // still offer general keyword completions
    const std::string schema = "users, orders, products";
    const std::string q      = "FOR u IN users RETURN ";
    auto items = ac.complete(makeCtx(q, schema));
    EXPECT_FALSE(items.empty());
    // Plain schema has no field info, so keyword completions should be present
    EXPECT_TRUE(hasLabel(items, "FILTER"));
}

// ============================================================================
// Cursor offset
// ============================================================================

class AQLAutoCompleteCursorTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteCursorTest, CursorAtMiddleOfQuery) {
    // "FOR u IN users FI|LTER u.age > 18 RETURN u"
    //                  ^ cursor here (after FI)
    const std::string q = "FOR u IN users FILTER u.age > 18 RETURN u";
    // Position just after "FI" in "FILTER" → offset 17
    std::size_t cursor = q.find("FILTER") + 2; // after "FI"
    auto items = ac.complete(makeCtxAt(q, cursor));
    EXPECT_TRUE(hasLabel(items, "FILTER"));
}

TEST_F(AQLAutoCompleteCursorTest, CursorAtStart) {
    const std::string q = "FILTER u.age > 18 RETURN u";
    auto items = ac.complete(makeCtxAt(q, 0));
    // Empty prefix at position 0 → all candidates
    EXPECT_FALSE(items.empty());
}

TEST_F(AQLAutoCompleteCursorTest, CursorBeyondEnd_ClampedToEnd) {
    const std::string q = "FOR u IN ";
    auto items = ac.complete(makeCtxAt(q, 9999));
    // Should not crash; effectively cursor at end
    EXPECT_FALSE(items.empty());
}

// ============================================================================
// CompletionItem fields
// ============================================================================

class AQLAutoCompleteItemFieldsTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteItemFieldsTest, KeywordHasInsertText) {
    auto items = ac.complete(makeCtx("FO"));
    auto it = std::find_if(items.begin(), items.end(),
        [](const CompletionItem& i){ return i.label == "FOR"; });
    ASSERT_NE(it, items.end());
    EXPECT_FALSE(it->insert_text.empty());
}

TEST_F(AQLAutoCompleteItemFieldsTest, FunctionHasSignatureDetail) {
    auto items = ac.complete(makeCtx("COU"));
    auto it = std::find_if(items.begin(), items.end(),
        [](const CompletionItem& i){ return i.label == "COUNT"; });
    ASSERT_NE(it, items.end());
    EXPECT_FALSE(it->detail.empty());
    // insert_text should contain a parenthesis
    EXPECT_NE(it->insert_text.find('('), std::string::npos);
}

TEST_F(AQLAutoCompleteItemFieldsTest, PrefixStartSetCorrectly) {
    // "FOR u IN users FILT" → prefix="FILT", prefix_start = position of F
    const std::string q = "FOR u IN users FILT";
    auto items = ac.complete(makeCtx(q));
    auto it = std::find_if(items.begin(), items.end(),
        [](const CompletionItem& i){ return i.label == "FILTER"; });
    ASSERT_NE(it, items.end());
    // "FILT" starts at offset 15 in the string
    EXPECT_EQ(it->prefix_start, q.rfind("FILT"));
}

TEST_F(AQLAutoCompleteItemFieldsTest, LSPKindValuesMatchSpec) {
    // Verify that key CompletionItemKind enum values match LSP spec
    EXPECT_EQ(static_cast<int>(CompletionItemKind::Keyword),  14);
    EXPECT_EQ(static_cast<int>(CompletionItemKind::Function),  3);
    EXPECT_EQ(static_cast<int>(CompletionItemKind::Variable),  6);
    EXPECT_EQ(static_cast<int>(CompletionItemKind::Field),     5);
    EXPECT_EQ(static_cast<int>(CompletionItemKind::Snippet),  15);
}

// ============================================================================
// Sort order
// ============================================================================

class AQLAutoCompleteSortTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteSortTest, SortOrderMonotonicallyNonDecreasing) {
    auto items = ac.complete(makeCtx(""));
    for (std::size_t i = 1; i < items.size(); ++i) {
        EXPECT_LE(items[i-1].sort_order, items[i].sort_order)
            << "Sort order not monotone at index " << i;
    }
}

TEST_F(AQLAutoCompleteSortTest, DeclaredVariablesRankedFirst) {
    // When a user has typed a variable prefix, declared vars should appear
    // before keywords (they have lower sort_order)
    const std::string q = "FOR user IN users RETURN user";
    auto items = ac.complete(makeCtx(q));
    // First item should be the variable 'user'
    ASSERT_FALSE(items.empty());
    EXPECT_EQ(items[0].label, "user");
    EXPECT_EQ(items[0].kind, CompletionItemKind::Variable);
}

// ============================================================================
// LLM keywords
// ============================================================================

class AQLAutoCompleteLLMTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteLLMTest, LLMKeywordInAllKeywords) {
    auto kws = ac.allKeywords();
    EXPECT_NE(std::find(kws.begin(), kws.end(), "LLM"), kws.end());
    EXPECT_NE(std::find(kws.begin(), kws.end(), "INFER"), kws.end());
    EXPECT_NE(std::find(kws.begin(), kws.end(), "RAG"), kws.end());
    EXPECT_NE(std::find(kws.begin(), kws.end(), "EMBED"), kws.end());
}

TEST_F(AQLAutoCompleteLLMTest, PrefixLL_ReturnLLMKeyword) {
    auto items = ac.complete(makeCtx("FOR u IN users LET r = LL"));
    EXPECT_TRUE(hasLabel(items, "LLM"));
}

TEST_F(AQLAutoCompleteLLMTest, PrefixINF_ReturnInfer) {
    auto items = ac.complete(makeCtx("LLM INF"));
    EXPECT_TRUE(hasLabel(items, "INFER"));
}

// ============================================================================
// Snippet / insert_text format
// ============================================================================

class AQLAutoCompleteSnippetTest : public ::testing::Test {
protected:
    AQLAutoComplete ac;
};

TEST_F(AQLAutoCompleteSnippetTest, ForInsertTextHasPlaceholders) {
    auto items = ac.complete(makeCtx("FO"));
    auto it = std::find_if(items.begin(), items.end(),
        [](const CompletionItem& i){ return i.label == "FOR"; });
    ASSERT_NE(it, items.end());
    // FOR snippet should contain "${1:" style placeholders
    EXPECT_NE(it->insert_text.find("${1"), std::string::npos);
}

TEST_F(AQLAutoCompleteSnippetTest, FunctionInsertTextHasParens) {
    auto items = ac.complete(makeCtx("SIM"));
    auto it = std::find_if(items.begin(), items.end(),
        [](const CompletionItem& i){ return i.label == "SIMILARITY"; });
    ASSERT_NE(it, items.end());
    EXPECT_NE(it->insert_text.find('('), std::string::npos);
    EXPECT_NE(it->insert_text.find(')'), std::string::npos);
}
