// Copyright (c) 2026 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_ocr_processor.cpp
 * @brief Unit tests for OcrProcessor - Tesseract OCR text extraction from images
 */

#include <gtest/gtest.h>
#include "content/ocr_processor.h"
#include "content/content_metrics.h"
#include "content/content_type.h"
#include <string>
#include <vector>
#include <cstdint>

using namespace themis::content;

// ============================================================================
// Helper: minimal image byte sequences (magic bytes only – not valid images,
// but sufficient for format-detection and unavailability-path tests)
// ============================================================================

/// Minimal JPEG header (3 magic bytes + padding)
static std::string makeMinimalJpeg() {
    std::string blob = {};
    blob += '\xFF'; blob += '\xD8'; blob += '\xFF';  // JPEG SOI + APP marker
    blob.append(64, '\x00');                           // padding
    return blob;
}

/// Minimal PNG header (4 magic bytes + padding)
static std::string makeMinimalPng() {
    std::string blob = {};
    blob += '\x89'; blob += 'P'; blob += 'N'; blob += 'G';
    blob.append(60, '\x00');
    return blob;
}

/// Minimal BMP header
static std::string makeMinimalBmp() {
    std::string blob = {};
    blob += 'B'; blob += 'M';
    blob.append(62, '\x00');
    return blob;
}

/// Minimal TIFF little-endian header
static std::string makeMinimalTiff() {
    std::string blob = {};
    blob += 'I'; blob += 'I';   // little-endian
    blob.append(62, '\x00');
    return blob;
}

/// Minimal GIF header
static std::string makeMinimalGif() {
    std::string blob = {};
    blob += 'G'; blob += 'I'; blob += 'F'; blob += '8';
    blob.append(60, '\x00');
    return blob;
}

/// Build a default ContentType for image testing
static ContentType makeImageContentType(const std::string& mime = "image/jpeg") {
    ContentType ct;
    ct.mime_type = mime;
    ct.category = ContentCategory::IMAGE;
    ct.supports_text_extraction = true;
    ct.supports_embedding = false;
    ct.supports_chunking = false;
    ct.supports_metadata_extraction = true;
    ct.binary_storage_required = true;
    return ct;
}

// ============================================================================
// OcrProcessor::Config defaults
// ============================================================================

TEST(OcrProcessorConfigTest, DefaultValues) {
    OcrProcessor::Config cfg;
    EXPECT_EQ(cfg.language, "eng");
    EXPECT_TRUE(cfg.data_dir.empty());
    EXPECT_EQ(cfg.page_seg_mode, 3);
    EXPECT_TRUE(cfg.extract_metadata);
    EXPECT_FALSE(cfg.enable_char_whitelist);
    EXPECT_TRUE(cfg.char_whitelist.empty());
    EXPECT_EQ(cfg.max_text_size, 1024u * 1024u);
    EXPECT_EQ(cfg.metrics, nullptr);
    // DPI rescaling defaults
    EXPECT_EQ(cfg.target_dpi, 300);
    EXPECT_TRUE(cfg.enable_dpi_rescaling);
    EXPECT_TRUE(cfg.enable_adaptive_binarization);
}

TEST(OcrProcessorConfigTest, CustomLanguage) {
    OcrProcessor::Config cfg;
    cfg.language = "deu";
    EXPECT_EQ(cfg.language, "deu");
}

TEST(OcrProcessorConfigTest, CharWhitelistConfiguration) {
    OcrProcessor::Config cfg;
    cfg.enable_char_whitelist = true;
    cfg.char_whitelist = "0123456789";
    EXPECT_TRUE(cfg.enable_char_whitelist);
    EXPECT_EQ(cfg.char_whitelist, "0123456789");
}

// ============================================================================
// OcrProcessor: constructor and getName
// ============================================================================

TEST(OcrProcessorTest, DefaultConstructor) {
    OcrProcessor proc;
    EXPECT_EQ(proc.getName(), "OcrProcessor");
}

TEST(OcrProcessorTest, ConfigConstructor) {
    OcrProcessor::Config cfg;
    cfg.language = "fra";
    OcrProcessor proc(std::move(cfg));
    EXPECT_EQ(proc.getName(), "OcrProcessor");
}

TEST(OcrProcessorTest, SupportedCategories) {
    OcrProcessor proc;
    auto cats = proc.getSupportedCategories();
    ASSERT_EQ(cats.size(), 1u);
    EXPECT_EQ(cats[0], ContentCategory::IMAGE);
}

// ============================================================================
// OcrProcessor::isAvailable and getTesseractVersion
// ============================================================================

TEST(OcrProcessorTest, IsAvailableReturnsCorrectValue) {
#ifdef THEMIS_ENABLE_OCR
    EXPECT_TRUE(OcrProcessor::isAvailable());
#else
    EXPECT_FALSE(OcrProcessor::isAvailable());
#endif
}

TEST(OcrProcessorTest, GetTesseractVersionNotEmpty) {
    // Should always return a non-empty string (either real version or fallback marker)
    EXPECT_FALSE(OcrProcessor::getTesseractVersion().empty());
}

TEST(OcrProcessorTest, GetTesseractVersionContainsNoneWhenUnavailable) {
#ifndef THEMIS_ENABLE_OCR
    EXPECT_NE(OcrProcessor::getTesseractVersion().find("none"), std::string::npos);
#else
    GTEST_SKIP() << "Tesseract is available - skipping unavailability check";
#endif
}

// ============================================================================
// OcrProcessor::extract - invalid / edge-case inputs
// ============================================================================

TEST(OcrProcessorTest, ExtractRejectsEmptyBlob) {
    OcrProcessor proc;
    ContentType ct = makeImageContentType();
    auto result = proc.extract("", ct);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(OcrProcessorTest, ExtractRejectsUnsupportedFormat) {
    OcrProcessor proc;
    ContentType ct = makeImageContentType("application/pdf");
    // Pass random bytes that don't match any known image magic
    std::string random_data = "NOTANIMAGE1234567890abcdef";
    auto result = proc.extract(random_data, ct);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(OcrProcessorTest, ExtractRejectsTooShortBlob) {
    OcrProcessor proc;
    ContentType ct = makeImageContentType();
    auto result = proc.extract("\xFF\xD8", ct);  // Only 2 bytes - too short
    EXPECT_FALSE(result.ok);
}

// ============================================================================
// OcrProcessor::extract - format detection with magic bytes
// (when Tesseract is unavailable the processor returns unavailability error;
//  when available but given truncated/invalid data it may return empty text)
// ============================================================================

TEST(OcrProcessorTest, ExtractDetectsJpegFormat) {
    OcrProcessor proc;
    ContentType ct = makeImageContentType("image/jpeg");
    auto result = proc.extract(makeMinimalJpeg(), ct);
#ifndef THEMIS_ENABLE_OCR
    // OCR unavailable: should fail with explicit message about missing library
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("OCR not available"), std::string::npos);
#else
    // OCR available: may succeed (empty text) or fail (truncated image) but
    // must not report "Unsupported image format"
    EXPECT_EQ(result.error_message.find("Unsupported image format"), std::string::npos);
#endif
}

TEST(OcrProcessorTest, ExtractDetectsPngFormat) {
    OcrProcessor proc;
    ContentType ct = makeImageContentType("image/png");
    auto result = proc.extract(makeMinimalPng(), ct);
#ifndef THEMIS_ENABLE_OCR
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("OCR not available"), std::string::npos);
#else
    EXPECT_EQ(result.error_message.find("Unsupported image format"), std::string::npos);
#endif
}

TEST(OcrProcessorTest, ExtractDetectsBmpFormat) {
    OcrProcessor proc;
    ContentType ct = makeImageContentType("image/bmp");
    auto result = proc.extract(makeMinimalBmp(), ct);
#ifndef THEMIS_ENABLE_OCR
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("OCR not available"), std::string::npos);
#else
    EXPECT_EQ(result.error_message.find("Unsupported image format"), std::string::npos);
#endif
}

TEST(OcrProcessorTest, ExtractDetectsTiffFormat) {
    OcrProcessor proc;
    ContentType ct = makeImageContentType("image/tiff");
    auto result = proc.extract(makeMinimalTiff(), ct);
#ifndef THEMIS_ENABLE_OCR
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("OCR not available"), std::string::npos);
#else
    EXPECT_EQ(result.error_message.find("Unsupported image format"), std::string::npos);
#endif
}

TEST(OcrProcessorTest, ExtractDetectsGifFormat) {
    OcrProcessor proc;
    ContentType ct = makeImageContentType("image/gif");
    auto result = proc.extract(makeMinimalGif(), ct);
#ifndef THEMIS_ENABLE_OCR
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("OCR not available"), std::string::npos);
#else
    EXPECT_EQ(result.error_message.find("Unsupported image format"), std::string::npos);
#endif
}

// ============================================================================
// OcrProcessor::extract - metadata population
// ============================================================================

TEST(OcrProcessorTest, ExtractPopulatesMetadataOnSuccess) {
    // This test only runs a meaningful check when OCR is available and the
    // processor returns ok=true.
#ifdef THEMIS_ENABLE_OCR
    // With real Tesseract available but a truncated image, extract may return
    // ok=false. We verify at the metadata level only when ok=true.
    OcrProcessor proc;
    ContentType ct = makeImageContentType("image/jpeg");
    auto result = proc.extract(makeMinimalJpeg(), ct);
    if (result.ok) {
        EXPECT_TRUE(result.metadata.contains("ocr_language"));
        EXPECT_TRUE(result.metadata.contains("ocr_text_length"));
        EXPECT_TRUE(result.metadata.contains("content_ocr_text"));
        EXPECT_EQ(result.metadata["ocr_language"].get<std::string>(), "eng");
    }
#else
    GTEST_SKIP() << "Tesseract is not available - skipping metadata test";
#endif
}

TEST(OcrProcessorTest, ExtractMetadataContainsMimeType) {
#ifdef THEMIS_ENABLE_OCR
    OcrProcessor proc;
    ContentType ct = makeImageContentType("image/png");
    auto result = proc.extract(makeMinimalPng(), ct);
    if (result.ok) {
        EXPECT_EQ(result.metadata.value("mime_type", ""), "image/png");
    }
#else
    GTEST_SKIP() << "Tesseract is not available";
#endif
}

// ============================================================================
// OcrProcessor::extract - metrics integration
// ============================================================================

TEST(OcrProcessorTest, MetricsRecordExtractErrorOnUnavailableOcr) {
#ifndef THEMIS_ENABLE_OCR
    ContentMetrics metrics;
    uint64_t before = metrics.getExtractErrorsTotal();

    OcrProcessor::Config cfg;
    cfg.metrics = &metrics;
    OcrProcessor proc(std::move(cfg));

    ContentType ct = makeImageContentType();
    proc.extract(makeMinimalJpeg(), ct);

    EXPECT_GT(metrics.getExtractErrorsTotal(), before);
#else
    GTEST_SKIP() << "Tesseract is available - unavailability error path not triggered";
#endif
}

TEST(OcrProcessorTest, MetricsRecordOcrExtractedOnSuccess) {
#ifdef THEMIS_ENABLE_OCR
    ContentMetrics metrics;
    uint64_t before = metrics.getOcrExtractedTotal();

    OcrProcessor::Config cfg;
    cfg.metrics = &metrics;
    OcrProcessor proc(std::move(cfg));

    ContentType ct = makeImageContentType();
    auto result = proc.extract(makeMinimalJpeg(), ct);
    if (result.ok) {
        EXPECT_GT(metrics.getOcrExtractedTotal(), before);
    }
#else
    GTEST_SKIP() << "Tesseract is not available";
#endif
}

// ============================================================================
// OcrProcessor::chunk
// ============================================================================

TEST(OcrProcessorTest, ChunkFailedResultReturnsEmpty) {
    OcrProcessor proc;
    ExtractionResult er;
    er.ok = false;
    er.text = "Some text";
    auto chunks = proc.chunk(er, 200, 0);
    EXPECT_TRUE(chunks.empty());
}

TEST(OcrProcessorTest, ChunkEmptyTextReturnsEmpty) {
    OcrProcessor proc;
    ExtractionResult er;
    er.ok = true;
    er.text = "";
    auto chunks = proc.chunk(er, 200, 0);
    EXPECT_TRUE(chunks.empty());
}

TEST(OcrProcessorTest, ChunkNoSplitReturnsOneChunk) {
    OcrProcessor proc;
    ExtractionResult er;
    er.ok = true;
    er.text = "This is a short OCR text sentence.";
    auto chunks = proc.chunk(er, 0, 0);
    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0]["type"].get<std::string>(), "ocr_text");
    EXPECT_EQ(chunks[0]["sequence"].get<int>(), 0);
    EXPECT_EQ(chunks[0]["text"].get<std::string>(), er.text);
}

TEST(OcrProcessorTest, ChunkShortTextProducesOneChunk) {
    OcrProcessor proc;
    ExtractionResult er;
    er.ok = true;
    er.text = "Hello world. This is OCR text.";
    auto chunks = proc.chunk(er, 500, 0);
    ASSERT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0]["type"].get<std::string>(), "ocr_text");
    EXPECT_EQ(chunks[0]["sequence"].get<int>(), 0);
}

TEST(OcrProcessorTest, ChunkMultipleSentencesProducesMultipleChunks) {
    OcrProcessor proc;
    ExtractionResult er;
    er.ok = true;
    // Construct text with many words so it exceeds a small chunk_size
    er.text = "First sentence with many words here. "
              "Second sentence with more words too. "
              "Third sentence adds even more words now. "
              "Fourth sentence provides additional content text. "
              "Fifth sentence completes this longer text.";
    // Use a very small chunk_size (5 tokens) to force multiple chunks
    auto chunks = proc.chunk(er, 5, 0);
    EXPECT_GT(chunks.size(), 1u);
    // Verify sequence numbers are monotonically increasing
    for (size_t i = 0; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i]["sequence"].get<int>(), static_cast<int>(i));
    }
}

TEST(OcrProcessorTest, ChunkResultsHaveRequiredFields) {
    OcrProcessor proc;
    ExtractionResult er;
    er.ok = true;
    er.text = "A simple OCR sentence here.";
    auto chunks = proc.chunk(er, 100, 0);
    for (const auto& chunk : chunks) {
        EXPECT_TRUE(chunk.contains("text"));
        EXPECT_TRUE(chunk.contains("type"));
        EXPECT_TRUE(chunk.contains("sequence"));
        EXPECT_TRUE(chunk.contains("token_count"));
        EXPECT_EQ(chunk["type"].get<std::string>(), "ocr_text");
    }
}

// ============================================================================
// OcrProcessor::generateEmbedding
// ============================================================================

TEST(OcrProcessorTest, GenerateEmbeddingNonEmptyForText) {
    OcrProcessor proc;
    auto emb = proc.generateEmbedding("some OCR text");
    EXPECT_EQ(static_cast<int>(emb.size()), 768);
    // L2-normalised result must have a non-zero norm
    float norm = 0.0f;
    for (float v : emb) {
      norm += v * v;
    }
    EXPECT_GT(norm, 0.0f);
}

TEST(OcrProcessorTest, GenerateEmbeddingEmptyInputReturnsZeroVector) {
    OcrProcessor proc;
    auto emb = proc.generateEmbedding("");
    EXPECT_EQ(static_cast<int>(emb.size()), 768);
    for (float v : emb) {
        EXPECT_FLOAT_EQ(v, 0.0f);
    }
}

TEST(OcrProcessorTest, GenerateEmbeddingDifferentTextsProduceDifferentVectors) {
    OcrProcessor proc;
    auto emb1 = proc.generateEmbedding("hello world");
    auto emb2 = proc.generateEmbedding("completely different text");
    ASSERT_EQ(emb1.size(), emb2.size());
    bool differs = false;
    for (size_t i = 0; i < emb1.size(); ++i) {
        if (emb1[i] != emb2[i]) { differs = true; break; }
    }
    EXPECT_TRUE(differs);
}

// ============================================================================
// OcrProcessor::performOcr - static convenience method
// ============================================================================

TEST(OcrProcessorTest, PerformOcrEmptyBlobReturnsEmpty) {
    std::vector<uint8_t> empty;
    std::string result = OcrProcessor::performOcr(empty);
    EXPECT_TRUE(result.empty());
}

TEST(OcrProcessorTest, PerformOcrUnsupportedFormatReturnsEmpty) {
    std::vector<uint8_t> garbage = {0x00, 0x01, 0x02, 0x03, 0x04};
    std::string result = OcrProcessor::performOcr(garbage);
    EXPECT_TRUE(result.empty());
}

TEST(OcrProcessorTest, PerformOcrReturnsStringType) {
    // Just verify the method compiles and returns a string without crashing
    std::vector<uint8_t> jpeg_magic = {0xFF, 0xD8, 0xFF, 0x00};
    jpeg_magic.resize(64, 0x00);
    std::string result = OcrProcessor::performOcr(jpeg_magic);
    // Result is a string (empty when OCR unavailable or Tesseract can't decode the blob)
    // No assertion on content - just that it doesn't throw
    (void)result;
}

// ============================================================================
// Factory functions
// ============================================================================

TEST(OcrProcessorFactoryTest, CreateOcrProcessorDefault) {
    auto proc = createOcrProcessor();
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->getName(), "OcrProcessor");
}

TEST(OcrProcessorFactoryTest, CreateOcrProcessorWithConfig) {
    OcrProcessor::Config cfg;
    cfg.language = "deu";
    auto proc = createOcrProcessor(std::move(cfg));
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->getName(), "OcrProcessor");
}

// ============================================================================
// OcrProcessor: data_dir default via ConfigPathResolver
// ============================================================================

TEST(OcrProcessorConfigTest, DefaultDataDirIsEmpty) {
    // The Config struct intentionally keeps data_dir empty so the processor
    // can resolve it lazily at OCR-time via ConfigPathResolver.
    OcrProcessor::Config cfg;
    EXPECT_TRUE(cfg.data_dir.empty());
}

TEST(OcrProcessorConfigTest, ExplicitDataDirOverridesDefault) {
    OcrProcessor::Config cfg;
    cfg.data_dir = "/custom/tessdata";
    EXPECT_EQ(cfg.data_dir, "/custom/tessdata");
}

TEST(OcrProcessorConfigTest, PerCollectionLanguageOverride) {
    // Verify that per-collection configs can set a custom language while
    // leaving data_dir empty (resolved to default at OCR-time).
    OcrProcessor::Config cfg;
    cfg.language = "deu";
    EXPECT_EQ(cfg.language, "deu");
    EXPECT_TRUE(cfg.data_dir.empty());
}

TEST(OcrProcessorConfigTest, DefaultLanguageFallbackIsEng) {
    // language must default to "eng" unless overridden by the caller.
    OcrProcessor::Config cfg;
    EXPECT_EQ(cfg.language, "eng");
}

TEST(OcrProcessorConfigTest, EmptyDataDirDoesNotCrashExtract) {
    // When data_dir is empty the processor resolves via ConfigPathResolver.
    // If config/ai_ml/tesseract_lang does not exist on this machine Tesseract
    // auto-detects; if it does exist it is used as tessdata.  Either way the
    // processor must not crash.
    OcrProcessor::Config cfg;
    EXPECT_TRUE(cfg.data_dir.empty());

    OcrProcessor proc(std::move(cfg));
    std::string jpeg_magic = {};
    jpeg_magic += '\xFF'; jpeg_magic += '\xD8'; jpeg_magic += '\xFF';
    jpeg_magic.append(64, '\x00');

    ContentType ct;
    ct.mime_type = "image/jpeg";
    ct.category = ContentCategory::IMAGE;
    ct.supports_text_extraction = true;
    ct.supports_embedding = false;
    ct.supports_chunking = false;
    ct.supports_metadata_extraction = true;
    ct.binary_storage_required = true;

    // Must not throw regardless of whether OCR or the tessdata dir is available.
    EXPECT_NO_THROW({ auto result = proc.extract(jpeg_magic, ct); (void)result; });
}

TEST(OcrProcessorConfigTest, PerformOcrEmptyDataDirDoesNotCrash) {
    // Static performOcr() with empty data_dir must also resolve gracefully.
    std::vector<uint8_t> jpeg_magic = {0xFF, 0xD8, 0xFF, 0x00};
    jpeg_magic.resize(64, 0x00);
    std::string result = {};
    EXPECT_NO_THROW(result = OcrProcessor::performOcr(jpeg_magic, "eng", ""));
    // Result is a string; no crash is the key requirement.
    (void)result;
}

// DPI rescaling and adaptive binarisation – config flags
// ============================================================================

TEST(OcrProcessorConfigTest, DpiRescalingConfigFields) {
    OcrProcessor::Config cfg;
    // target_dpi default is 300
    EXPECT_EQ(cfg.target_dpi, 300);

    // Can be overridden
    cfg.target_dpi = 200;
    EXPECT_EQ(cfg.target_dpi, 200);

    // Rescaling can be disabled
    cfg.enable_dpi_rescaling = false;
    EXPECT_FALSE(cfg.enable_dpi_rescaling);
}

TEST(OcrProcessorConfigTest, AdaptiveBinarizationConfigField) {
    OcrProcessor::Config cfg;
    EXPECT_TRUE(cfg.enable_adaptive_binarization);

    // Can be disabled
    cfg.enable_adaptive_binarization = false;
    EXPECT_FALSE(cfg.enable_adaptive_binarization);
}

// ============================================================================
// DPI rescaling and adaptive binarisation – extract() metadata
// ============================================================================

TEST(OcrProcessorTest, ExtractPopulatesPreprocessingMetadata) {
    // Verify that the three new preprocessing metadata fields are always present
    // when extract_metadata=true, regardless of whether OCR is available.
    // (When OCR is unavailable, extract() returns ok=false before populating
    //  metadata; so we can only assert their presence when OCR is available.)
#ifdef THEMIS_ENABLE_OCR
    OcrProcessor proc;
    ContentType ct = makeImageContentType("image/jpeg");
    auto result = proc.extract(makeMinimalJpeg(), ct);
    if (result.ok) {
        EXPECT_TRUE(result.metadata.contains("ocr_input_dpi"));
        EXPECT_TRUE(result.metadata.contains("ocr_rescaled"));
        EXPECT_TRUE(result.metadata.contains("ocr_binarized"));
    }
#else
    GTEST_SKIP() << "Tesseract not available – metadata fields not populated on failure path";
#endif
}

TEST(OcrProcessorTest, ExtractMetadataSuppressedWhenExtractMetadataFalse) {
    OcrProcessor::Config cfg;
    cfg.extract_metadata = false;
    OcrProcessor proc(std::move(cfg));
    ContentType ct = makeImageContentType("image/jpeg");
    auto result = proc.extract(makeMinimalJpeg(), ct);
    // Regardless of ok, no preprocessing metadata keys must be set
    EXPECT_FALSE(result.metadata.contains("ocr_input_dpi"));
    EXPECT_FALSE(result.metadata.contains("ocr_rescaled"));
    EXPECT_FALSE(result.metadata.contains("ocr_binarized"));
}

TEST(OcrProcessorTest, ExtractPreprocessingFlagsAreFalseWhenFeaturesDisabled) {
    // When both pre-processing features are disabled, the corresponding flags
    // in result.metadata must be false.
#ifdef THEMIS_ENABLE_OCR
    OcrProcessor::Config cfg;
    cfg.enable_dpi_rescaling       = false;
    cfg.enable_adaptive_binarization = false;
    OcrProcessor proc(std::move(cfg));

    ContentType ct = makeImageContentType("image/jpeg");
    auto result = proc.extract(makeMinimalJpeg(), ct);
    if (result.ok) {
        EXPECT_FALSE(result.metadata.value("ocr_rescaled",  true));
        EXPECT_FALSE(result.metadata.value("ocr_binarized", true));
    }
#else
    GTEST_SKIP() << "Tesseract not available";
#endif
}

// ============================================================================
// DPI rescaling and adaptive binarisation – low-res simulated inputs
// ============================================================================

// Low-resolution simulation: disabling rescaling/binarisation should not
// change the OCR availability result (regression guard).
TEST(OcrProcessorTest, LowResSampleDpiRescalingDisabledNoRegression) {
    OcrProcessor::Config cfg;
    cfg.enable_dpi_rescaling = false;
    cfg.enable_adaptive_binarization = false;
    OcrProcessor proc(std::move(cfg));

    ContentType ct = makeImageContentType("image/jpeg");
    auto result_off = proc.extract(makeMinimalJpeg(), ct);

    // Compare against a processor with defaults (rescaling+binarisation on)
    OcrProcessor proc_default;
    auto result_on = proc_default.extract(makeMinimalJpeg(), ct);

    // Both should agree on whether OCR is available
    bool off_unavailable = (result_off.error_message.find("OCR not available") != std::string::npos);
    bool on_unavailable  = (result_on.error_message.find("OCR not available")  != std::string::npos);
    EXPECT_EQ(off_unavailable, on_unavailable);
}

// Verify that a non-300 target DPI can be configured and the processor still
// runs without crashing.
TEST(OcrProcessorTest, LowResSampleCustomTargetDpi) {
    OcrProcessor::Config cfg;
    cfg.target_dpi = 150;  // non-standard target
    OcrProcessor proc(std::move(cfg));

    ContentType ct = makeImageContentType("image/jpeg");
    // Must not throw; result content depends on OCR availability
    EXPECT_NO_THROW({
        auto result = proc.extract(makeMinimalJpeg(), ct);
        (void)result;
    });
}

// Verify that binarisation-only mode (rescaling disabled) also runs cleanly.
TEST(OcrProcessorTest, LowResSampleBinarisationOnlyMode) {
    OcrProcessor::Config cfg;
    cfg.enable_dpi_rescaling = false;
    cfg.enable_adaptive_binarization = true;
    OcrProcessor proc(std::move(cfg));

    ContentType ct = makeImageContentType("image/png");
    EXPECT_NO_THROW({
        auto result = proc.extract(makeMinimalPng(), ct);
        (void)result;
    });
}


// ============================================================================
// Integration: MimeDetector OCR routing → OcrProcessor
// ============================================================================

#include "content/content_policy.h"
#include "content/mime_detector.h"

/// Verify that shouldTriggerOcr() correctly gates OcrProcessor invocation
/// for the three OCR-eligible MIME types.
TEST(OcrMimeRoutingIntegrationTest, OcrTriggeredForPng_WhenEnabled) {
    MimeDetector detector;
    detector.enableOcr(true);

    // Confirm routing decision
    EXPECT_TRUE(detector.shouldTriggerOcr("image/png"));

    // Confirm OcrProcessor is called and handles the format gracefully
    std::vector<uint8_t> png_magic = {0x89, 'P', 'N', 'G'};
    png_magic.resize(64, 0x00);
    std::string text = OcrProcessor::performOcr(png_magic);
    // When Tesseract is unavailable or the image is minimal, result is empty.
    // The important thing is that it does not throw and returns a string.
    (void)text;
}

TEST(OcrMimeRoutingIntegrationTest, OcrTriggeredForJpeg_WhenEnabled) {
    MimeDetector detector;
    detector.enableOcr(true);

    EXPECT_TRUE(detector.shouldTriggerOcr("image/jpeg"));

    std::vector<uint8_t> jpeg_magic = {0xFF, 0xD8, 0xFF};
    jpeg_magic.resize(64, 0x00);
    std::string text = OcrProcessor::performOcr(jpeg_magic);
    (void)text;
}

TEST(OcrMimeRoutingIntegrationTest, OcrTriggeredForTiff_WhenEnabled) {
    MimeDetector detector;
    detector.enableOcr(true);

    EXPECT_TRUE(detector.shouldTriggerOcr("image/tiff"));

    // Minimal TIFF header (little-endian)
    std::vector<uint8_t> tiff_magic = {'I', 'I', 0x2A, 0x00};
    tiff_magic.resize(64, 0x00);
    std::string text = OcrProcessor::performOcr(tiff_magic);
    (void)text;
}

TEST(OcrMimeRoutingIntegrationTest, OcrNotTriggeredWhenDisabled) {
    MimeDetector detector;
    // OCR disabled by default — no trigger even for image/png

    EXPECT_FALSE(detector.shouldTriggerOcr("image/png"));
    EXPECT_FALSE(detector.shouldTriggerOcr("image/jpeg"));
    EXPECT_FALSE(detector.shouldTriggerOcr("image/tiff"));
}

TEST(OcrMimeRoutingIntegrationTest, OcrNotTriggeredForNonOcrMimeTypes) {
    MimeDetector detector;
    detector.enableOcr(true);

    // image/gif and image/bmp are not in the OCR-eligible list
    EXPECT_FALSE(detector.shouldTriggerOcr("image/gif"));
    EXPECT_FALSE(detector.shouldTriggerOcr("image/bmp"));
    // Text and documents must never trigger OCR
    EXPECT_FALSE(detector.shouldTriggerOcr("text/plain"));
    EXPECT_FALSE(detector.shouldTriggerOcr("application/pdf"));
}

TEST(OcrMimeRoutingIntegrationTest, ContentPolicyOcrEnabledGatesDetector) {
    // ContentPolicy with ocr_enabled drives MimeDetector via enableOcr()
    ContentPolicy policy;
    ASSERT_FALSE(policy.ocrEnabled());

    MimeDetector detector;
    detector.enableOcr(policy.ocrEnabled());
    EXPECT_FALSE(detector.shouldTriggerOcr("image/png"));

    policy.ocr_enabled = true;
    detector.enableOcr(policy.ocrEnabled());
    EXPECT_TRUE(detector.shouldTriggerOcr("image/png"));
    EXPECT_TRUE(detector.shouldTriggerOcr("image/jpeg"));
    EXPECT_TRUE(detector.shouldTriggerOcr("image/tiff"));
}

TEST(OcrMimeRoutingIntegrationTest, StatelessOverload_ContentPolicyOcrFlag) {
    // The two-argument overload used by ContentManager::ingestRawBlob is stateless:
    // it must agree with the single-argument overload when the flag matches.
    ContentPolicy policy;
    MimeDetector detector;

    // OCR disabled: both overloads must return false
    detector.enableOcr(policy.ocrEnabled());
    EXPECT_FALSE(detector.shouldTriggerOcr("image/png", policy.ocrEnabled()));
    EXPECT_EQ(detector.shouldTriggerOcr("image/png"),
              detector.shouldTriggerOcr("image/png", policy.ocrEnabled()));

    // OCR enabled: both overloads must return true for eligible MIME types
    policy.ocr_enabled = true;
    detector.enableOcr(policy.ocrEnabled());
    EXPECT_TRUE(detector.shouldTriggerOcr("image/png",  policy.ocrEnabled()));
    EXPECT_TRUE(detector.shouldTriggerOcr("image/jpeg", policy.ocrEnabled()));
    EXPECT_TRUE(detector.shouldTriggerOcr("image/tiff", policy.ocrEnabled()));
    EXPECT_EQ(detector.shouldTriggerOcr("image/png"),
              detector.shouldTriggerOcr("image/png", policy.ocrEnabled()));
}

/// Config::data_dir starts empty (lazy resolution at OCR time).
TEST(OcrProcessorDefaultDataDirTest, DefaultDataDirIsEmpty) {
    OcrProcessor::Config cfg;
    EXPECT_TRUE(cfg.data_dir.empty());
}

/// An explicit data_dir override is preserved unchanged.
TEST(OcrProcessorDefaultDataDirTest, ExplicitDataDirOverridesDefault) {
    OcrProcessor::Config cfg;
    cfg.data_dir = "/custom/tessdata";
    EXPECT_EQ(cfg.data_dir, "/custom/tessdata");
}

/// Per-collection language override leaves data_dir empty for lazy resolution.
TEST(OcrProcessorDefaultDataDirTest, PerCollectionLanguageOverride) {
    OcrProcessor::Config cfg;
    cfg.language = "deu";
    EXPECT_EQ(cfg.language, "deu");
    EXPECT_TRUE(cfg.data_dir.empty());
}

/// Language defaults to "eng" when not overridden.
TEST(OcrProcessorDefaultDataDirTest, DefaultLanguageFallbackIsEng) {
    OcrProcessor::Config cfg;
    EXPECT_EQ(cfg.language, "eng");
}

// ============================================================================
// OCR output sanitization — control character stripping
// ============================================================================

/// extract() on a blob that would produce control characters (tested via
/// the non-OCR path so no Tesseract runtime is needed) shows that
/// sanitizeOcrText is exercised on the empty-unavailable code path gracefully.
TEST(OcrProcessorSanitizationTest, ExtractWithNoOcrYieldsCleanResult) {
    // Without -DTHEMIS_ENABLE_OCR the processor reports unavailable;
    // the important invariant is that result.text never contains
    // ASCII control characters (0x00–0x1F except \t\n\r, or 0x7F).
    OcrProcessor proc;
    std::string fake_blob(4, '\0');  // 4 null bytes – not a valid image header
    // 0x89 is the first byte of a PNG file signature; used here so the blob
    // passes the magic-byte check without requiring a full valid PNG fixture.
    fake_blob[0] = static_cast<char>(0x89);
    ContentType ct;
    ct.mime_type = "image/png";
    ct.category  = ContentCategory::IMAGE;
    auto result = proc.extract(fake_blob, ct);
    // Regardless of OCR availability, the text must contain no raw control chars
    for (unsigned char c : result.text) {
        bool allowed = (c == 0x09 || c == 0x0A || c == 0x0D || c >= 0x20);
        EXPECT_TRUE(allowed) << "Unexpected control char 0x" << std::hex << static_cast<int>(c);
    }
}


// Note: No custom main here; linked with GTest::gtest_main
