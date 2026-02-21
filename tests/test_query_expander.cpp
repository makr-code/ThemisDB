/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_query_expander.cpp                            ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 13:49:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 1f4de1436  2026-02-21  Search module: v1.4.0 hardening + v1.5.0 feature set (7 n... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_query_expander.cpp
 * @brief Unit tests for QueryExpander (v1.5.0)
 */

#include <gtest/gtest.h>
#include "search/query_expander.h"
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(QueryExpanderConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(QueryExpander{});
}

TEST(QueryExpanderConfig, NegativeEditDistanceThrows) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = -1;
    EXPECT_THROW(QueryExpander{cfg}, std::invalid_argument);
}

TEST(QueryExpanderConfig, ZeroMaxExpansionsThrows) {
    QueryExpander::Config cfg;
    cfg.max_expansions = 0;
    EXPECT_THROW(QueryExpander{cfg}, std::invalid_argument);
}

TEST(QueryExpanderConfig, ConfigRoundtrip) {
    QueryExpander::Config cfg;
    cfg.max_expansions = 3;
    cfg.max_edit_distance = 1;
    QueryExpander qe{cfg};
    EXPECT_EQ(qe.getConfig().max_expansions, 3u);
    EXPECT_EQ(qe.getConfig().max_edit_distance, 1);
}

// ============================================================================
// expand() — empty / trivial cases
// ============================================================================

TEST(QueryExpanderExpand, EmptyQueryReturnsEmptyResult) {
    QueryExpander qe;
    auto r = qe.expand("");
    EXPECT_EQ(r.original, "");
    EXPECT_TRUE(r.all_terms.empty());
    EXPECT_TRUE(r.synonyms.empty());
}

TEST(QueryExpanderExpand, OriginalPreserved) {
    QueryExpander qe;
    auto r = qe.expand("machine learning");
    EXPECT_EQ(r.original, "machine learning");
}

TEST(QueryExpanderExpand, TokensWithPunctuationStripped) {
    QueryExpander qe;
    auto r = qe.expand("hello, world!");
    EXPECT_EQ(r.all_terms.size(), 2u);
    EXPECT_EQ(r.all_terms[0], "hello");
    EXPECT_EQ(r.all_terms[1], "world");
}

// ============================================================================
// Synonym expansion
// ============================================================================

TEST(QueryExpanderSynonyms, SynonymAddedToAllTerms) {
    QueryExpander qe;
    qe.addSynonyms("ml", {"machine learning"});
    auto r = qe.expand("ml algorithms");
    EXPECT_EQ(r.synonyms.size(), 1u);
    EXPECT_EQ(r.synonyms[0], "machine learning");
    // all_terms should include both original tokens and synonym
    auto it = std::find(r.all_terms.begin(), r.all_terms.end(), "machine learning");
    EXPECT_NE(it, r.all_terms.end());
}

TEST(QueryExpanderSynonyms, MaxExpansionsHonored) {
    QueryExpander::Config cfg;
    cfg.max_expansions = 2;
    QueryExpander qe{cfg};
    qe.addSynonyms("db", {"database", "data store", "persistent storage", "rdbms"});
    auto r = qe.expand("db query");
    EXPECT_LE(r.synonyms.size(), 2u);
}

TEST(QueryExpanderSynonyms, NoSynonymForUnknownTerm) {
    QueryExpander qe;
    auto r = qe.expand("quantum computing");
    EXPECT_TRUE(r.synonyms.empty());
}

TEST(QueryExpanderSynonyms, SynonymsDisabledByConfig) {
    QueryExpander::Config cfg;
    cfg.use_synonyms = false;
    QueryExpander qe{cfg};
    qe.addSynonyms("ml", {"machine learning"});
    auto r = qe.expand("ml");
    EXPECT_TRUE(r.synonyms.empty());
}

TEST(QueryExpanderSynonyms, NoDuplicateSynonyms) {
    QueryExpander qe;
    qe.addSynonyms("ai", {"artificial intelligence"});
    qe.addSynonyms("ai", {"artificial intelligence"}); // duplicate
    auto r = qe.expand("ai");
    // "artificial intelligence" should appear at most once
    size_t count = std::count(r.synonyms.begin(), r.synonyms.end(), "artificial intelligence");
    EXPECT_EQ(count, 1u);
}

// ============================================================================
// Spelling correction
// ============================================================================

TEST(QueryExpanderSpelling, ExactWordInVocabularyNotCorrected) {
    QueryExpander qe;
    qe.addVocabulary({"machine", "learning"});
    EXPECT_EQ(qe.correctSpelling("machine"), "machine");
}

TEST(QueryExpanderSpelling, SingleEditCorrected) {
    QueryExpander qe;
    qe.addVocabulary({"machine", "learning"});
    // "machne" is 1 edit from "machine"
    std::string corrected = qe.correctSpelling("machne");
    EXPECT_EQ(corrected, "machine");
}

TEST(QueryExpanderSpelling, CorrectionAppearsInExpandResult) {
    QueryExpander qe;
    qe.addVocabulary({"database", "query", "index"});
    auto r = qe.expand("databse"); // 1 edit from "database"
    EXPECT_FALSE(r.corrected.empty());
    EXPECT_EQ(r.corrected, "database");
}

TEST(QueryExpanderSpelling, NoCorrectionWhenVocabularyEmpty) {
    QueryExpander qe;
    EXPECT_EQ(qe.correctSpelling("anything"), "anything");
}

TEST(QueryExpanderSpelling, SpellingCorrectionDisabledByConfig) {
    QueryExpander::Config cfg;
    cfg.correct_spelling = false;
    QueryExpander qe{cfg};
    qe.addVocabulary({"database"});
    auto r = qe.expand("databse");
    EXPECT_TRUE(r.corrected.empty());
}

// ============================================================================
// suggestAlternatives
// ============================================================================

TEST(QueryExpanderAlternatives, AlternativeQueriesGenerated) {
    QueryExpander qe;
    qe.addSynonyms("ml", {"machine learning"});
    auto alts = qe.suggestAlternatives("ml algorithms");
    EXPECT_FALSE(alts.empty());
    EXPECT_EQ(alts[0], "machine learning algorithms");
}

TEST(QueryExpanderAlternatives, EmptyWhenNoSynonyms) {
    QueryExpander qe;
    auto alts = qe.suggestAlternatives("no synonyms here");
    EXPECT_TRUE(alts.empty());
}

// ============================================================================
// relaxQuery
// ============================================================================

TEST(QueryExpanderRelax, SingleTokenReturnsEmpty) {
    QueryExpander qe;
    EXPECT_EQ(qe.relaxQuery("database"), "");
}

TEST(QueryExpanderRelax, MultiTokenDropsLastToken) {
    QueryExpander qe;
    std::string relaxed = qe.relaxQuery("machine learning algorithms");
    EXPECT_EQ(relaxed, "machine learning");
}

TEST(QueryExpanderRelax, EmptyQueryReturnsEmpty) {
    QueryExpander qe;
    EXPECT_EQ(qe.relaxQuery(""), "");
}
