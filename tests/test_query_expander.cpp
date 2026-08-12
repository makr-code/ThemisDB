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

// ============================================================================
// suggestSpellingCorrections
// ============================================================================

TEST(QueryExpanderSpellSuggest, EmptyVocabularyReturnsEmpty) {
    QueryExpander qe;
    auto suggestions = qe.suggestSpellingCorrections("databse");
    EXPECT_TRUE(suggestions.empty());
}

TEST(QueryExpanderSpellSuggest, ExactWordInVocabularyReturnsEmpty) {
    QueryExpander qe;
    qe.addVocabulary({"database"});
    auto suggestions = qe.suggestSpellingCorrections("database");
    EXPECT_TRUE(suggestions.empty());
}

TEST(QueryExpanderSpellSuggest, SingleEditDistanceFound) {
    QueryExpander qe;
    qe.addVocabulary({"database", "query", "index"});
    // "databse" is 1 edit from "database"
    auto suggestions = qe.suggestSpellingCorrections("databse");
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0].suggestion, "database");
    EXPECT_EQ(suggestions[0].edit_distance, 1);
    EXPECT_GT(suggestions[0].confidence, 0.0);
    EXPECT_LE(suggestions[0].confidence, 1.0);
}

TEST(QueryExpanderSpellSuggest, ResultsRankedByEditDistance) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    qe.addVocabulary({"machine", "machines"});
    // "machne" is 1 edit from "machine" and 2 edits from "machines"
    auto suggestions = qe.suggestSpellingCorrections("machne", 5);
    ASSERT_GE(suggestions.size(), 1u);
    // First result should have the lowest edit distance
    EXPECT_LE(suggestions[0].edit_distance, suggestions.back().edit_distance);
}

TEST(QueryExpanderSpellSuggest, MaxSuggestionsHonored) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    qe.addVocabulary({"machine", "machina", "machines", "mashine", "mochine"});
    auto suggestions = qe.suggestSpellingCorrections("machne", 2);
    EXPECT_LE(suggestions.size(), 2u);
}

TEST(QueryExpanderSpellSuggest, ConfidenceHigherForLowerEditDistance) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    qe.addVocabulary({"machine", "machines"});
    auto suggestions = qe.suggestSpellingCorrections("machne", 5);
    // If both are returned, the one with distance 1 has higher confidence
    if (suggestions.size() >= 2) {
        EXPECT_GE(suggestions[0].confidence, suggestions[1].confidence);
    }
}

TEST(QueryExpanderSpellSuggest, SpellingDisabledReturnsEmpty) {
    QueryExpander::Config cfg;
    cfg.correct_spelling = false;
    QueryExpander qe{cfg};
    qe.addVocabulary({"database"});
    auto suggestions = qe.suggestSpellingCorrections("databse");
    EXPECT_TRUE(suggestions.empty());
}

// ============================================================================
// suggestQueryCorrections
// ============================================================================

TEST(QueryExpanderQueryCorrections, EmptyVocabularyReturnsEmpty) {
    QueryExpander qe;
    auto suggestions = qe.suggestQueryCorrections("databse qurey");
    EXPECT_TRUE(suggestions.empty());
}

TEST(QueryExpanderQueryCorrections, AllCorrectTokensReturnsEmpty) {
    QueryExpander qe;
    qe.addVocabulary({"database", "query"});
    auto suggestions = qe.suggestQueryCorrections("database query");
    EXPECT_TRUE(suggestions.empty());
}

TEST(QueryExpanderQueryCorrections, SingleMisspelledToken) {
    QueryExpander qe;
    qe.addVocabulary({"database", "query", "index"});
    // "databse" is 1 edit from "database"
    auto suggestions = qe.suggestQueryCorrections("databse query");
    ASSERT_FALSE(suggestions.empty());
    // Corrected query should contain "database"
    bool found = false;
    for (const auto& s : suggestions) {
        if (s.suggestion == "database query") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(QueryExpanderQueryCorrections, MultipleMisspelledTokens) {
    QueryExpander qe;
    qe.addVocabulary({"database", "query"});
    // both tokens misspelled
    auto suggestions = qe.suggestQueryCorrections("databse qurey");
    ASSERT_FALSE(suggestions.empty());
    // All-corrected variant should appear
    bool found_all_corrected = false;
    for (const auto& s : suggestions) {
        if (s.suggestion == "database query") { found_all_corrected = true; break; }
    }
    EXPECT_TRUE(found_all_corrected);
}

TEST(QueryExpanderQueryCorrections, MaxSuggestionsHonored) {
    QueryExpander qe;
    qe.addVocabulary({"database", "query", "index"});
    auto suggestions = qe.suggestQueryCorrections("databse qurey", 1);
    EXPECT_LE(suggestions.size(), 1u);
}

TEST(QueryExpanderQueryCorrections, ResultsHaveValidConfidence) {
    QueryExpander qe;
    qe.addVocabulary({"database", "query"});
    auto suggestions = qe.suggestQueryCorrections("databse query");
    for (const auto& s : suggestions) {
        EXPECT_GE(s.confidence, 0.0);
        EXPECT_LE(s.confidence, 1.0);
    }
}

TEST(QueryExpanderQueryCorrections, SpellingDisabledReturnsEmpty) {
    QueryExpander::Config cfg;
    cfg.correct_spelling = false;
    QueryExpander qe{cfg};
    qe.addVocabulary({"database"});
    auto suggestions = qe.suggestQueryCorrections("databse");
    EXPECT_TRUE(suggestions.empty());
}

// ============================================================================
// addVocabularyWithFrequencies + frequency-weighted suggestions
// ============================================================================

TEST(QueryExpanderFrequency, FrequentWordRankedFirstOnTie) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    // "machina" and "machine" are both 1 edit from "machne";
    // give "machine" a much higher frequency so it should rank first.
    qe.addVocabularyWithFrequencies({{"machine", 1000}, {"machina", 10}});
    auto suggestions = qe.suggestSpellingCorrections("machne", 5);
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0].suggestion, "machine");
}

TEST(QueryExpanderFrequency, LessFrequentWordRankedSecond) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    qe.addVocabularyWithFrequencies({{"machine", 1000}, {"machina", 10}});
    auto suggestions = qe.suggestSpellingCorrections("machne", 5);
    ASSERT_GE(suggestions.size(), 2u);
    EXPECT_EQ(suggestions[1].suggestion, "machina");
}

TEST(QueryExpanderFrequency, CorrectSpellingPrefersHigherFrequency) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    // Two equidistant candidates: "cat" (high freq) and "bat" (low freq)
    qe.addVocabularyWithFrequencies({{"cat", 500}, {"bat", 5}});
    // "cot" is 1 edit from both "cat" and "bat"
    std::string corrected = qe.correctSpelling("cot");
    EXPECT_EQ(corrected, "cat");
}

TEST(QueryExpanderFrequency, ConfidenceInRangeWithFrequency) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    qe.addVocabularyWithFrequencies({{"database", 100}, {"datacase", 10}});
    auto suggestions = qe.suggestSpellingCorrections("databse", 5);
    for (const auto& s : suggestions) {
        EXPECT_GE(s.confidence, 0.0);
        EXPECT_LE(s.confidence, 1.0);
    }
}

TEST(QueryExpanderFrequency, ZeroFrequencyEntryIgnored) {
    QueryExpander qe;
    // Zero-frequency entry should be silently skipped (not added to vocabulary)
    qe.addVocabularyWithFrequencies({{"database", 0}, {"query", 100}});
    // "database" was not added (freq=0); no close match exists, so the word is returned unchanged
    EXPECT_EQ(qe.correctSpelling("database"), "database");
    // "query" was added (freq=100) and is already correct
    EXPECT_EQ(qe.correctSpelling("query"), "query");
}

TEST(QueryExpanderFrequency, FrequenciesAccumulate) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    qe.addVocabularyWithFrequencies({{"machine", 100}});
    qe.addVocabularyWithFrequencies({{"machine", 200}});
    // After two calls, "machine" should have freq=300, outranking "machina" with freq=50
    qe.addVocabularyWithFrequencies({{"machina", 50}});
    auto suggestions = qe.suggestSpellingCorrections("machne", 5);
    ASSERT_FALSE(suggestions.empty());
    EXPECT_EQ(suggestions[0].suggestion, "machine");
}

TEST(QueryExpanderFrequency, NoFrequencyDataFallsBackToAlphabetical) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    // "bat", "hat", "mat" are all 1 edit from "cat" — without frequency data,
    // they should be returned in alphabetical order.
    qe.addVocabulary({"bat", "hat", "mat"});
    auto suggestions = qe.suggestSpellingCorrections("cat", 5);
    ASSERT_GE(suggestions.size(), 2u);
    // All at same edit distance → alphabetical order
    for (size_t i = 1; i < suggestions.size(); ++i) {
        EXPECT_LT(suggestions[i-1].suggestion, suggestions[i].suggestion);
    }
}

TEST(QueryExpanderFrequency, MixedFreqAndNoFreqVocabulary) {
    QueryExpander::Config cfg;
    cfg.max_edit_distance = 2;
    QueryExpander qe{cfg};
    // "cat" and "bat" are both 1 edit from "hat"
    // "cat" has frequency, "bat" does not → "cat" should rank first.
    qe.addVocabularyWithFrequencies({{"cat", 500}});
    qe.addVocabulary({"bat"});
    auto suggestions = qe.suggestSpellingCorrections("hat", 5);
    ASSERT_GE(suggestions.size(), 2u);
    // "cat" (has frequency) should rank before "bat" (no frequency)
    EXPECT_EQ(suggestions[0].suggestion, "cat");
}
