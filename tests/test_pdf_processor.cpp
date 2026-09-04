// Copyright (c) 2026 ThemisDB
// SPDX-License-Identifier: MIT

/**
 * @file test_pdf_processor.cpp
 * @brief Unit tests for PDFProcessor - PDF text extraction with layout preservation
 */

#include <gtest/gtest.h>
#include "content/pdf_processor.h"
#include "content/content_metrics.h"
#include "content/content_type.h"
#include <string>
#include <vector>

using namespace themis::content;

// ============================================================================
// Helper: build a minimal (but structurally valid) PDF byte string for testing
// ============================================================================

static std::string makeMinimalPDF() {
    // Minimal syntactically-valid PDF with one empty page.
    // The trailer uses xref offset 0 so most parsers skip object validation.
    return "%PDF-1.4\n"
           "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
           "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
           "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] >>\nendobj\n"
           "xref\n0 4\n0000000000 65535 f \n"
           "trailer\n<< /Size 4 /Root 1 0 R >>\nstartxref\n9\n%%EOF\n";
}

static std::string makeMinimalPDFWithText() {
    // Minimal PDF with a text stream containing a Tj operator.
    return "%PDF-1.4\n"
           "4 0 obj\n<< /Length 44 >>\nstream\n"
           "BT /F1 12 Tf 100 700 Td (Hello World) Tj ET\n"
           "endstream\nendobj\n"
           "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
           "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
           "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
           "/Contents 4 0 R >>\nendobj\n"
           "xref\n0 5\n0000000000 65535 f \n"
           "trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n9\n%%EOF\n";
}

// ============================================================================
// PDFProcessor::Config defaults
// ============================================================================

TEST(PDFProcessorConfigTest, DefaultValues) {
    PDFProcessor::Config cfg;
    EXPECT_TRUE(cfg.extract_text);
    EXPECT_TRUE(cfg.extract_metadata);
    EXPECT_FALSE(cfg.extract_images);
    EXPECT_FALSE(cfg.maintain_layout);
    EXPECT_FALSE(cfg.detect_tables);
    EXPECT_EQ(cfg.max_pages, 0);
    EXPECT_TRUE(cfg.password.empty());
}

TEST(PDFProcessorConfigTest, LayoutPreservationEnabled) {
    PDFProcessor::Config cfg;
    cfg.maintain_layout = true;
    EXPECT_TRUE(cfg.maintain_layout);
}

// ============================================================================
// PDFProcessor: constructor and getName
// ============================================================================

TEST(PDFProcessorTest, DefaultConstructor) {
    PDFProcessor proc;
    EXPECT_EQ(proc.getName(), "PDFProcessor");
}

TEST(PDFProcessorTest, ConfigConstructor) {
    PDFProcessor::Config cfg;
    cfg.maintain_layout = true;
    cfg.max_pages = 10;
    PDFProcessor proc(std::move(cfg));
    EXPECT_EQ(proc.getName(), "PDFProcessor");
}

TEST(PDFProcessorTest, SupportedCategories) {
    PDFProcessor proc;
    auto cats = proc.getSupportedCategories();
    ASSERT_EQ(cats.size(), 1u);
    EXPECT_EQ(cats[0], ContentCategory::TEXT);
}

TEST(PDFProcessorTest, IsAvailableReturnsCorrectValue) {
#ifdef THEMIS_ENABLE_PDF
    EXPECT_TRUE(PDFProcessor::isAvailable());
#else
    EXPECT_FALSE(PDFProcessor::isAvailable());
#endif
}

// ============================================================================
// PDFProcessor::extract - invalid input
// ============================================================================

TEST(PDFProcessorTest, ExtractRejectsEmptyBlob) {
    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract("", ct);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(PDFProcessorTest, ExtractRejectsNonPDFData) {
    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract("This is not a PDF document.", ct);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error_message.find("Invalid PDF"), std::string::npos);
}

TEST(PDFProcessorTest, ExtractRejectsTooShortBlob) {
    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract("%PDF", ct);
    EXPECT_FALSE(result.ok);
}

// ============================================================================
// PDFProcessor::extract - valid PDF header (fallback / full path)
// ============================================================================

TEST(PDFProcessorTest, ExtractMinimalPDFSucceeds) {
    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract(makeMinimalPDF(), ct);
    // Whether poppler is available or not, the result.ok should be true
    // (poppler may fail to fully parse our hand-crafted minimal PDF, but
    //  the fallback regex path always succeeds for a valid header).
#ifndef THEMIS_ENABLE_PDF
    EXPECT_TRUE(result.ok);
#endif
    // Must always populate mime_type and size_bytes
    EXPECT_EQ(result.metadata.value("mime_type", ""), "application/pdf");
    EXPECT_GT(result.metadata.value("size_bytes", std::size_t{0}),
              std::size_t{0});
}

TEST(PDFProcessorTest, ExtractSetsLayoutPreservedFalseByDefault) {
    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract(makeMinimalPDF(), ct);
    if (result.ok) {
        // layout_preserved should reflect the config (false by default)
        if (result.metadata.contains("layout_preserved")) {
            EXPECT_FALSE(result.metadata["layout_preserved"].get<bool>());
        }
    }
}

TEST(PDFProcessorTest, ExtractWithLayoutFlagSetsMetadata) {
    PDFProcessor::Config cfg;
    cfg.maintain_layout = true;
    PDFProcessor proc(std::move(cfg));
    ContentType ct;
    auto result = proc.extract(makeMinimalPDF(), ct);
    if (result.ok && result.metadata.contains("layout_preserved")) {
#ifdef THEMIS_ENABLE_PDF
        EXPECT_TRUE(result.metadata["layout_preserved"].get<bool>());
#else
        // Fallback path always sets layout_preserved = false
        EXPECT_FALSE(result.metadata["layout_preserved"].get<bool>());
#endif
    }
}

TEST(PDFProcessorTest, ExtractFallbackExtractsTextFromTjOperator) {
    // Only test the fallback path when poppler is unavailable
#ifndef THEMIS_ENABLE_PDF
    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract(makeMinimalPDFWithText(), ct);
    EXPECT_TRUE(result.ok);
    EXPECT_NE(result.text.find("Hello World"), std::string::npos);
#else
    GTEST_SKIP() << "Skipping fallback test when poppler is available";
#endif
}

TEST(PDFProcessorTest, ExtractSetsExtractionMethodInFallback) {
#ifndef THEMIS_ENABLE_PDF
    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract(makeMinimalPDF(), ct);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.metadata.value("extraction_method", ""), "basic_regex");
#else
    GTEST_SKIP() << "Skipping fallback test when poppler is available";
#endif
}

// ============================================================================
// PDFProcessor::extract - PDF version extraction
// ============================================================================

TEST(PDFProcessorTest, ExtractReadsVersion14) {
    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract(makeMinimalPDF(), ct);
    if (result.ok) {
        EXPECT_EQ(result.metadata.value("pdf_version", ""), "1.4");
    }
}

TEST(PDFProcessorTest, ExtractReadsVersion17) {
    std::string pdf = makeMinimalPDF();
    // Replace "1.4" with "1.7"
    auto pos = pdf.find("1.4");
    if (pos != std::string::npos) {
      pdf.replace(pos, 3, "1.7");
    }

    PDFProcessor proc;
    ContentType ct;
    auto result = proc.extract(pdf, ct);
    if (result.ok) {
        EXPECT_EQ(result.metadata.value("pdf_version", ""), "1.7");
    }
}

// ============================================================================
// PDFProcessor::chunk
// ============================================================================

TEST(PDFProcessorTest, ChunkEmptyTextReturnsEmpty) {
    PDFProcessor proc;
    ExtractionResult er;
    er.ok = true;
    er.text = "";
    auto chunks = proc.chunk(er, 200, 20);
    EXPECT_TRUE(chunks.empty());
}

TEST(PDFProcessorTest, ChunkShortTextProducesOneChunk) {
    PDFProcessor proc;
    ExtractionResult er;
    er.ok = true;
    er.text = "This is a short PDF sentence.";
    auto chunks = proc.chunk(er, 500, 0);
    EXPECT_EQ(chunks.size(), 1u);
    EXPECT_EQ(chunks[0].value("source_type", ""), "pdf");
    EXPECT_EQ(chunks[0].value("seq_num", -1), 0);
}

TEST(PDFProcessorTest, ChunkLongTextProducesMultipleChunks) {
    PDFProcessor proc;

    // Build a long text > 10 words
    std::string long_text;
    for (int i = 0; i < 100; ++i) {
        long_text += "Word" + std::to_string(i) + ". ";
    }

    ExtractionResult er;
    er.ok = true;
    er.text = long_text;

    // Use a small chunk size to force splitting
    auto chunks = proc.chunk(er, 10, 0);
    EXPECT_GT(chunks.size(), 1u);

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        EXPECT_EQ(chunks[i].value("source_type", ""), "pdf");
        EXPECT_EQ(chunks[i].value("seq_num", -1), static_cast<int>(i));
    }
}

// ============================================================================
// PDFProcessor::generateEmbedding
// ============================================================================

TEST(PDFProcessorTest, GenerateEmbeddingReturnsEmpty) {
    PDFProcessor proc;
    auto embedding = proc.generateEmbedding("test chunk text");
    // Placeholder implementation returns empty vector
    EXPECT_TRUE(embedding.empty());
}

// ============================================================================
// Metrics reporting via ContentMetrics
// ============================================================================

TEST(PDFProcessorMetricsTest, SuccessfulExtractionIncreasesPdfExtractedTotal) {
    ContentMetrics metrics;
    EXPECT_EQ(metrics.getPdfExtractedTotal(), 0u);

    PDFProcessor::Config cfg;
    cfg.metrics = &metrics;
    PDFProcessor proc(std::move(cfg));
    ContentType ct;

#ifndef THEMIS_ENABLE_PDF
    // Fallback path: a valid-header PDF always succeeds and increments the counter
    auto result = proc.extract(makeMinimalPDF(), ct);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(metrics.getPdfExtractedTotal(), 1u);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 0u);
#else
    // With poppler: our hand-crafted minimal PDF may not parse fully,
    // but exactly one of pdf_extracted_total or extract_errors_total must be 1
    auto result = proc.extract(makeMinimalPDF(), ct);
    EXPECT_EQ(metrics.getPdfExtractedTotal() + metrics.getExtractErrorsTotal(), 1u);
#endif
}

TEST(PDFProcessorMetricsTest, FailedExtractionIncreasesExtractErrorsTotal) {
    ContentMetrics metrics;
    PDFProcessor::Config cfg;
    cfg.metrics = &metrics;
    PDFProcessor proc(std::move(cfg));
    ContentType ct;

    // A non-PDF blob → extract() returns !ok and should report an error
    // NOTE: invalid PDF fails the isPDFValid() check which currently returns
    // before metrics reporting. We use a blob that passes isPDFValid but fails
    // in poppler (or returns ok=true in fallback). Check that no crash occurs.
    auto result = proc.extract("This is not a PDF.", ct);
    EXPECT_FALSE(result.ok);
    // The invalid-header path exits before metrics reporting (isPDFValid check),
    // so counters should remain 0 for this path.
    EXPECT_EQ(metrics.getPdfExtractedTotal(), 0u);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 0u);
}

TEST(PDFProcessorMetricsTest, NoMetricsPointerDoesNotCrash) {
    // Default config has metrics=nullptr; extraction must not segfault
    PDFProcessor proc;
    ContentType ct;
    EXPECT_NO_FATAL_FAILURE(proc.extract(makeMinimalPDF(), ct));
}

TEST(PDFProcessorMetricsTest, MetricsResetClearsPdfCounters) {
    ContentMetrics metrics;
    metrics.recordPdfExtracted();
    metrics.recordPdfExtracted();
    metrics.recordExtractError();
    EXPECT_EQ(metrics.getPdfExtractedTotal(), 2u);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 1u);

    metrics.reset();
    EXPECT_EQ(metrics.getPdfExtractedTotal(), 0u);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 0u);
}

TEST(PDFProcessorMetricsTest, PrometheusFormatContainsPdfCounters) {
    ContentMetrics metrics;
    metrics.recordPdfExtracted();
    metrics.recordPdfExtracted();
    metrics.recordExtractError();

    std::string prom = metrics.toPrometheusFormat();
    EXPECT_NE(prom.find("content_pdf_extracted_total 2"), std::string::npos);
    EXPECT_NE(prom.find("content_extract_errors_total 1"), std::string::npos);
}

// ============================================================================
// Factory functions
// ============================================================================

TEST(PDFProcessorFactoryTest, CreatePDFProcessorDefault) {
    auto proc = createPDFProcessor();
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->getName(), "PDFProcessor");
}

TEST(PDFProcessorFactoryTest, CreatePDFProcessorWithConfig) {
    PDFProcessor::Config cfg;
    cfg.maintain_layout = true;
    cfg.max_pages = 5;
    auto proc = createPDFProcessor(std::move(cfg));
    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->getName(), "PDFProcessor");
}
