/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_office_processor.cpp                          ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 04:05:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     346                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 3b1aefed2  2026-02-26  Audit: add content_office_extracted_total metrics, wire i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_office_processor.cpp
 * @brief Unit tests for Office document processor (DOCX/XLSX/PPTX)
 * 
 * Tests the office_processor module for extracting text and metadata
 * from Microsoft Office documents.
 */

#include <gtest/gtest.h>
#include "content/office_processor.h"
#include "content/content_metrics.h"
#include <string>
#include <vector>

using namespace themis::content;

class OfficeProcessorTest : public ::testing::Test {
protected:
    std::unique_ptr<OfficeProcessor> processor;
    
    void SetUp() override {
        OfficeProcessor::Config config;
        config.extract_text = true;
        config.extract_metadata = true;
        config.extract_speaker_notes = true;
        processor = std::make_unique<OfficeProcessor>(config);
    }
    
    void TearDown() override {
        processor.reset();
    }
    
    // Helper: Create a minimal ZIP file signature
    std::string createMinimalZipSignature() {
        std::string zip;
        // ZIP local file header signature: PK\x03\x04
        zip += "PK\x03\x04";
        // Add some padding
        zip.resize(100, '\0');
        return zip;
    }
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(OfficeProcessorTest, ProcessorInstantiation) {
    ASSERT_NE(processor, nullptr);
    EXPECT_EQ(processor->getName(), "OfficeProcessor");
}

TEST_F(OfficeProcessorTest, SupportedCategories) {
    auto categories = processor->getSupportedCategories();
    ASSERT_FALSE(categories.empty());
    
    // Office processor should support TEXT and STRUCTURED categories
    bool has_text = false;
    bool has_structured = false;
    
    for (const auto& cat : categories) {
        if (cat == ContentCategory::TEXT) has_text = true;
        if (cat == ContentCategory::STRUCTURED) has_structured = true;
    }
    
    EXPECT_TRUE(has_text) << "Office processor should support TEXT category";
    EXPECT_TRUE(has_structured) << "Office processor should support STRUCTURED category";
}

TEST_F(OfficeProcessorTest, IsAvailable) {
    // Check if office processing is available
    // This depends on whether THEMIS_ENABLE_OFFICE was defined at compile time
    bool available = OfficeProcessor::isAvailable();
    
#ifdef THEMIS_ENABLE_OFFICE
    EXPECT_TRUE(available) << "Office processor should be available when THEMIS_ENABLE_OFFICE is defined";
#else
    EXPECT_FALSE(available) << "Office processor should not be available when THEMIS_ENABLE_OFFICE is not defined";
#endif
}

// ============================================================================
// Document Type Detection Tests
// ============================================================================

TEST_F(OfficeProcessorTest, DetectDocumentType_Empty) {
    std::string empty_blob;
    auto doc_type = OfficeProcessor::detectDocumentType(empty_blob);
    EXPECT_EQ(doc_type, OfficeDocumentType::UNKNOWN);
}

TEST_F(OfficeProcessorTest, DetectDocumentType_TooSmall) {
    std::string small_blob = "PK";
    auto doc_type = OfficeProcessor::detectDocumentType(small_blob);
    EXPECT_EQ(doc_type, OfficeDocumentType::UNKNOWN);
}

TEST_F(OfficeProcessorTest, DetectDocumentType_DOCX) {
    std::string docx_blob = createMinimalZipSignature();
    docx_blob += "word/document.xml";
    
    auto doc_type = OfficeProcessor::detectDocumentType(docx_blob);
    EXPECT_EQ(doc_type, OfficeDocumentType::DOCX);
}

TEST_F(OfficeProcessorTest, DetectDocumentType_XLSX) {
    std::string xlsx_blob = createMinimalZipSignature();
    xlsx_blob += "xl/workbook.xml";
    
    auto doc_type = OfficeProcessor::detectDocumentType(xlsx_blob);
    EXPECT_EQ(doc_type, OfficeDocumentType::XLSX);
}

TEST_F(OfficeProcessorTest, DetectDocumentType_PPTX) {
    std::string pptx_blob = createMinimalZipSignature();
    pptx_blob += "ppt/presentation.xml";
    
    auto doc_type = OfficeProcessor::detectDocumentType(pptx_blob);
    EXPECT_EQ(doc_type, OfficeDocumentType::PPTX);
}

TEST_F(OfficeProcessorTest, DetectDocumentType_RTF) {
    std::string rtf_blob = "{\\rtf1\\ansi\\deff0 {\\fonttbl {\\f0 Times New Roman;}}";
    
    auto doc_type = OfficeProcessor::detectDocumentType(rtf_blob);
    EXPECT_EQ(doc_type, OfficeDocumentType::RTF);
}

// ============================================================================
// Extraction Tests
// ============================================================================

TEST_F(OfficeProcessorTest, ExtractFromInvalidBlob) {
    std::string invalid_blob = "This is not a valid Office document";
    ContentType content_type;
    content_type.mime_type = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    
    auto result = processor->extract(invalid_blob, content_type);
    
    // Should fail gracefully - either ok is false OR there's an error message
    EXPECT_TRUE(!result.ok || !result.error_message.empty()) 
        << "Invalid document should fail or have error message";
}

TEST_F(OfficeProcessorTest, ExtractFromEmptyBlob) {
    std::string empty_blob;
    ContentType content_type;
    content_type.mime_type = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    
    auto result = processor->extract(empty_blob, content_type);
    
    // Should fail or return empty result
    EXPECT_FALSE(result.ok);
}

// ============================================================================
// Chunking Tests
// ============================================================================

TEST_F(OfficeProcessorTest, ChunkEmptyText) {
    ExtractionResult extraction_result;
    extraction_result.text = "";
    extraction_result.ok = true;
    
    auto chunks = processor->chunk(extraction_result, 512, 50);
    EXPECT_TRUE(chunks.empty()) << "Empty text should produce no chunks";
}

TEST_F(OfficeProcessorTest, ChunkSimpleText) {
    ExtractionResult extraction_result;
    extraction_result.text = "First paragraph.\nSecond paragraph.\nThird paragraph.";
    extraction_result.ok = true;
    
    auto chunks = processor->chunk(extraction_result, 100, 10);
    
    // Should produce at least one chunk
    ASSERT_FALSE(chunks.empty()) << "Non-empty text should produce chunks";
    
    // Verify chunk structure
    for (const auto& chunk : chunks) {
        EXPECT_TRUE(chunk.contains("text")) << "Chunk should have 'text' field";
        EXPECT_TRUE(chunk.contains("seq_num")) << "Chunk should have 'seq_num' field";
        EXPECT_TRUE(chunk.contains("source_type")) << "Chunk should have 'source_type' field";
        
        if (chunk.contains("source_type")) {
            EXPECT_EQ(chunk["source_type"], "office") << "Source type should be 'office'";
        }
    }
}

// ============================================================================
// Factory Function Tests
// ============================================================================

TEST_F(OfficeProcessorTest, CreateOfficeProcessor_Default) {
    auto proc = createOfficeProcessor();
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->getName(), "OfficeProcessor");
}

TEST_F(OfficeProcessorTest, CreateOfficeProcessor_WithConfig) {
    OfficeProcessor::Config config;
    config.extract_text = true;
    config.extract_metadata = false;
    config.extract_speaker_notes = false;
    
    auto proc = createOfficeProcessor(config);
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->getName(), "OfficeProcessor");
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(OfficeProcessorTest, ConfigDefaultValues) {
    OfficeProcessor::Config config;
    
    EXPECT_TRUE(config.extract_text);
    EXPECT_TRUE(config.extract_metadata);
    EXPECT_TRUE(config.extract_comments);
    EXPECT_TRUE(config.extract_formulas);
    EXPECT_TRUE(config.extract_speaker_notes);
    EXPECT_FALSE(config.include_hidden_text);
    EXPECT_EQ(config.max_cell_count, 1000000);
    EXPECT_TRUE(config.password.empty());
    EXPECT_EQ(config.metrics, nullptr);
}

// ============================================================================
// Metadata Structure Tests
// ============================================================================

TEST_F(OfficeProcessorTest, MetadataInitialization) {
    OfficeMetadata metadata;
    
    // Verify default initialization
    EXPECT_TRUE(metadata.title.empty());
    EXPECT_TRUE(metadata.author.empty());
    EXPECT_TRUE(metadata.subject.empty());
    EXPECT_TRUE(metadata.keywords.empty());
}

TEST_F(OfficeProcessorTest, PowerPointInfoInitialization) {
    PowerPointInfo ppt_info;
    
    EXPECT_EQ(ppt_info.slide_count, 0);
    EXPECT_TRUE(ppt_info.slides.empty());
}

// ============================================================================
// Metrics reporting via ContentMetrics
// ============================================================================

TEST(OfficeProcessorMetricsTest, DefaultConfigHasNullMetrics) {
    OfficeProcessor::Config config;
    EXPECT_EQ(config.metrics, nullptr);
}

TEST(OfficeProcessorMetricsTest, SuccessfulRTFExtractionIncrementsCounter) {
    ContentMetrics metrics;
    EXPECT_EQ(metrics.getOfficeExtractedTotal(), 0u);

    OfficeProcessor::Config cfg;
    cfg.metrics = &metrics;
    OfficeProcessor proc(std::move(cfg));
    ContentType ct;

    // RTF blob — always succeeds via the basic regex path
    std::string rtf_blob = "{\\rtf1\\ansi\\deff0 Hello World}";
    auto result = proc.extract(rtf_blob, ct);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(metrics.getOfficeExtractedTotal(), 1u);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 0u);
}

TEST(OfficeProcessorMetricsTest, UnknownFormatIncrementsErrorCounter) {
    ContentMetrics metrics;
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 0u);

    OfficeProcessor::Config cfg;
    cfg.metrics = &metrics;
    OfficeProcessor proc(std::move(cfg));
    ContentType ct;

    // Completely unrecognised blob — detectDocumentType returns UNKNOWN
    auto result = proc.extract("This is not an Office document", ct);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 1u);
    EXPECT_EQ(metrics.getOfficeExtractedTotal(), 0u);
}

TEST(OfficeProcessorMetricsTest, NoMetricsPointerDoesNotCrash) {
    // Default config has metrics=nullptr; extraction must not segfault
    OfficeProcessor proc;
    ContentType ct;
    EXPECT_NO_FATAL_FAILURE(proc.extract("Not an office document.", ct));
}

TEST(OfficeProcessorMetricsTest, MetricsResetClearsOfficeCounters) {
    ContentMetrics metrics;
    metrics.recordOfficeExtracted();
    metrics.recordOfficeExtracted();
    metrics.recordExtractError();
    EXPECT_EQ(metrics.getOfficeExtractedTotal(), 2u);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 1u);

    metrics.reset();
    EXPECT_EQ(metrics.getOfficeExtractedTotal(), 0u);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 0u);
}

TEST(OfficeProcessorMetricsTest, PrometheusFormatContainsOfficeCounters) {
    ContentMetrics metrics;
    metrics.recordOfficeExtracted();
    metrics.recordOfficeExtracted();
    metrics.recordOfficeExtracted();
    metrics.recordExtractError();

    std::string prom = metrics.toPrometheusFormat();
    EXPECT_NE(prom.find("content_office_extracted_total 3"), std::string::npos);
    EXPECT_NE(prom.find("content_extract_errors_total 1"), std::string::npos);
}

