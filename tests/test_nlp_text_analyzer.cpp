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
        for (auto& c : lower_text) {
          c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
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

TEST_F(NlpMultiLanguageTest, DetectFrenchNotSpanishWithLa) {
    // "la" appears in both French and Spanish; other French words must dominate
    auto lang = analyzer.detectLanguage("Le chat est dans la maison et les enfants ne sont pas là.");
    EXPECT_EQ(lang, NlpTextAnalyzer::Language::FRENCH);
}

TEST_F(NlpMultiLanguageTest, DetectSpanishNotFrenchWithLa) {
    // "la" appears in both; other Spanish words must dominate
    auto lang = analyzer.detectLanguage("El perro y los niños van para la playa pero una pelota no está.");
    EXPECT_EQ(lang, NlpTextAnalyzer::Language::SPANISH);
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
        if (m.category == "obligation") {
          found_obligation = true;
        }
        if (m.category == "permission") {
          found_permission = true;
        }
    }
    EXPECT_TRUE(found_obligation);
    EXPECT_TRUE(found_permission);
}

