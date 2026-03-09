/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_ocr_processor.cpp                             ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:05:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     486                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 718c75097  2026-02-28  feat(content): Integrate Tesseract OCR processor (content... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
    std::string blob;
    blob += '\xFF'; blob += '\xD8'; blob += '\xFF';  // JPEG SOI + APP marker
    blob.append(64, '\x00');                           // padding
    return blob;
}

/// Minimal PNG header (4 magic bytes + padding)
static std::string makeMinimalPng() {
    std::string blob;
    blob += '\x89'; blob += 'P'; blob += 'N'; blob += 'G';
    blob.append(60, '\x00');
    return blob;
}

/// Minimal BMP header
static std::string makeMinimalBmp() {
    std::string blob;
    blob += 'B'; blob += 'M';
    blob.append(62, '\x00');
    return blob;
}

/// Minimal TIFF little-endian header
static std::string makeMinimalTiff() {
    std::string blob;
    blob += 'I'; blob += 'I';   // little-endian
    blob.append(62, '\x00');
    return blob;
}

/// Minimal GIF header
static std::string makeMinimalGif() {
    std::string blob;
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
// OcrProcessor::generateEmbedding (stub)
// ============================================================================

TEST(OcrProcessorTest, GenerateEmbeddingReturnsEmptyVector) {
    OcrProcessor proc;
    auto emb = proc.generateEmbedding("some OCR text");
    EXPECT_TRUE(emb.empty());
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

// Note: No custom main here; linked with GTest::gtest_main
