/**
 * @file test_content_language_detector.cpp
 * @brief Unit tests for LanguageDetector — multi-language text detection and
 *        routing for the content ingestion pipeline.
 *
 * Covers:
 *  - LanguageDetector::detect()  — code, name, confidence, indicator hits
 *  - LanguageDetector::detectCode() — convenience wrapper
 *  - LanguageDetector::routingHint() — static routing hint mapping
 *  - Script-based detection (Cyrillic, Arabic, Japanese, Chinese)
 *  - Integration: TextProcessor::extract() populates language metadata fields
 */

#include <gtest/gtest.h>
#include "content/language_detector.h"
#include "content/content_processor.h"
#include "content/content_type.h"

using namespace themis::content;

// ============================================================================
// Helper
// ============================================================================

namespace {

ContentType makePlainTextType() {
    ContentType ct;
    ct.mime_type = "text/plain";
    ct.category  = ContentCategory::TEXT;
    return ct;
}

} // namespace

// ============================================================================
// LanguageDetector::detect — basic language identification
// ============================================================================

TEST(LanguageDetector, EmptyTextReturnsUndetermined) {
    LanguageDetector det;
    auto result = det.detect("");
    EXPECT_EQ(result.code, "und");
    EXPECT_EQ(result.indicator_hits, 0u);
    EXPECT_FLOAT_EQ(result.confidence, 0.0f);
}

TEST(LanguageDetector, EnglishTextDetected) {
    LanguageDetector det;
    const std::string text =
        "The quick brown fox jumps over the lazy dog. "
        "This is a sample text that should be identified as English. "
        "It contains many common English words and phrases with the article the.";
    auto result = det.detect(text);
    EXPECT_EQ(result.code, "en");
    EXPECT_EQ(result.name, "English");
    EXPECT_GT(result.confidence, 0.2f);
    EXPECT_GT(result.indicator_hits, 1u);
}

TEST(LanguageDetector, GermanTextDetected) {
    LanguageDetector det;
    const std::string text =
        "Der schnelle braune Fuchs springt über den faulen Hund. "
        "Das ist ein Beispieltext, der auf Deutsch erkannt werden soll. "
        "Er enthält viele gebräuchliche deutsche Wörter und Ausdrücke wie die und das.";
    auto result = det.detect(text);
    EXPECT_EQ(result.code, "de");
    EXPECT_EQ(result.name, "German");
    EXPECT_GT(result.confidence, 0.2f);
}

TEST(LanguageDetector, FrenchTextDetected) {
    LanguageDetector det;
    const std::string text =
        "Le renard brun rapide saute par-dessus le chien paresseux. "
        "C'est un exemple de texte en français avec les mots courants. "
        "Il contient aussi des expressions françaises comme le mot pour dire que.";
    auto result = det.detect(text);
    EXPECT_EQ(result.code, "fr");
    EXPECT_EQ(result.name, "French");
    EXPECT_GT(result.confidence, 0.2f);
}

TEST(LanguageDetector, SpanishTextDetected) {
    LanguageDetector det;
    const std::string text =
        "El zorro marrón rápido salta sobre el perro perezoso. "
        "Este es un texto de ejemplo en español con los artículos el y las. "
        "Contiene muchas palabras y frases que son comunes del español.";
    auto result = det.detect(text);
    EXPECT_EQ(result.code, "es");
    EXPECT_EQ(result.name, "Spanish");
    EXPECT_GT(result.confidence, 0.2f);
}

TEST(LanguageDetector, ItalianTextDetected) {
    LanguageDetector det;
    const std::string text =
        "Gli studenti sono nel laboratorio e fanno una ricerca. "
        "Questo è un testo di esempio in italiano che non hanno mai fatto. "
        "La natura della materia viene studiata e anche molte cose del libro.";
    auto result = det.detect(text);
    EXPECT_EQ(result.code, "it");
    EXPECT_EQ(result.name, "Italian");
    EXPECT_GT(result.confidence, 0.2f);
}

TEST(LanguageDetector, DutchTextDetected) {
    LanguageDetector det;
    const std::string text =
        "De snelle bruine vos springt over de luie hond. "
        "Dit is een voorbeeldtekst die als Nederlands herkend moet worden. "
        "Het bevat veel gebruikelijke Nederlandse woorden en uitdrukkingen van het dagelijks leven.";
    auto result = det.detect(text);
    EXPECT_EQ(result.code, "nl");
    EXPECT_EQ(result.name, "Dutch");
    EXPECT_GT(result.confidence, 0.2f);
}

// ============================================================================
// Script-based detection (non-Latin scripts)
// ============================================================================

TEST(LanguageDetector, RussianCyrillicDetected) {
    LanguageDetector det;
    // UTF-8 encoded Russian text
    const std::string text =
        "\xD0\x91\xD1\x8B\xD1\x81\xD1\x82\xD1\x80\xD0\xB0\xD1\x8F "
        "\xD0\xBA\xD0\xBE\xD1\x80\xD0\xB8\xD1\x87\xD0\xBD\xD0\xB5\xD0\xB2\xD0\xB0\xD1\x8F "
        "\xD0\xBB\xD0\xB8\xD1\x81\xD0\xB8\xD1\x86\xD0\xB0 "
        "\xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBF\xD1\x80\xD1\x8B\xD0\xB3\xD0\xB8\xD0\xB2\xD0\xB0\xD0\xB5\xD1\x82 "
        "\xD1\x87\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB7";
    auto result = det.detect(text);
    EXPECT_EQ(result.code, "ru");
    EXPECT_EQ(result.name, "Russian");
    EXPECT_GT(result.confidence, 0.3f);
}

TEST(LanguageDetector, JapaneseHiraganaDetected) {
    LanguageDetector det;
    // UTF-8 encoded Japanese hiragana text (はやいきつね — "fast fox")
    const std::string text =
        "\xE3\x81\xAF\xE3\x82\x84\xE3\x81\x84 "
        "\xE3\x81\x8D\xE3\x81\xA4\xE3\x81\xAD\xE3\x81\x8C "
        "\xE3\x81\xAB\xE3\x82\x93\xE3\x81\x92\xE3\x82\x93\xE3\x81\x94";
    auto result = det.detect(text);
    EXPECT_EQ(result.code, "ja");
    EXPECT_EQ(result.name, "Japanese");
    EXPECT_GT(result.confidence, 0.3f);
}

// ============================================================================
// LanguageDetector::detectCode — convenience wrapper
// ============================================================================

TEST(LanguageDetector, DetectCodeReturnsCorrectCode) {
    LanguageDetector det;
    const std::string en_text =
        "The database stores documents efficiently with vector indexing.";
    EXPECT_EQ(det.detectCode(en_text), "en");
}

TEST(LanguageDetector, DetectCodeUndeterminedForEmpty) {
    LanguageDetector det;
    EXPECT_EQ(det.detectCode(""), "und");
}

// ============================================================================
// LanguageDetector::routingHint — static mapping
// ============================================================================

TEST(LanguageDetector, RoutingHintEnglish) {
    EXPECT_EQ(LanguageDetector::routingHint("en"), "latin-en");
}

TEST(LanguageDetector, RoutingHintGerman) {
    EXPECT_EQ(LanguageDetector::routingHint("de"), "latin-de");
}

TEST(LanguageDetector, RoutingHintFrench) {
    EXPECT_EQ(LanguageDetector::routingHint("fr"), "latin-fr");
}

TEST(LanguageDetector, RoutingHintSpanish) {
    EXPECT_EQ(LanguageDetector::routingHint("es"), "latin-es");
}

TEST(LanguageDetector, RoutingHintItalian) {
    EXPECT_EQ(LanguageDetector::routingHint("it"), "latin-it");
}

TEST(LanguageDetector, RoutingHintDutch) {
    EXPECT_EQ(LanguageDetector::routingHint("nl"), "latin-other");
}

TEST(LanguageDetector, RoutingHintPortuguese) {
    EXPECT_EQ(LanguageDetector::routingHint("pt"), "latin-other");
}

TEST(LanguageDetector, RoutingHintRussian) {
    EXPECT_EQ(LanguageDetector::routingHint("ru"), "cyrillic");
}

TEST(LanguageDetector, RoutingHintArabic) {
    EXPECT_EQ(LanguageDetector::routingHint("ar"), "arabic");
}

TEST(LanguageDetector, RoutingHintJapanese) {
    EXPECT_EQ(LanguageDetector::routingHint("ja"), "japanese");
}

TEST(LanguageDetector, RoutingHintChinese) {
    EXPECT_EQ(LanguageDetector::routingHint("zh"), "chinese");
}

TEST(LanguageDetector, RoutingHintUndetermined) {
    EXPECT_EQ(LanguageDetector::routingHint("und"), "unknown");
}

TEST(LanguageDetector, RoutingHintUnknownCode) {
    EXPECT_EQ(LanguageDetector::routingHint("xx"), "unknown");
}

// ============================================================================
// Integration: TextProcessor::extract() populates language metadata fields
// ============================================================================

TEST(TextProcessorLanguageIntegration, ExtractPopulatesLanguageMetadata) {
    TextProcessor proc;
    const std::string en_text =
        "The quick brown fox jumps over the lazy dog. "
        "This document is written in English and contains common English words.";
    ContentType ct = makePlainTextType();
    auto result = proc.extract(en_text, ct);

    ASSERT_TRUE(result.ok);
    ASSERT_TRUE(result.metadata.contains("detected_language"))
        << "metadata must contain 'detected_language'";
    ASSERT_TRUE(result.metadata.contains("language_name"))
        << "metadata must contain 'language_name'";
    ASSERT_TRUE(result.metadata.contains("language_confidence"))
        << "metadata must contain 'language_confidence'";
    ASSERT_TRUE(result.metadata.contains("language_routing_hint"))
        << "metadata must contain 'language_routing_hint'";

    EXPECT_EQ(result.metadata["detected_language"].get<std::string>(), "en");
    EXPECT_EQ(result.metadata["language_name"].get<std::string>(), "English");
    EXPECT_GT(result.metadata["language_confidence"].get<float>(), 0.0f);
    EXPECT_EQ(result.metadata["language_routing_hint"].get<std::string>(), "latin-en");
}

TEST(TextProcessorLanguageIntegration, ExtractDetectsGermanText) {
    TextProcessor proc;
    const std::string de_text =
        "Der schnelle braune Fuchs springt über den faulen Hund. "
        "Dieses Dokument ist auf Deutsch geschrieben und enthält viele typische "
        "deutsche Wörter wie der, die, das und nicht.";
    ContentType ct = makePlainTextType();
    auto result = proc.extract(de_text, ct);

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.metadata["detected_language"].get<std::string>(), "de");
    EXPECT_EQ(result.metadata["language_routing_hint"].get<std::string>(), "latin-de");
}

TEST(TextProcessorLanguageIntegration, ExtractEmptyTextHasUndetermined) {
    TextProcessor proc;
    ContentType ct = makePlainTextType();
    auto result = proc.extract("", ct);

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.metadata["detected_language"].get<std::string>(), "und");
    EXPECT_EQ(result.metadata["language_routing_hint"].get<std::string>(), "unknown");
}
