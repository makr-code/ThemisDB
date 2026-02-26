/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_nlp_text_analyzer.cpp                         ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-02-26                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                       ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "analytics/nlp_text_analyzer.h"

using namespace themis::analytics;

class NlpMultiLanguageTest : public ::testing::Test {
protected:
    NlpTextAnalyzer analyzer;
};

// ========== Language Detection Tests ==========

TEST_F(NlpMultiLanguageTest, DetectGerman) {
    auto lang = analyzer.detectLanguage("Der schnelle braune Fuchs und die flinke Katze sind nicht da.");
    EXPECT_EQ(lang, NlpTextAnalyzer::Language::GERMAN);
}

TEST_F(NlpMultiLanguageTest, DetectEnglish) {
    auto lang = analyzer.detectLanguage("The quick brown fox and the cat are not here.");
    EXPECT_EQ(lang, NlpTextAnalyzer::Language::ENGLISH);
}

TEST_F(NlpMultiLanguageTest, DetectFrench) {
    auto lang = analyzer.detectLanguage("Le renard brun et la chatte ne sont pas là.");
    EXPECT_EQ(lang, NlpTextAnalyzer::Language::FRENCH);
}

TEST_F(NlpMultiLanguageTest, DetectSpanish) {
    auto lang = analyzer.detectLanguage("El zorro marrón y los gatos están para casa.");
    EXPECT_EQ(lang, NlpTextAnalyzer::Language::SPANISH);
}

TEST_F(NlpMultiLanguageTest, DetectItalian) {
    auto lang = analyzer.detectLanguage("Gli animali della foresta sono molto belli per tutti.");
    EXPECT_EQ(lang, NlpTextAnalyzer::Language::ITALIAN);
}

TEST_F(NlpMultiLanguageTest, DetectDutch) {
    auto lang = analyzer.detectLanguage("Het snelle bruine vos van het bos is niet hier.");
    EXPECT_EQ(lang, NlpTextAnalyzer::Language::DUTCH);
}

// ========== Stop Words Tests ==========

TEST_F(NlpMultiLanguageTest, SpanishStopWords) {
    EXPECT_TRUE(analyzer.isStopWord("el", NlpTextAnalyzer::Language::SPANISH));
    EXPECT_TRUE(analyzer.isStopWord("la", NlpTextAnalyzer::Language::SPANISH));
    EXPECT_TRUE(analyzer.isStopWord("para", NlpTextAnalyzer::Language::SPANISH));
    EXPECT_FALSE(analyzer.isStopWord("zorro", NlpTextAnalyzer::Language::SPANISH));
}

TEST_F(NlpMultiLanguageTest, ItalianStopWords) {
    EXPECT_TRUE(analyzer.isStopWord("il", NlpTextAnalyzer::Language::ITALIAN));
    EXPECT_TRUE(analyzer.isStopWord("gli", NlpTextAnalyzer::Language::ITALIAN));
    EXPECT_TRUE(analyzer.isStopWord("sono", NlpTextAnalyzer::Language::ITALIAN));
    EXPECT_FALSE(analyzer.isStopWord("volpe", NlpTextAnalyzer::Language::ITALIAN));
}

TEST_F(NlpMultiLanguageTest, DutchStopWords) {
    EXPECT_TRUE(analyzer.isStopWord("het", NlpTextAnalyzer::Language::DUTCH));
    EXPECT_TRUE(analyzer.isStopWord("van", NlpTextAnalyzer::Language::DUTCH));
    EXPECT_TRUE(analyzer.isStopWord("niet", NlpTextAnalyzer::Language::DUTCH));
    EXPECT_FALSE(analyzer.isStopWord("vos", NlpTextAnalyzer::Language::DUTCH));
}

// ========== Stemming Tests ==========

TEST_F(NlpMultiLanguageTest, SpanishStemming) {
    // Spanish "-ando" gerund suffix
    std::string stem = analyzer.stemWord("corriendo", NlpTextAnalyzer::Language::SPANISH);
    EXPECT_NE(stem, "corriendo");
    // Spanish "-ar" infinitive suffix
    stem = analyzer.stemWord("hablar", NlpTextAnalyzer::Language::SPANISH);
    EXPECT_NE(stem, "hablar");
}

TEST_F(NlpMultiLanguageTest, ItalianStemming) {
    // Italian "-are" infinitive suffix
    std::string stem = analyzer.stemWord("parlare", NlpTextAnalyzer::Language::ITALIAN);
    EXPECT_NE(stem, "parlare");
    // Italian "-zione" suffix
    stem = analyzer.stemWord("comunicazione", NlpTextAnalyzer::Language::ITALIAN);
    EXPECT_NE(stem, "comunicazione");
}

TEST_F(NlpMultiLanguageTest, DutchStemming) {
    // Dutch "-ing" suffix
    std::string stem = analyzer.stemWord("vergadering", NlpTextAnalyzer::Language::DUTCH);
    EXPECT_NE(stem, "vergadering");
    // Dutch "-en" suffix
    stem = analyzer.stemWord("lopen", NlpTextAnalyzer::Language::DUTCH);
    EXPECT_NE(stem, "lopen");
}

TEST_F(NlpMultiLanguageTest, FrenchStemming) {
    // French "-ment" adverb suffix
    std::string stem = analyzer.stemWord("rapidement", NlpTextAnalyzer::Language::FRENCH);
    EXPECT_NE(stem, "rapidement");
    // French "-er" infinitive suffix
    stem = analyzer.stemWord("parler", NlpTextAnalyzer::Language::FRENCH);
    EXPECT_NE(stem, "parler");
}

// ========== Legal Modality Tests ==========

TEST_F(NlpMultiLanguageTest, LegalModalitiesHardcodedFallback) {
    // When YAML config is not found, fallback patterns must work
    auto modalities = analyzer.extractLegalModalities(
        "Der Antragsteller muss die Unterlagen einreichen. Die Behörde kann eine Fristverlängerung gewähren.");
    ASSERT_FALSE(modalities.empty());
    bool found_obligation = false;
    bool found_permission = false;
    for (const auto& m : modalities) {
        if (m.category == "obligation") found_obligation = true;
        if (m.category == "permission") found_permission = true;
    }
    EXPECT_TRUE(found_obligation);
    EXPECT_TRUE(found_permission);
}

