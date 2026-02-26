/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_nlp_text_analyzer.cpp                         ║
  Version:         0.0.33                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "analytics/nlp_text_analyzer.h"

using namespace themis::analytics;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------
class NlpLemmatizeTest : public ::testing::Test {
protected:
    NlpTextAnalyzer analyzer;
};

// ---------------------------------------------------------------------------
// English lemmatization
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, English_IrregularVerbs) {
    EXPECT_EQ(analyzer.lemmatizeWord("was",      NlpTextAnalyzer::Language::ENGLISH), "be");
    EXPECT_EQ(analyzer.lemmatizeWord("were",     NlpTextAnalyzer::Language::ENGLISH), "be");
    EXPECT_EQ(analyzer.lemmatizeWord("has",      NlpTextAnalyzer::Language::ENGLISH), "have");
    EXPECT_EQ(analyzer.lemmatizeWord("had",      NlpTextAnalyzer::Language::ENGLISH), "have");
    EXPECT_EQ(analyzer.lemmatizeWord("went",     NlpTextAnalyzer::Language::ENGLISH), "go");
    EXPECT_EQ(analyzer.lemmatizeWord("ran",      NlpTextAnalyzer::Language::ENGLISH), "run");
    EXPECT_EQ(analyzer.lemmatizeWord("wrote",    NlpTextAnalyzer::Language::ENGLISH), "write");
    EXPECT_EQ(analyzer.lemmatizeWord("thought",  NlpTextAnalyzer::Language::ENGLISH), "think");
    EXPECT_EQ(analyzer.lemmatizeWord("built",    NlpTextAnalyzer::Language::ENGLISH), "build");
}

TEST_F(NlpLemmatizeTest, English_IrregularNouns) {
    EXPECT_EQ(analyzer.lemmatizeWord("children", NlpTextAnalyzer::Language::ENGLISH), "child");
    EXPECT_EQ(analyzer.lemmatizeWord("men",      NlpTextAnalyzer::Language::ENGLISH), "man");
    EXPECT_EQ(analyzer.lemmatizeWord("women",    NlpTextAnalyzer::Language::ENGLISH), "woman");
    EXPECT_EQ(analyzer.lemmatizeWord("feet",     NlpTextAnalyzer::Language::ENGLISH), "foot");
    EXPECT_EQ(analyzer.lemmatizeWord("teeth",    NlpTextAnalyzer::Language::ENGLISH), "tooth");
    EXPECT_EQ(analyzer.lemmatizeWord("data",     NlpTextAnalyzer::Language::ENGLISH), "datum");
    EXPECT_EQ(analyzer.lemmatizeWord("criteria", NlpTextAnalyzer::Language::ENGLISH), "criterion");
    EXPECT_EQ(analyzer.lemmatizeWord("matrices", NlpTextAnalyzer::Language::ENGLISH), "matrix");
}

TEST_F(NlpLemmatizeTest, English_RegularSuffixStripping) {
    EXPECT_EQ(analyzer.lemmatizeWord("running",  NlpTextAnalyzer::Language::ENGLISH), "run");
    EXPECT_EQ(analyzer.lemmatizeWord("stopped",  NlpTextAnalyzer::Language::ENGLISH), "stop");
    EXPECT_EQ(analyzer.lemmatizeWord("studies",  NlpTextAnalyzer::Language::ENGLISH), "study");
    EXPECT_EQ(analyzer.lemmatizeWord("studied",  NlpTextAnalyzer::Language::ENGLISH), "study");
    EXPECT_EQ(analyzer.lemmatizeWord("walks",    NlpTextAnalyzer::Language::ENGLISH), "walk");
    EXPECT_EQ(analyzer.lemmatizeWord("quickly",  NlpTextAnalyzer::Language::ENGLISH), "quick");
}

TEST_F(NlpLemmatizeTest, English_BaseFormUnchanged) {
    EXPECT_EQ(analyzer.lemmatizeWord("run",   NlpTextAnalyzer::Language::ENGLISH), "run");
    EXPECT_EQ(analyzer.lemmatizeWord("walk",  NlpTextAnalyzer::Language::ENGLISH), "walk");
    EXPECT_EQ(analyzer.lemmatizeWord("be",    NlpTextAnalyzer::Language::ENGLISH), "be");
}

// ---------------------------------------------------------------------------
// German lemmatization
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, German_IrregularAuxiliaries) {
    EXPECT_EQ(analyzer.lemmatizeWord("ist",     NlpTextAnalyzer::Language::GERMAN), "sein");
    EXPECT_EQ(analyzer.lemmatizeWord("war",     NlpTextAnalyzer::Language::GERMAN), "sein");
    EXPECT_EQ(analyzer.lemmatizeWord("gewesen", NlpTextAnalyzer::Language::GERMAN), "sein");
    EXPECT_EQ(analyzer.lemmatizeWord("hat",     NlpTextAnalyzer::Language::GERMAN), "haben");
    EXPECT_EQ(analyzer.lemmatizeWord("hatte",   NlpTextAnalyzer::Language::GERMAN), "haben");
    EXPECT_EQ(analyzer.lemmatizeWord("wird",    NlpTextAnalyzer::Language::GERMAN), "werden");
    EXPECT_EQ(analyzer.lemmatizeWord("wurde",   NlpTextAnalyzer::Language::GERMAN), "werden");
}

TEST_F(NlpLemmatizeTest, German_ModalVerbs) {
    EXPECT_EQ(analyzer.lemmatizeWord("kann",    NlpTextAnalyzer::Language::GERMAN), "können");
    EXPECT_EQ(analyzer.lemmatizeWord("muss",    NlpTextAnalyzer::Language::GERMAN), "müssen");
    EXPECT_EQ(analyzer.lemmatizeWord("soll",    NlpTextAnalyzer::Language::GERMAN), "sollen");
    EXPECT_EQ(analyzer.lemmatizeWord("will",    NlpTextAnalyzer::Language::GERMAN), "wollen");
    EXPECT_EQ(analyzer.lemmatizeWord("darf",    NlpTextAnalyzer::Language::GERMAN), "dürfen");
}

TEST_F(NlpLemmatizeTest, German_RegularSuffixStripping) {
    EXPECT_EQ(analyzer.lemmatizeWord("spielend", NlpTextAnalyzer::Language::GERMAN), "spielen");
    EXPECT_EQ(analyzer.lemmatizeWord("spielten", NlpTextAnalyzer::Language::GERMAN), "spielen");
    EXPECT_EQ(analyzer.lemmatizeWord("Häuser",   NlpTextAnalyzer::Language::GERMAN), "haus");
}

// ---------------------------------------------------------------------------
// French lemmatization
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, French_IrregularForms) {
    EXPECT_EQ(analyzer.lemmatizeWord("est",     NlpTextAnalyzer::Language::FRENCH), "être");
    EXPECT_EQ(analyzer.lemmatizeWord("sont",    NlpTextAnalyzer::Language::FRENCH), "être");
    EXPECT_EQ(analyzer.lemmatizeWord("étais",   NlpTextAnalyzer::Language::FRENCH), "être");
    EXPECT_EQ(analyzer.lemmatizeWord("ont",     NlpTextAnalyzer::Language::FRENCH), "avoir");
    EXPECT_EQ(analyzer.lemmatizeWord("eu",      NlpTextAnalyzer::Language::FRENCH), "avoir");
    EXPECT_EQ(analyzer.lemmatizeWord("vont",    NlpTextAnalyzer::Language::FRENCH), "aller");
    EXPECT_EQ(analyzer.lemmatizeWord("fait",    NlpTextAnalyzer::Language::FRENCH), "faire");
    EXPECT_EQ(analyzer.lemmatizeWord("peuvent", NlpTextAnalyzer::Language::FRENCH), "pouvoir");
}

TEST_F(NlpLemmatizeTest, French_RegularVerbSuffixes) {
    EXPECT_EQ(analyzer.lemmatizeWord("parlons",  NlpTextAnalyzer::Language::FRENCH), "parler");
    EXPECT_EQ(analyzer.lemmatizeWord("parlez",   NlpTextAnalyzer::Language::FRENCH), "parler");
    EXPECT_EQ(analyzer.lemmatizeWord("parlait",  NlpTextAnalyzer::Language::FRENCH), "parler");
    EXPECT_EQ(analyzer.lemmatizeWord("parlé",    NlpTextAnalyzer::Language::FRENCH), "parler");
    EXPECT_EQ(analyzer.lemmatizeWord("parler",   NlpTextAnalyzer::Language::FRENCH), "parler");
}

TEST_F(NlpLemmatizeTest, French_AdjectiveAgreement) {
    EXPECT_EQ(analyzer.lemmatizeWord("heureuse",  NlpTextAnalyzer::Language::FRENCH), "heureux");
    EXPECT_EQ(analyzer.lemmatizeWord("heureuses", NlpTextAnalyzer::Language::FRENCH), "heureux");
    EXPECT_EQ(analyzer.lemmatizeWord("actives",   NlpTextAnalyzer::Language::FRENCH), "actif");
    EXPECT_EQ(analyzer.lemmatizeWord("active",    NlpTextAnalyzer::Language::FRENCH), "actif");
    EXPECT_EQ(analyzer.lemmatizeWord("animaux",   NlpTextAnalyzer::Language::FRENCH), "animal");
}

// ---------------------------------------------------------------------------
// Spanish lemmatization
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, Spanish_IrregularForms) {
    EXPECT_EQ(analyzer.lemmatizeWord("soy",    NlpTextAnalyzer::Language::SPANISH), "ser");
    EXPECT_EQ(analyzer.lemmatizeWord("es",     NlpTextAnalyzer::Language::SPANISH), "ser");
    EXPECT_EQ(analyzer.lemmatizeWord("fue",    NlpTextAnalyzer::Language::SPANISH), "ser");
    EXPECT_EQ(analyzer.lemmatizeWord("estoy",  NlpTextAnalyzer::Language::SPANISH), "estar");
    EXPECT_EQ(analyzer.lemmatizeWord("tiene",  NlpTextAnalyzer::Language::SPANISH), "tener");
    EXPECT_EQ(analyzer.lemmatizeWord("hizo",   NlpTextAnalyzer::Language::SPANISH), "hacer");
    EXPECT_EQ(analyzer.lemmatizeWord("puedo",  NlpTextAnalyzer::Language::SPANISH), "poder");
}

TEST_F(NlpLemmatizeTest, Spanish_RegularVerbSuffixes) {
    EXPECT_EQ(analyzer.lemmatizeWord("hablando",  NlpTextAnalyzer::Language::SPANISH), "hablar");
    EXPECT_EQ(analyzer.lemmatizeWord("hablamos",  NlpTextAnalyzer::Language::SPANISH), "hablar");
    EXPECT_EQ(analyzer.lemmatizeWord("hablaron",  NlpTextAnalyzer::Language::SPANISH), "hablar");
    EXPECT_EQ(analyzer.lemmatizeWord("hablado",   NlpTextAnalyzer::Language::SPANISH), "hablar");
    EXPECT_EQ(analyzer.lemmatizeWord("hablar",    NlpTextAnalyzer::Language::SPANISH), "hablar");
    EXPECT_EQ(analyzer.lemmatizeWord("corriendo", NlpTextAnalyzer::Language::SPANISH), "correr");
}

// ---------------------------------------------------------------------------
// Italian lemmatization
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, Italian_IrregularForms) {
    EXPECT_EQ(analyzer.lemmatizeWord("sono",     NlpTextAnalyzer::Language::ITALIAN), "essere");
    EXPECT_EQ(analyzer.lemmatizeWord("stato",    NlpTextAnalyzer::Language::ITALIAN), "essere");
    EXPECT_EQ(analyzer.lemmatizeWord("ho",       NlpTextAnalyzer::Language::ITALIAN), "avere");
    EXPECT_EQ(analyzer.lemmatizeWord("avuto",    NlpTextAnalyzer::Language::ITALIAN), "avere");
    EXPECT_EQ(analyzer.lemmatizeWord("vado",     NlpTextAnalyzer::Language::ITALIAN), "andare");
    EXPECT_EQ(analyzer.lemmatizeWord("andato",   NlpTextAnalyzer::Language::ITALIAN), "andare");
    EXPECT_EQ(analyzer.lemmatizeWord("fatto",    NlpTextAnalyzer::Language::ITALIAN), "fare");
    EXPECT_EQ(analyzer.lemmatizeWord("posso",    NlpTextAnalyzer::Language::ITALIAN), "potere");
    EXPECT_EQ(analyzer.lemmatizeWord("voglio",   NlpTextAnalyzer::Language::ITALIAN), "volere");
}

TEST_F(NlpLemmatizeTest, Italian_RegularVerbSuffixes) {
    EXPECT_EQ(analyzer.lemmatizeWord("parlando",  NlpTextAnalyzer::Language::ITALIAN), "parlare");
    EXPECT_EQ(analyzer.lemmatizeWord("parlato",   NlpTextAnalyzer::Language::ITALIAN), "parlare");
    EXPECT_EQ(analyzer.lemmatizeWord("parlata",   NlpTextAnalyzer::Language::ITALIAN), "parlare");
    EXPECT_EQ(analyzer.lemmatizeWord("parlare",   NlpTextAnalyzer::Language::ITALIAN), "parlare");
    EXPECT_EQ(analyzer.lemmatizeWord("venduto",   NlpTextAnalyzer::Language::ITALIAN), "vendere");
    EXPECT_EQ(analyzer.lemmatizeWord("finito",    NlpTextAnalyzer::Language::ITALIAN), "finire");
}

// ---------------------------------------------------------------------------
// Dutch lemmatization
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, Dutch_IrregularForms) {
    EXPECT_EQ(analyzer.lemmatizeWord("is",      NlpTextAnalyzer::Language::DUTCH), "zijn");
    EXPECT_EQ(analyzer.lemmatizeWord("was",     NlpTextAnalyzer::Language::DUTCH), "zijn");
    EXPECT_EQ(analyzer.lemmatizeWord("geweest", NlpTextAnalyzer::Language::DUTCH), "zijn");
    EXPECT_EQ(analyzer.lemmatizeWord("heeft",   NlpTextAnalyzer::Language::DUTCH), "hebben");
    EXPECT_EQ(analyzer.lemmatizeWord("had",     NlpTextAnalyzer::Language::DUTCH), "hebben");
    EXPECT_EQ(analyzer.lemmatizeWord("gaat",    NlpTextAnalyzer::Language::DUTCH), "gaan");
    EXPECT_EQ(analyzer.lemmatizeWord("gegaan",  NlpTextAnalyzer::Language::DUTCH), "gaan");
    EXPECT_EQ(analyzer.lemmatizeWord("kan",     NlpTextAnalyzer::Language::DUTCH), "kunnen");
    EXPECT_EQ(analyzer.lemmatizeWord("wil",     NlpTextAnalyzer::Language::DUTCH), "willen");
    EXPECT_EQ(analyzer.lemmatizeWord("moet",    NlpTextAnalyzer::Language::DUTCH), "moeten");
    EXPECT_EQ(analyzer.lemmatizeWord("zal",     NlpTextAnalyzer::Language::DUTCH), "zullen");
}

TEST_F(NlpLemmatizeTest, Dutch_RegularSuffixStripping) {
    EXPECT_EQ(analyzer.lemmatizeWord("spelende",  NlpTextAnalyzer::Language::DUTCH), "spelen");
    EXPECT_EQ(analyzer.lemmatizeWord("spelden",   NlpTextAnalyzer::Language::DUTCH), "spelen");
}

// ---------------------------------------------------------------------------
// Case-insensitivity
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, CaseInsensitive) {
    EXPECT_EQ(analyzer.lemmatizeWord("WAS",   NlpTextAnalyzer::Language::ENGLISH), "be");
    EXPECT_EQ(analyzer.lemmatizeWord("Was",   NlpTextAnalyzer::Language::ENGLISH), "be");
    EXPECT_EQ(analyzer.lemmatizeWord("IST",   NlpTextAnalyzer::Language::GERMAN),  "sein");
    EXPECT_EQ(analyzer.lemmatizeWord("SONT",  NlpTextAnalyzer::Language::FRENCH),  "être");
}

// ---------------------------------------------------------------------------
// stemWord delegates to lemmatizeWord
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, StemWordDelegatesToLemmatize) {
    EXPECT_EQ(analyzer.stemWord("was",   NlpTextAnalyzer::Language::ENGLISH), "be");
    EXPECT_EQ(analyzer.stemWord("running", NlpTextAnalyzer::Language::ENGLISH), "run");
    EXPECT_EQ(analyzer.stemWord("ist",   NlpTextAnalyzer::Language::GERMAN),   "sein");
}

// ---------------------------------------------------------------------------
// tokenize sets lemma via lemmatizeWord
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, TokenizeSetsLemma) {
    // English sentence: tokenizer detects English
    auto tokens = analyzer.tokenize("The children were running");
    bool found_children = false;
    bool found_running  = false;
    for (const auto& tok : tokens) {
        std::string lower_text = tok.text;
        for (auto& c : lower_text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower_text == "children") {
            EXPECT_EQ(tok.lemma, "child");
            found_children = true;
        }
        if (lower_text == "running") {
            EXPECT_EQ(tok.lemma, "run");
            found_running = true;
        }
    }
    EXPECT_TRUE(found_children) << "Token 'children' not found";
    EXPECT_TRUE(found_running)  << "Token 'running' not found";
}

// ---------------------------------------------------------------------------
// Unknown language falls through to lowercase
// ---------------------------------------------------------------------------
TEST_F(NlpLemmatizeTest, UnknownLanguageFallback) {
    EXPECT_EQ(analyzer.lemmatizeWord("Running", NlpTextAnalyzer::Language::UNKNOWN), "running");
}

