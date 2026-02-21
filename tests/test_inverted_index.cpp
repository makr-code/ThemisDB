// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 ThemisDB Contributors

// ============================================================================
// Unit tests for the standalone InvertedIndex class + HIGHLIGHT / FULLTEXT_SNIPPET
// ============================================================================

#include <gtest/gtest.h>
#include <filesystem>

#include "index/inverted_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "query/functions/function_registry.h"
#include "query/functions/fulltext_functions.h"

using namespace themis;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class InvertedIndexTest : public ::testing::Test {
protected:
    static std::string testDbPath() {
        return "data/test_inverted_index_db";
    }

    void SetUp() override {
        std::filesystem::remove_all(testDbPath());
        RocksDBWrapper::Config cfg;
        cfg.db_path = testDbPath();
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test RocksDB";
        idx_ = std::make_unique<InvertedIndex>(*db_);
    }

    void TearDown() override {
        idx_.reset();
        db_.reset();
        std::filesystem::remove_all(testDbPath());
    }

    // Helper: create a default index on articles.content
    void createDefaultIndex(bool stemming = false,
                             const std::string& lang = "en",
                             bool stopwords = false) {
        InvertedIndex::Config cfg;
        cfg.stemming_enabled  = stemming;
        cfg.language          = lang;
        cfg.stopwords_enabled = stopwords;
        auto st = idx_->create("articles", "content", cfg);
        ASSERT_TRUE(st.ok) << st.message;
    }

    // Helper: index a document and also write a relational record so that
    // phrase-search can retrieve the original field text.
    void addDocument(const std::string& pk, const std::string& text) {
        auto st = idx_->index("articles", "content", pk, text);
        ASSERT_TRUE(st.ok) << st.message;

        // Write a BaseEntity so phrase search can read the raw field value
        BaseEntity entity(pk);
        entity.setField("content", text);
        auto blob = entity.serialize();
        std::vector<uint8_t> bytes(blob.begin(), blob.end());
        db_->put(KeySchema::makeRelationalKey("articles", pk), bytes);
    }

    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<InvertedIndex>  idx_;
};

// ===========================================================================
// Key-schema helpers
// ===========================================================================

TEST(InvertedIndexKeySchema, MetaKey) {
    EXPECT_EQ(InvertedIndex::makeMetaKey("users", "bio"),
              "ftidxmeta:users:bio");
}

TEST(InvertedIndexKeySchema, IndexKey) {
    EXPECT_EQ(InvertedIndex::makeIndexKey("users", "bio", "foo", "pk1"),
              "ftidx:users:bio:foo:pk1");
}

TEST(InvertedIndexKeySchema, IndexPrefix) {
    EXPECT_EQ(InvertedIndex::makeIndexPrefix("users", "bio", "foo"),
              "ftidx:users:bio:foo:");
}

TEST(InvertedIndexKeySchema, TFKey) {
    EXPECT_EQ(InvertedIndex::makeTFKey("users", "bio", "foo", "pk1"),
              "fttf:users:bio:foo:pk1");
}

TEST(InvertedIndexKeySchema, RevKey) {
    EXPECT_EQ(InvertedIndex::makeRevKey("users", "bio", "pk1"),
              "ftrev:users:bio:pk1");
}

TEST(InvertedIndexKeySchema, DocLenKey) {
    EXPECT_EQ(InvertedIndex::makeDocLenKey("users", "bio", "pk1"),
              "ftdlen:users:bio:pk1");
}

// ===========================================================================
// Tokenisation
// ===========================================================================

TEST(InvertedIndexTokenize, BasicTokenize) {
    auto tokens = InvertedIndex::tokenize("Hello, World! foo bar");
    ASSERT_EQ(tokens.size(), 4u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "foo");
    EXPECT_EQ(tokens[3], "bar");
}

TEST(InvertedIndexTokenize, EmptyText) {
    EXPECT_TRUE(InvertedIndex::tokenize("").empty());
    EXPECT_TRUE(InvertedIndex::tokenize("   ").empty());
}

TEST(InvertedIndexTokenize, WithStopwords) {
    InvertedIndex::Config cfg;
    cfg.stopwords_enabled = true;
    cfg.language          = "en";
    auto tokens = InvertedIndex::tokenize("the quick brown fox", cfg);
    // "the" is a common English stop word
    for (const auto& t : tokens)
        EXPECT_NE(t, "the");
    EXPECT_FALSE(tokens.empty());
}

TEST(InvertedIndexTokenize, WithStemming) {
    InvertedIndex::Config cfg;
    cfg.stemming_enabled = true;
    cfg.language         = "en";
    auto tokens = InvertedIndex::tokenize("running machines", cfg);
    // Porter stemmer should reduce "running" and "machines"
    EXPECT_FALSE(tokens.empty());
    // Stemmed form of "running" should not be "running" (at minimum truncated)
    bool found_running = false;
    for (const auto& t : tokens)
        if (t == "running") found_running = true;
    EXPECT_FALSE(found_running) << "Stemming should shorten 'running'";
}

// ===========================================================================
// Lifecycle
// ===========================================================================

TEST_F(InvertedIndexTest, CreateDropExists) {
    EXPECT_FALSE(idx_->exists("articles", "content"));

    auto st = idx_->create("articles", "content");
    EXPECT_TRUE(st.ok) << st.message;
    EXPECT_TRUE(idx_->exists("articles", "content"));

    st = idx_->drop("articles", "content");
    EXPECT_TRUE(st.ok) << st.message;
    EXPECT_FALSE(idx_->exists("articles", "content"));
}

TEST_F(InvertedIndexTest, CreatePersistsConfig) {
    InvertedIndex::Config cfg;
    cfg.stemming_enabled  = true;
    cfg.language          = "de";
    cfg.stopwords_enabled = true;
    cfg.normalize_umlauts = true;
    cfg.stopwords         = {"und", "oder"};

    ASSERT_TRUE(idx_->create("articles", "title", cfg).ok);

    auto loaded = idx_->getConfig("articles", "title");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->stemming_enabled);
    EXPECT_EQ(loaded->language, "de");
    EXPECT_TRUE(loaded->stopwords_enabled);
    EXPECT_TRUE(loaded->normalize_umlauts);
    ASSERT_EQ(loaded->stopwords.size(), 2u);
    EXPECT_EQ(loaded->stopwords[0], "und");
    EXPECT_EQ(loaded->stopwords[1], "oder");
}

TEST_F(InvertedIndexTest, CreateRejectsEmptyTableColumn) {
    EXPECT_FALSE(idx_->create("", "content").ok);
    EXPECT_FALSE(idx_->create("articles", "").ok);
}

TEST_F(InvertedIndexTest, CreateRejectsColonInName) {
    EXPECT_FALSE(idx_->create("art:icles", "content").ok);
    EXPECT_FALSE(idx_->create("articles", "con:tent").ok);
}

// ===========================================================================
// Document indexing
// ===========================================================================

TEST_F(InvertedIndexTest, IndexRequiresExistingIndex) {
    auto st = idx_->index("articles", "content", "pk1", "hello world");
    EXPECT_FALSE(st.ok);
}

TEST_F(InvertedIndexTest, IndexAndSearchBasic) {
    createDefaultIndex();

    addDocument("pk1", "machine learning algorithms");
    addDocument("pk2", "deep learning neural networks");
    addDocument("pk3", "database query optimization");

    auto [st, results] = idx_->search("articles", "content", "learning", 10);
    ASSERT_TRUE(st.ok) << st.message;
    ASSERT_EQ(results.size(), 2u);

    // Both pk1 and pk2 mention "learning"
    std::vector<std::string> pks;
    for (const auto& r : results) pks.push_back(r.pk);
    EXPECT_NE(std::find(pks.begin(), pks.end(), "pk1"), pks.end());
    EXPECT_NE(std::find(pks.begin(), pks.end(), "pk2"), pks.end());
}

TEST_F(InvertedIndexTest, SearchReturnsHighScoreForBetterMatch) {
    createDefaultIndex();

    // pk1 mentions "machine" twice → higher TF → higher BM25
    addDocument("pk1", "machine learning is machine intelligence");
    addDocument("pk2", "machine learning");

    auto [st, results] = idx_->search("articles", "content", "machine", 10);
    ASSERT_TRUE(st.ok);
    ASSERT_GE(results.size(), 2u);
    EXPECT_EQ(results[0].pk, "pk1");   // higher TF → ranked first
    EXPECT_GT(results[0].score, 0.0);
}

TEST_F(InvertedIndexTest, SearchAndLogic) {
    createDefaultIndex();

    addDocument("pk1", "neural network deep learning");
    addDocument("pk2", "neural network");
    addDocument("pk3", "deep learning");

    // Only pk1 has both "neural" and "deep"
    auto [st, results] =
        idx_->search("articles", "content", "neural deep", 10);
    ASSERT_TRUE(st.ok);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].pk, "pk1");
}

TEST_F(InvertedIndexTest, SearchEmptyQueryReturnsEmpty) {
    createDefaultIndex();
    addDocument("pk1", "hello world");

    auto [st, results] = idx_->search("articles", "content", "", 10);
    EXPECT_TRUE(st.ok);
    EXPECT_TRUE(results.empty());
}

TEST_F(InvertedIndexTest, SearchNoIndexReturnsError) {
    auto [st, results] = idx_->search("articles", "content", "foo", 10);
    EXPECT_FALSE(st.ok);
}

TEST_F(InvertedIndexTest, SearchLimitIsRespected) {
    createDefaultIndex();
    for (int i = 0; i < 20; ++i)
        addDocument("pk" + std::to_string(i), "common word");

    auto [st, results] = idx_->search("articles", "content", "common", 5);
    ASSERT_TRUE(st.ok);
    EXPECT_LE(results.size(), 5u);
}

// ===========================================================================
// Upsert semantics (re-indexing)
// ===========================================================================

TEST_F(InvertedIndexTest, ReindexUpdatesPostings) {
    createDefaultIndex();

    addDocument("pk1", "machine learning");

    // Re-index with different text
    auto st = idx_->index("articles", "content", "pk1", "database query");
    ASSERT_TRUE(st.ok);

    // Old token "learning" should no longer return pk1
    auto [st1, r1] = idx_->search("articles", "content", "learning", 10);
    ASSERT_TRUE(st1.ok);
    for (const auto& r : r1) EXPECT_NE(r.pk, "pk1");

    // New token "database" should return pk1
    auto [st2, r2] = idx_->search("articles", "content", "database", 10);
    ASSERT_TRUE(st2.ok);
    ASSERT_EQ(r2.size(), 1u);
    EXPECT_EQ(r2[0].pk, "pk1");
}

// ===========================================================================
// Deindex
// ===========================================================================

TEST_F(InvertedIndexTest, DeindexRemovesDocument) {
    createDefaultIndex();
    addDocument("pk1", "machine learning");
    addDocument("pk2", "deep learning");

    auto st = idx_->deindex("articles", "content", "pk1", "machine learning");
    ASSERT_TRUE(st.ok);

    auto [st2, results] = idx_->search("articles", "content", "learning", 10);
    ASSERT_TRUE(st2.ok);
    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].pk, "pk2");
}

TEST_F(InvertedIndexTest, DeindexRequiresExistingIndex) {
    auto st = idx_->deindex("articles", "content", "pk1", "hello");
    EXPECT_FALSE(st.ok);
}

// ===========================================================================
// Phrase search
// ===========================================================================

TEST_F(InvertedIndexTest, PhraseSearchFindsExactPhrase) {
    createDefaultIndex();

    addDocument("pk1", "machine learning algorithms for data science");
    addDocument("pk2", "data science and machine learning");
    addDocument("pk3", "algorithms and data structures");

    // Exact phrase "machine learning" appears in both pk1 and pk2
    auto [st, results] =
        idx_->searchPhrase("articles", "content", "machine learning", 10);
    ASSERT_TRUE(st.ok) << st.message;
    ASSERT_GE(results.size(), 2u);
    std::vector<std::string> pks;
    for (const auto& r : results) pks.push_back(r.pk);
    EXPECT_NE(std::find(pks.begin(), pks.end(), "pk1"), pks.end());
    EXPECT_NE(std::find(pks.begin(), pks.end(), "pk2"), pks.end());
    // pk3 does NOT contain "machine learning"
    EXPECT_EQ(std::find(pks.begin(), pks.end(), "pk3"), pks.end());
}

TEST_F(InvertedIndexTest, PhraseSearchEmptyReturnsEmpty) {
    createDefaultIndex();
    addDocument("pk1", "hello world");

    auto [st, results] =
        idx_->searchPhrase("articles", "content", "", 10);
    EXPECT_TRUE(st.ok);
    EXPECT_TRUE(results.empty());
}

TEST_F(InvertedIndexTest, PhraseSearchNoIndexReturnsError) {
    auto [st, results] =
        idx_->searchPhrase("articles", "content", "foo bar", 10);
    EXPECT_FALSE(st.ok);
}

// ===========================================================================
// Fuzzy search
// ===========================================================================

TEST_F(InvertedIndexTest, FuzzySearchFindsTypos) {
    createDefaultIndex();

    addDocument("pk1", "machine learning");
    addDocument("pk2", "database query");

    // "lerning" is distance 1 from "learning"
    auto [st, results] =
        idx_->searchFuzzy("articles", "content", "lerning", 2, 10);
    ASSERT_TRUE(st.ok) << st.message;

    std::vector<std::string> pks;
    for (const auto& r : results) pks.push_back(r.pk);
    EXPECT_NE(std::find(pks.begin(), pks.end(), "pk1"), pks.end());
}

TEST_F(InvertedIndexTest, FuzzySearchExactMatchScoreIsHigher) {
    createDefaultIndex();

    addDocument("pk1", "learning");
    addDocument("pk2", "lerning");  // typo

    auto [st, results] =
        idx_->searchFuzzy("articles", "content", "learning", 2, 10);
    ASSERT_TRUE(st.ok);
    ASSERT_GE(results.size(), 2u);
    // pk1 has exact match → highest score (distance 0)
    EXPECT_EQ(results[0].pk, "pk1");
    EXPECT_DOUBLE_EQ(results[0].score, 1.0);  // 1/(1+0)
}

TEST_F(InvertedIndexTest, FuzzySearchMaxDistanceZeroIsExact) {
    createDefaultIndex();

    addDocument("pk1", "learning");
    addDocument("pk2", "lerning");

    auto [st, results] =
        idx_->searchFuzzy("articles", "content", "learning", 0, 10);
    ASSERT_TRUE(st.ok);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].pk, "pk1");
}

TEST_F(InvertedIndexTest, FuzzySearchNoIndexReturnsError) {
    auto [st, results] =
        idx_->searchFuzzy("articles", "content", "foo", 1, 10);
    EXPECT_FALSE(st.ok);
}

// ===========================================================================
// Multiple indexes on the same RocksDB (interoperability)
// ===========================================================================

TEST_F(InvertedIndexTest, MultipleTableColumnIndexes) {
    ASSERT_TRUE(idx_->create("articles", "title").ok);
    ASSERT_TRUE(idx_->create("articles", "body").ok);

    EXPECT_TRUE(idx_->exists("articles", "title"));
    EXPECT_TRUE(idx_->exists("articles", "body"));
    EXPECT_FALSE(idx_->exists("articles", "author"));

    ASSERT_TRUE(idx_->drop("articles", "title").ok);
    EXPECT_FALSE(idx_->exists("articles", "title"));
    EXPECT_TRUE(idx_->exists("articles", "body"));
}

// ===========================================================================
// HIGHLIGHT AQL function (pure-string, no RocksDB required)
// ===========================================================================

class HighlightFunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        themis::query::functions::registerFulltextFunctions(
            themis::query::functions::FunctionRegistry::instance());
    }

    themis::query::functions::FunctionContext ctx_;
};

TEST_F(HighlightFunctionTest, BasicHighlight) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    auto result = reg.call("HIGHLIGHT",
                           {nlohmann::json("Machine Learning is great"),
                            nlohmann::json("machine learning")}, ctx_);
    ASSERT_TRUE(result.is_string());
    std::string s = result.get<std::string>();
    EXPECT_NE(s.find("<em>Machine</em>"), std::string::npos);
    EXPECT_NE(s.find("<em>Learning</em>"), std::string::npos);
    // Non-matching word is unchanged
    EXPECT_NE(s.find("is"), std::string::npos);
}

TEST_F(HighlightFunctionTest, NoMatchReturnsOriginal) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    auto result = reg.call("HIGHLIGHT",
                           {nlohmann::json("hello world"),
                            nlohmann::json("python")}, ctx_);
    EXPECT_EQ(result.get<std::string>(), "hello world");
}

TEST_F(HighlightFunctionTest, CustomTags) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    nlohmann::json opts = {{"openTag", "<b>"}, {"closeTag", "</b>"}};
    auto result = reg.call("HIGHLIGHT",
                           {nlohmann::json("deep learning network"),
                            nlohmann::json("learning"),
                            opts}, ctx_);
    std::string s = result.get<std::string>();
    EXPECT_NE(s.find("<b>learning</b>"), std::string::npos);
    EXPECT_EQ(s.find("<em>"), std::string::npos);  // default tag not used
}

TEST_F(HighlightFunctionTest, ArrayQueryTerms) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    nlohmann::json terms = nlohmann::json::array({"neural", "network"});
    auto result = reg.call("HIGHLIGHT",
                           {nlohmann::json("A neural network model"),
                            terms}, ctx_);
    std::string s = result.get<std::string>();
    EXPECT_NE(s.find("<em>neural</em>"), std::string::npos);
    EXPECT_NE(s.find("<em>network</em>"), std::string::npos);
}

TEST_F(HighlightFunctionTest, EmptyQueryReturnsOriginal) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    auto result = reg.call("HIGHLIGHT",
                           {nlohmann::json("hello world"),
                            nlohmann::json("")}, ctx_);
    EXPECT_EQ(result.get<std::string>(), "hello world");
}

// ===========================================================================
// FULLTEXT_SNIPPET AQL function (pure-string, no RocksDB required)
// ===========================================================================

class FulltextSnippetFunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        themis::query::functions::registerFulltextFunctions(
            themis::query::functions::FunctionRegistry::instance());
    }

    themis::query::functions::FunctionContext ctx_;
};

TEST_F(FulltextSnippetFunctionTest, ShortTextReturnsFull) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    // Text shorter than default windowSize — entire text should come back highlighted
    auto result = reg.call("FULLTEXT_SNIPPET",
                           {nlohmann::json("machine learning is great"),
                            nlohmann::json("machine")}, ctx_);
    ASSERT_TRUE(result.is_string());
    std::string s = result.get<std::string>();
    EXPECT_NE(s.find("<em>machine</em>"), std::string::npos);
    // No separator when text fits
    EXPECT_EQ(s.find("..."), std::string::npos);
}

TEST_F(FulltextSnippetFunctionTest, LongTextTruncated) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    // Build a long document with the keyword buried in the middle
    std::string longText =
        std::string(300, 'a') + " " +   // filler before
        "target keyword here" +
        " " + std::string(300, 'z');     // filler after
    nlohmann::json opts = {{"windowSize", 50}};
    auto result = reg.call("FULLTEXT_SNIPPET",
                           {nlohmann::json(longText),
                            nlohmann::json("target"),
                            opts}, ctx_);
    ASSERT_TRUE(result.is_string());
    std::string s = result.get<std::string>();
    // Snippet must be reasonably short (windowSize + tags + separators)
    EXPECT_LT(s.size(), static_cast<size_t>(200));
    // Separator must appear (text was truncated on at least one side)
    EXPECT_NE(s.find("..."), std::string::npos);
    // The keyword should be highlighted
    EXPECT_NE(s.find("<em>target</em>"), std::string::npos);
}

TEST_F(FulltextSnippetFunctionTest, CustomSeparatorAndTags) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    std::string longText =
        std::string(200, 'x') + " keyword " + std::string(200, 'y');
    nlohmann::json opts = {{"windowSize", 40},
                           {"openTag",    "["},
                           {"closeTag",   "]"},
                           {"separator",  "…"}};
    auto result = reg.call("FULLTEXT_SNIPPET",
                           {nlohmann::json(longText),
                            nlohmann::json("keyword"),
                            opts}, ctx_);
    std::string s = result.get<std::string>();
    EXPECT_NE(s.find("[keyword]"), std::string::npos);
    EXPECT_NE(s.find("…"), std::string::npos);  // custom separator
    EXPECT_EQ(s.find("..."), std::string::npos); // default separator not used
}

TEST_F(FulltextSnippetFunctionTest, NoMatchReturnsHead) {
    auto& reg = themis::query::functions::FunctionRegistry::instance();
    std::string longText(300, 'q');
    nlohmann::json opts = {{"windowSize", 50}};
    auto result = reg.call("FULLTEXT_SNIPPET",
                           {nlohmann::json(longText),
                            nlohmann::json("xyz"),
                            opts}, ctx_);
    ASSERT_TRUE(result.is_string());
    // No match — starts at offset 0 (head of text) with trailing separator
    std::string s = result.get<std::string>();
    EXPECT_NE(s.find("..."), std::string::npos);
}
