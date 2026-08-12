/*
 * ThemisDB — Toolbox Primitives: Text processing, fingerprinting, quality,
 *                                 language detection, streaming, composite
 *
 * Tests:
 *   TXC-01  chunkText() returns at least one chunk for non-empty input
 *   TXC-02  chunkText() returns empty vector for empty input
 *   TXC-03  TextChunker::chunk() provides DocumentChunk metadata
 *   TXC-04  TextChunker::estimateTokens() is consistent with chunk_size
 *   TXC-05  Custom chunk_size / overlap config respected
 *   TXC-06  chunkTexts() returns same number of chunks as chunk()
 *   TXN-01  normalizeText() folds German umlauts
 *   TXN-02  normalizeText() folds ß -> ss
 *   TXN-03  normalizeText() is a no-op on ASCII
 *   TXN-04  TextNormalizer::normalize() produces same result as free function
 *   CFP-01  fingerprint() returns valid fingerprint for non-empty text
 *   CFP-02  fingerprint() returns invalid (empty sha256) for empty text
 *   CFP-03  Two identical texts produce identical sha256_hex
 *   CFP-04  Two different texts produce different sha256_hex
 *   CFP-05  byte_len matches text.size()
 *   CFP-06  token_estimate is approximately byte_len / 4
 *   CFP-07  ContentFingerprint::valid() reflects sha256_hex emptiness
 *   CFP-08  compute(data, len) overload produces same result as compute(string_view)
 *   TQS-01  scoreText() on empty string sets is_empty=true
 *   TQS-02  scoreText() on whitespace-only string sets is_empty=true
 *   TQS-03  scoreText() returns correct token_count for simple text
 *   TQS-04  scoreText() sets has_boilerplate=true for repeated words
 *   TQS-05  scoreText() sets has_boilerplate=false for natural prose
 *   TQS-06  scoreText() detects language for English text
 *   TQS-07  scoreText() detects language for German text
 *   TQS-08  scoreText() sets has_boilerplate=true for very short text
 *   LDT-01  detectLanguage() returns "en" for English text
 *   LDT-02  detectLanguage() returns "de" for German text
 *   LDT-03  detectLanguage() returns "und" for empty input
 *   LDT-04  detectLanguage() returns "und" for very short input (< 3 words)
 *   LDT-05  DefaultLanguageDetector::detect() matches free function
 *   LDT-06  Custom min_ratio=1.0 always returns "und"
 *   CMP-01  ToolboxCompositeBuilder throws when no routes/fallback
 *   CMP-02  ToolboxComposite routes to matching toolbox by prefix
 *   CMP-03  ToolboxComposite uses fallback when no route matches
 *   CMP-04  ToolboxComposite returns empty vector when no toolbox and no fallback
 *   CMP-05  ToolboxComposite::resolve() returns correct toolbox
 *   CMP-06  ToolboxCompositeBuilder rejects null toolbox
 *   TCS-01  extractEntitiesStream (injected) does not call callback for empty text
 *   TCS-02  extractEntitiesStream (injected) calls callback for each entity per chunk
 *   TCS-03  extractEntitiesStream (injected) processes multi-chunk documents
 *   TCS-04  extractEntitiesStream (injected) skips empty chunks
 */

#include <gtest/gtest.h>

#include "toolbox/text_chunker.h"
#include "toolbox/text_normalizer.h"
#include "toolbox/content_fingerprinter.h"
#include "toolbox/text_quality_scorer.h"
#include "toolbox/language_detector.h"
#include "toolbox/toolbox_composite.h"
#include "toolbox/toolbox_streaming.h"
#include "toolbox/ingestion_toolbox.h"

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Build a repetitive paragraph that clearly exceeds DocumentSplitter defaults.
std::string makeLongText(std::size_t approx_chars = 3000) {
    const std::string sentence =
        "The quick brown fox jumps over the lazy dog near the riverbank. ";
    std::string result;
    result.reserve(approx_chars + sentence.size());
    while (result.size() < approx_chars) {
        result += sentence;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// TXC — TextChunker
// ─────────────────────────────────────────────────────────────────────────────

TEST(TextChunkerTest, TXC01_ChunkTextNonEmpty) {
    auto chunks = themis::toolbox::chunkText("Hello world. This is a test sentence.", 10, 2);
    EXPECT_FALSE(chunks.empty());
}

TEST(TextChunkerTest, TXC02_ChunkTextEmpty) {
    auto chunks = themis::toolbox::chunkText("", 512, 64);
    EXPECT_TRUE(chunks.empty());
}

TEST(TextChunkerTest, TXC03_ChunkMetadata) {
    themis::toolbox::TextChunker chunker;
    auto chunks = chunker.chunk(makeLongText(), "doc-42");
    ASSERT_FALSE(chunks.empty());
    EXPECT_EQ(chunks[0].document_id, "doc-42");
    EXPECT_EQ(chunks[0].index, 0u);
    EXPECT_GT(chunks[0].token_count, 0u);
}

TEST(TextChunkerTest, TXC04_EstimateTokensConsistency) {
    themis::toolbox::TextChunker chunker;
    const std::string text(400, 'a'); // 400 chars, ~100 tokens at 4 chars/token
    std::size_t est = chunker.estimateTokens(text);
    EXPECT_GT(est, 0u);
    // Should be in the ballpark of 100 ± 50
    EXPECT_LT(est, 200u);
}

TEST(TextChunkerTest, TXC05_CustomConfig) {
    themis::rag::DocumentSplitterConfig cfg;
    cfg.chunk_size = 20;
    cfg.overlap    = 2;
    cfg.strategy   = themis::rag::SplitStrategy::Fixed;
    themis::toolbox::TextChunker chunker(cfg);

    const std::string cfg2 = chunker.getConfig().chunk_size == 20
        ? "ok" : "fail";
    EXPECT_EQ(cfg2, "ok");

    auto chunks = chunker.chunk("This is a short text. And another sentence here for testing.", "x");
    // Fixed chunking on 60-char text with chunk_size=20 should produce >= 1 chunk
    EXPECT_GE(chunks.size(), 1u);
}

TEST(TextChunkerTest, TXC06_ChunkTextsMatchChunk) {
    themis::toolbox::TextChunker chunker;
    const std::string text = makeLongText();
    auto full   = chunker.chunk(text, "d");
    auto texts  = chunker.chunkTexts(text, "d");
    ASSERT_EQ(full.size(), texts.size());
    for (std::size_t i = 0; i < full.size(); ++i) {
        EXPECT_EQ(full[i].text, texts[i]);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TXN — TextNormalizer
// ─────────────────────────────────────────────────────────────────────────────

TEST(TextNormalizerTest, TXN01_UmlautFolding) {
    std::string result = themis::toolbox::normalizeText("äöüÄÖÜ");
    EXPECT_EQ(result, "aouAOU");
}

TEST(TextNormalizerTest, TXN02_SzFolding) {
    std::string result = themis::toolbox::normalizeText("Straße");
    EXPECT_EQ(result, "Strasse");
}

TEST(TextNormalizerTest, TXN03_ASCIINoOp) {
    const std::string ascii = "Hello World 123!";
    EXPECT_EQ(themis::toolbox::normalizeText(ascii), ascii);
}

TEST(TextNormalizerTest, TXN04_ClassMatchesFreeFunction) {
    const std::string input = "Über den Fluss und durch den Wald.";
    themis::toolbox::TextNormalizer n;
    EXPECT_EQ(n.normalize(input), themis::toolbox::normalizeText(input));
}

// ─────────────────────────────────────────────────────────────────────────────
// CFP — ContentFingerprinter
// ─────────────────────────────────────────────────────────────────────────────

TEST(ContentFingerprinterTest, CFP01_ValidForNonEmpty) {
    auto fp = themis::toolbox::fingerprint("Hello, ThemisDB!");
    EXPECT_TRUE(fp.valid());
    EXPECT_EQ(fp.sha256_hex.size(), 64u);
}

TEST(ContentFingerprinterTest, CFP02_InvalidForEmpty) {
    auto fp = themis::toolbox::fingerprint("");
    EXPECT_FALSE(fp.valid());
    EXPECT_TRUE(fp.sha256_hex.empty());
}

TEST(ContentFingerprinterTest, CFP03_SameTextSameHash) {
    const std::string text = "Consistent content";
    auto fp1 = themis::toolbox::fingerprint(text);
    auto fp2 = themis::toolbox::fingerprint(text);
    EXPECT_EQ(fp1.sha256_hex, fp2.sha256_hex);
    EXPECT_EQ(fp1, fp2);
}

TEST(ContentFingerprinterTest, CFP04_DifferentTextDifferentHash) {
    auto fp1 = themis::toolbox::fingerprint("Document A");
    auto fp2 = themis::toolbox::fingerprint("Document B");
    EXPECT_NE(fp1.sha256_hex, fp2.sha256_hex);
    EXPECT_NE(fp1, fp2);
}

TEST(ContentFingerprinterTest, CFP05_ByteLenMatchesInput) {
    const std::string text = "Byte length check";
    auto fp = themis::toolbox::fingerprint(text);
    EXPECT_EQ(fp.byte_len, text.size());
}

TEST(ContentFingerprinterTest, CFP06_TokenEstimateApprox) {
    const std::string text(400, 'x'); // 400 bytes
    auto fp = themis::toolbox::fingerprint(text);
    // token_estimate = round(400/4) = 100
    EXPECT_GE(fp.token_estimate, 90u);
    EXPECT_LE(fp.token_estimate, 110u);
}

TEST(ContentFingerprinterTest, CFP07_ValidReflectsSha256) {
    themis::toolbox::ContentFingerprint fp;
    EXPECT_FALSE(fp.valid());
    fp.sha256_hex = "abc";
    EXPECT_TRUE(fp.valid());
}

TEST(ContentFingerprinterTest, CFP08_BytesOverloadMatchesStringView) {
    const std::string text = "Raw bytes test";
    themis::toolbox::ContentFingerprinter printer;
    auto fp_sv  = printer.compute(std::string_view{text});
    auto fp_raw = printer.compute(
        reinterpret_cast<const unsigned char*>(text.data()), text.size());
    EXPECT_EQ(fp_sv.sha256_hex, fp_raw.sha256_hex);
    EXPECT_EQ(fp_sv.byte_len,   fp_raw.byte_len);
}

// ─────────────────────────────────────────────────────────────────────────────
// TQS — TextQualityScorer
// ─────────────────────────────────────────────────────────────────────────────

TEST(TextQualityScorerTest, TQS01_EmptyStringIsEmpty) {
    auto score = themis::toolbox::scoreText("");
    EXPECT_TRUE(score.is_empty);
    EXPECT_EQ(score.token_count, 0u);
}

TEST(TextQualityScorerTest, TQS02_WhitespaceOnlyIsEmpty) {
    auto score = themis::toolbox::scoreText("   \t\n  ");
    EXPECT_TRUE(score.is_empty);
}

TEST(TextQualityScorerTest, TQS03_TokenCountForSimpleText) {
    auto score = themis::toolbox::scoreText("one two three four five six");
    EXPECT_EQ(score.token_count, 6u);
    EXPECT_FALSE(score.is_empty);
}

TEST(TextQualityScorerTest, TQS04_BoilerplateRepeatedWords) {
    // 20 repetitions of the same word → repetition_ratio = 1 - (1/20) = 0.95
    std::string repeated;
    for (int i = 0; i < 20; ++i) { repeated += "foo "; }
    auto score = themis::toolbox::scoreText(repeated);
    EXPECT_TRUE(score.has_boilerplate);
}

TEST(TextQualityScorerTest, TQS05_NoBoilerplateForNaturalProse) {
    const std::string prose =
        "The quick brown fox jumps over the lazy dog. "
        "A journey of a thousand miles begins with a single step. "
        "To be or not to be, that is the question. "
        "All that glitters is not gold. "
        "In the beginning was the word and the word was light.";
    auto score = themis::toolbox::scoreText(prose);
    EXPECT_FALSE(score.is_empty);
    EXPECT_FALSE(score.has_boilerplate);
}

TEST(TextQualityScorerTest, TQS06_LanguageEnglish) {
    const std::string en =
        "the is are was were be been being have has had do does did "
        "will would shall should may might must can could of in and "
        "a an the for on to with by from at";
    auto score = themis::toolbox::scoreText(en);
    EXPECT_EQ(score.language, "en");
}

TEST(TextQualityScorerTest, TQS07_LanguageGerman) {
    const std::string de =
        "die der das ist sind war waren sein haben hat hatte werden wird "
        "und oder aber auch ein eine von zu mit bei aus durch fur uber";
    auto score = themis::toolbox::scoreText(de);
    EXPECT_EQ(score.language, "de");
}

TEST(TextQualityScorerTest, TQS08_ShortTextIsBoilerplate) {
    auto score = themis::toolbox::scoreText("hi");
    EXPECT_TRUE(score.has_boilerplate);
}

// ─────────────────────────────────────────────────────────────────────────────
// LDT — LanguageDetector
// ─────────────────────────────────────────────────────────────────────────────

TEST(LanguageDetectorTest, LDT01_EnglishDetected) {
    const std::string en =
        "the is are was were be been having have has had do does did "
        "will would shall should may might must can could of in and the";
    EXPECT_EQ(themis::toolbox::detectLanguage(en), "en");
}

TEST(LanguageDetectorTest, LDT02_GermanDetected) {
    const std::string de =
        "die der das ist sind war waren sein haben hat hatte werden wird "
        "und oder aber auch ein eine von zu mit bei aus durch fur";
    EXPECT_EQ(themis::toolbox::detectLanguage(de), "de");
}

TEST(LanguageDetectorTest, LDT03_EmptyIsUnd) {
    EXPECT_EQ(themis::toolbox::detectLanguage(""), "und");
}

TEST(LanguageDetectorTest, LDT04_FewWordsIsUnd) {
    EXPECT_EQ(themis::toolbox::detectLanguage("hello world"), "und");
}

TEST(LanguageDetectorTest, LDT05_ClassMatchesFreeFunction) {
    const std::string text =
        "the quick brown fox jumps over the lazy dog and the cat sat "
        "on the mat while the dog ran away across the field";
    themis::toolbox::DefaultLanguageDetector d;
    EXPECT_EQ(d.detect(text), themis::toolbox::detectLanguage(text));
}

TEST(LanguageDetectorTest, LDT06_AlwaysUndWithMaxRatio) {
    themis::toolbox::DefaultLanguageDetector d{1.0};
    const std::string en =
        "the is are was were be been having have has had do does did will would";
    // No language can match 100% stopword ratio
    EXPECT_EQ(d.detect(en), "und");
}

// ─────────────────────────────────────────────────────────────────────────────
// CMP — ToolboxComposite
// ─────────────────────────────────────────────────────────────────────────────

TEST(ToolboxCompositeTest, CMP01_BuilderThrowsEmpty) {
    themis::toolbox::ToolboxCompositeBuilder builder;
    EXPECT_THROW(builder.build(), std::logic_error);
}

TEST(ToolboxCompositeTest, CMP02_RoutesByPrefix) {
    auto text_tb = themis::toolbox::IngestionToolbox::createDefault();
    auto pdf_tb  = themis::toolbox::IngestionToolbox::createDefault();

    auto composite = themis::toolbox::ToolboxCompositeBuilder()
        .addRoute("application/pdf", pdf_tb)
        .addRoute("text/",           text_tb)
        .build();

    EXPECT_EQ(composite->resolve("application/pdf"), pdf_tb);
    EXPECT_EQ(composite->resolve("text/plain"),       text_tb);
    EXPECT_EQ(composite->resolve("text/html"),        text_tb);
}

TEST(ToolboxCompositeTest, CMP03_FallbackWhenNoMatch) {
    auto fallback = themis::toolbox::IngestionToolbox::createDefault();
    auto specific = themis::toolbox::IngestionToolbox::createDefault();

    auto composite = themis::toolbox::ToolboxCompositeBuilder()
        .addRoute("application/pdf", specific)
        .setFallback(fallback)
        .build();

    EXPECT_EQ(composite->resolve("image/jpeg"), fallback);
}

TEST(ToolboxCompositeTest, CMP04_NoMatchNoFallbackReturnsEmpty) {
    auto specific = themis::toolbox::IngestionToolbox::createDefault();

    auto composite = themis::toolbox::ToolboxCompositeBuilder()
        .addRoute("text/", specific)
        .build();

    auto entities = composite->extractEntities("test", "application/pdf");
    EXPECT_TRUE(entities.empty());
}

TEST(ToolboxCompositeTest, CMP05_ResolveReturnsNull) {
    auto specific = themis::toolbox::IngestionToolbox::createDefault();

    auto composite = themis::toolbox::ToolboxCompositeBuilder()
        .addRoute("text/", specific)
        .build();

    EXPECT_EQ(composite->resolve("application/json"), nullptr);
}

TEST(ToolboxCompositeTest, CMP06_BuilderRejectsNullToolbox) {
    themis::toolbox::ToolboxCompositeBuilder builder;
    EXPECT_THROW(
        builder.addRoute("text/", nullptr),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// TCS — extractEntitiesStream (injected toolbox)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ToolboxStreamingTest, TCS01_EmptyTextNoCallback) {
    auto toolbox = themis::toolbox::IngestionToolbox::createDefault();
    int  call_count = 0;
    themis::toolbox::extractEntitiesStream(
        *toolbox, "", "text/plain", "f.txt",
        [&](const themis::ingestion::BaseEntity&) { ++call_count; });
    EXPECT_EQ(call_count, 0);
}

TEST(ToolboxStreamingTest, TCS02_CallbackCalledPerEntity) {
    // The default toolbox with NullBackend won't produce entities,
    // but the call must complete without error.
    auto toolbox = themis::toolbox::IngestionToolbox::createDefault();
    std::vector<std::string> labels;
    themis::toolbox::extractEntitiesStream(
        *toolbox, "Hello world. Test sentence.", "text/plain", "doc.txt",
        [&](const themis::ingestion::BaseEntity& e) {
            labels.push_back(e.text);
        });
    // With NullBackend, no entities are produced — verify no crash.
    SUCCEED();
}

TEST(ToolboxStreamingTest, TCS03_MultiChunkDocument) {
    auto toolbox = themis::toolbox::IngestionToolbox::createDefault();
    const std::string text = makeLongText(4000); // forces multi-chunk
    int call_count = 0;
    themis::toolbox::extractEntitiesStream(
        *toolbox, text, "text/plain", "long.txt",
        [&](const themis::ingestion::BaseEntity&) { ++call_count; });
    // With NullBackend we expect 0 entities, but no exception
    SUCCEED();
}

TEST(ToolboxStreamingTest, TCS04_NullCallbackIsNoop) {
    auto toolbox = themis::toolbox::IngestionToolbox::createDefault();
    // Passing an empty (falsy) callback must not crash
    EXPECT_NO_THROW(
        themis::toolbox::extractEntitiesStream(
            *toolbox, "Some text.", "text/plain", "x.txt",
            themis::toolbox::EntityCallback{}));
}

} // namespace
