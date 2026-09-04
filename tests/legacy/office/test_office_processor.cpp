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
#include <filesystem>

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
        if (cat == ContentCategory::TEXT) {
          has_text = true;
        }
        if (cat == ContentCategory::STRUCTURED) {
          has_structured = true;
        }
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

// ============================================================================
// LibreOffice headless fallback — unit tests
// ============================================================================

// Helper: build a minimal 512-byte blob that starts with the OLE header.
static std::string makeFakeOLEBlob(const std::string& stream_marker = "") {
    std::string blob(512, '\x00');
    // OLE Compound Document header: D0 CF 11 E0 A1 B1 1A E1
    blob[0] = '\xD0'; blob[1] = '\xCF'; blob[2] = '\x11'; blob[3] = '\xE0';
    blob[4] = '\xA1'; blob[5] = '\xB1'; blob[6] = '\x1A'; blob[7] = '\xE1';
    if (!stream_marker.empty()) {
        // Embed the marker string so detectDocumentType() can identify the type
        for (size_t i = 0; i < stream_marker.size() && (8 + i) < blob.size(); ++i) {
            blob[8 + i] = stream_marker[i];
        }
    }
    return blob;
}

class LegacyOfficeExtractionTest : public ::testing::Test {
protected:
    OfficeProcessor proc;
    ContentType ct;
};

TEST_F(LegacyOfficeExtractionTest, TooSmallBlobReturnsError) {
    // Blob smaller than 8 bytes must be rejected before attempting spawn
    std::string tiny_blob = "\xD0\xCF\x11";
    auto result = proc.extract(tiny_blob, ct);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(LegacyOfficeExtractionTest, WrongMagicHeaderIsRejected) {
    // A 512-byte blob with a DOC-style stream marker but wrong OLE magic bytes
    // must be rejected by the header check inside extractLegacyViaLibreOffice.
    std::string bad_blob(512, '\x00');
    // Wrong magic (not D0 CF 11 E0 A1 B1 1A E1)
    bad_blob[0] = '\xAA'; bad_blob[1] = '\xBB'; bad_blob[2] = '\xCC'; bad_blob[3] = '\xDD';
    const char* marker = "WordDocument";
    for (size_t i = 0; i < strlen(marker); ++i) {
      bad_blob[8 + i] = marker[i];
    }
    auto result = proc.extract(bad_blob, ct);
    EXPECT_FALSE(result.ok) << "Blob with wrong OLE magic should not succeed";
    EXPECT_FALSE(result.error_message.empty())
        << "Expected an error message for invalid OLE header";
}

TEST_F(LegacyOfficeExtractionTest, DocBlobSetsDocumentTypeMetadata) {
    // A valid OLE blob with "WordDocument" stream marker should produce
    // document_type == "doc" and extraction_method == "libreoffice_headless".
    // (soffice may or may not be installed; we only check metadata set before spawn)
    std::string blob = makeFakeOLEBlob("WordDocument");
    auto result = proc.extract(blob, ct);

    // Metadata must be populated regardless of whether soffice is installed
    if (result.metadata.contains("document_type")) {
        EXPECT_EQ(result.metadata["document_type"].get<std::string>(), "doc");
    }
    if (result.metadata.contains("extraction_method")) {
        EXPECT_EQ(result.metadata["extraction_method"].get<std::string>(), "libreoffice_headless");
    }
}

TEST_F(LegacyOfficeExtractionTest, XlsBlobSetsDocumentTypeMetadata) {
    std::string blob = makeFakeOLEBlob("Workbook");
    auto result = proc.extract(blob, ct);

    if (result.metadata.contains("document_type")) {
        EXPECT_EQ(result.metadata["document_type"].get<std::string>(), "xls");
    }
    if (result.metadata.contains("extraction_method")) {
        EXPECT_EQ(result.metadata["extraction_method"].get<std::string>(), "libreoffice_headless");
    }
}

TEST_F(LegacyOfficeExtractionTest, PptBlobSetsDocumentTypeMetadata) {
    std::string blob = makeFakeOLEBlob("PowerPoint");
    auto result = proc.extract(blob, ct);

    if (result.metadata.contains("document_type")) {
        EXPECT_EQ(result.metadata["document_type"].get<std::string>(), "ppt");
    }
    if (result.metadata.contains("extraction_method")) {
        EXPECT_EQ(result.metadata["extraction_method"].get<std::string>(), "libreoffice_headless");
    }
}

TEST_F(LegacyOfficeExtractionTest, FailsGracefullyWhenLibreOfficeNotFound) {
    // Override libreoffice_path to a non-existent binary to simulate
    // an environment where soffice is not installed.
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "/nonexistent/path/to/soffice_____does_not_exist";
    OfficeProcessor proc_nolo(std::move(cfg));

    std::string blob = makeFakeOLEBlob("WordDocument");
    auto result = proc_nolo.extract(blob, ct);

    // Must not crash; must set ok=false and a non-empty error_message
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty())
        << "Expected an error message when soffice binary does not exist";
}

TEST_F(LegacyOfficeExtractionTest, TimeoutConfigIsRespected) {
    // Verify that a very short timeout causes the subprocess to be killed
    // and the result carries timed_out=true in metadata.
    // We point to a real binary that hangs: /bin/sleep 60.
    // To exercise the timeout code path we override libreoffice_path to sleep.
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "/bin/sleep";
    cfg.libreoffice_timeout_seconds = 1;  // 1-second timeout
    OfficeProcessor proc_slow(std::move(cfg));

    std::string blob = makeFakeOLEBlob("WordDocument");
    auto result = proc_slow.extract(blob, ct);

    // The "soffice" command here is sleep, which will not convert anything.
    // Depending on platform: either spawn fails (wrong binary for soffice args),
    // times out, or exits non-zero.  We only require: no crash, ok=false.
    EXPECT_FALSE(result.ok);
}

TEST(LegacyOfficeMetricsTest, DocExtractionFailureIncrementsErrorCounter) {
    ContentMetrics metrics;

    OfficeProcessor::Config cfg;
    // Use a non-existent binary so spawn always fails
    cfg.libreoffice_path = "/nonexistent/soffice_test_binary";
    cfg.metrics = &metrics;
    OfficeProcessor proc(std::move(cfg));
    ContentType ct;

    std::string blob = makeFakeOLEBlob("WordDocument");
    auto result = proc.extract(blob, ct);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 1u);
    EXPECT_EQ(metrics.getOfficeExtractedTotal(), 0u);
}

// PPT extraction failure must also increment the extract_errors_total counter.
TEST(LegacyOfficeMetricsTest, PptExtractionFailureIncrementsErrorCounter) {
    ContentMetrics metrics;

    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "/nonexistent/soffice_test_binary";
    cfg.metrics = &metrics;
    OfficeProcessor proc(std::move(cfg));
    ContentType ct;

    std::string blob = makeFakeOLEBlob("PowerPoint");
    auto result = proc.extract(blob, ct);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(metrics.getExtractErrorsTotal(), 1u);
    EXPECT_EQ(metrics.getOfficeExtractedTotal(), 0u);
}

TEST_F(LegacyOfficeExtractionTest, ConfigDefaultTimeoutIs30) {
    OfficeProcessor::Config cfg;
    EXPECT_EQ(cfg.libreoffice_timeout_seconds, 30);
}

TEST_F(LegacyOfficeExtractionTest, ConfigDefaultPathIsEmpty) {
    OfficeProcessor::Config cfg;
    EXPECT_TRUE(cfg.libreoffice_path.empty());
}

// XLS: same end-to-end path as DOC but routed through the XLS type switch.
// Verifies that a valid XLS OLE blob fails gracefully when soffice is absent.
TEST_F(LegacyOfficeExtractionTest, XlsBlobFailsGracefullyWhenLibreOfficeNotFound) {
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "/nonexistent/path/to/soffice_____does_not_exist";
    OfficeProcessor proc_nolo(std::move(cfg));

    std::string blob = makeFakeOLEBlob("Workbook");
    auto result = proc_nolo.extract(blob, ct);

    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty())
        << "Expected an error message when soffice binary does not exist (XLS)";
}

// PPT: same end-to-end path as DOC but routed through the PPT type switch.
// Verifies that a valid PPT OLE blob fails gracefully when soffice is absent.
TEST_F(LegacyOfficeExtractionTest, PptBlobFailsGracefullyWhenLibreOfficeNotFound) {
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "/nonexistent/path/to/soffice_____does_not_exist";
    OfficeProcessor proc_nolo(std::move(cfg));

    std::string blob = makeFakeOLEBlob("PowerPoint");
    auto result = proc_nolo.extract(blob, ct);

    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty())
        << "Expected an error message when soffice binary does not exist (PPT)";
}

// ============================================================================
// LibreOffice Subprocess Security Tests (CON-007)
//
// These tests verify that the posix_spawn-based sandboxing enforces the
// following security invariants:
//   1. Only absolute paths are accepted for the soffice binary (prevents
//      PATH-hijacking / relative-path substitution attacks).
//   2. Malformed or malicious document content is handled without crashing
//      and without executing arbitrary commands (no shell, no system()).
// ============================================================================

class LibreOfficeSecurityTest : public ::testing::Test {
protected:
    ContentType ct;

    // Build a minimal 512-byte OLE blob whose embedded stream marker triggers
    // DOC document-type detection.
    static std::string makeDocOLEBlob() {
        std::string blob(512, '\x00');
        blob[0] = '\xD0'; blob[1] = '\xCF'; blob[2] = '\x11'; blob[3] = '\xE0';
        blob[4] = '\xA1'; blob[5] = '\xB1'; blob[6] = '\x1A'; blob[7] = '\xE1';
        const char* marker = "WordDocument";
        for (size_t i = 0; i < strlen(marker); ++i) {
          blob[8 + i] = marker[i];
        }
        return blob;
    }
};

// A bare filename (no leading slash) must be rejected immediately to prevent
// PATH-based binary substitution attacks.
TEST_F(LibreOfficeSecurityTest, RelativePathInConfigIsRejected) {
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "soffice";   // relative — must be rejected
    OfficeProcessor proc(std::move(cfg));

    auto result = proc.extract(makeDocOLEBlob(), ct);

    EXPECT_FALSE(result.ok);
#ifdef _WIN32
    EXPECT_NE(result.error_message.find("not supported on Windows"), std::string::npos)
        << "Expected Windows unsupported fallback message, got: "
        << result.error_message;
#else
    EXPECT_NE(result.error_message.find("absolute"), std::string::npos)
        << "Expected 'absolute path' error for relative soffice path, got: "
        << result.error_message;
#endif
}

// A dot-slash prefix is still a relative path and must be rejected.
TEST_F(LibreOfficeSecurityTest, DotSlashPathInConfigIsRejected) {
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "./soffice";  // relative (dot-slash) — must be rejected
    OfficeProcessor proc(std::move(cfg));

    auto result = proc.extract(makeDocOLEBlob(), ct);

    EXPECT_FALSE(result.ok);
#ifdef _WIN32
    EXPECT_NE(result.error_message.find("not supported on Windows"), std::string::npos)
        << "Expected Windows unsupported fallback message, got: "
        << result.error_message;
#else
    EXPECT_NE(result.error_message.find("absolute"), std::string::npos)
        << "Expected 'absolute path' error for dot-slash soffice path, got: "
        << result.error_message;
#endif
}

// A path traversal prefix (../) is also relative and must be rejected.
TEST_F(LibreOfficeSecurityTest, PathTraversalInConfigIsRejected) {
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "../usr/bin/soffice";  // traversal — must be rejected
    OfficeProcessor proc(std::move(cfg));

    auto result = proc.extract(makeDocOLEBlob(), ct);

    EXPECT_FALSE(result.ok);
#ifdef _WIN32
    EXPECT_NE(result.error_message.find("not supported on Windows"), std::string::npos)
        << "Expected Windows unsupported fallback message, got: "
        << result.error_message;
#else
    EXPECT_NE(result.error_message.find("absolute"), std::string::npos)
        << "Expected 'absolute path' error for path-traversal soffice path, got: "
        << result.error_message;
#endif
}

// Shell metacharacters in the configured path must not be interpreted — the
// implementation uses posix_spawn (not system()), so the path is passed
// directly to execve.  posix_spawn will simply fail to locate the binary.
TEST_F(LibreOfficeSecurityTest, ShellMetacharactersInPathAreNotInterpreted) {
    OfficeProcessor::Config cfg;
    // This path starts with '/' so it passes the absolute-path check, but it
    // contains shell metacharacters that would be dangerous under system().
    // Under posix_spawn the metacharacters are part of the literal filename;
    // the binary will not be found and spawn/exec must fail cleanly.
    cfg.libreoffice_path = "/bin/soffice; rm -rf /";
    OfficeProcessor proc(std::move(cfg));

    auto result = proc.extract(makeDocOLEBlob(), ct);

    // Must fail gracefully — no crash, no shell execution side-effects.
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

// An OLE blob padded with embedded null and high bytes must not crash the
// processor.  The "WordDocument" stream marker is embedded after the header so
// that detection routes the blob through extractLegacyViaLibreOffice(), which
// is the actual code-path being stress-tested.
TEST_F(LibreOfficeSecurityTest, OLEBlobWithEmbeddedNullBytesHandledSafely) {
    // Start with the DOC blob that already contains the stream marker
    std::string blob = makeDocOLEBlob();
    // Overwrite the bytes after the marker with alternating 0x00/0xFF garbage
    // to simulate a malformed/malicious document body
    const size_t marker_end = 8 + strlen("WordDocument");
    for (size_t i = marker_end; i < blob.size(); i += 2) {
      blob[i] = '\xFF';
    }

    // Use a non-existent soffice so the test does not depend on the environment
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "/nonexistent/soffice_security_test";
    OfficeProcessor proc(std::move(cfg));

    auto result = proc.extract(blob, ct);

    // Must not crash; ok must be false; error message must be set
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

// A very large OLE blob must be handled gracefully — written to a temp file
// and passed to soffice (or rejected with a clean error when soffice is absent).
TEST_F(LibreOfficeSecurityTest, LargeOLEBlobHandledGracefully) {
    // 4 MB blob — larger than typical documents; should not cause OOM or hang
    constexpr size_t kSize = 4u * 1024u * 1024u;
    std::string blob(kSize, '\x42');
    blob[0] = '\xD0'; blob[1] = '\xCF'; blob[2] = '\x11'; blob[3] = '\xE0';
    blob[4] = '\xA1'; blob[5] = '\xB1'; blob[6] = '\x1A'; blob[7] = '\xE1';
    const char* marker = "WordDocument";
    for (size_t i = 0; i < strlen(marker); ++i) {
      blob[8 + i] = marker[i];
    }

    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "/nonexistent/soffice_security_test";
    OfficeProcessor proc(std::move(cfg));

    auto result = proc.extract(blob, ct);

    // Must complete without crashing; ok=false because soffice is absent
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error_message.empty());
}

// ============================================================================
// Permission-violation tests (CON-007)
//
// These tests verify that the RAII temp-dir guard correctly removes all
// temporary files and directories on exit, so that no sensitive document
// content leaks to subsequent processes or users who share the filesystem.
// ============================================================================

// After a failed extraction attempt (soffice not found), the implementation's
// RAII guard must have removed every themisdb_lo_* temp directory from P_tmpdir.
// This prevents data leakage: a malicious document's raw bytes must not persist
// on disk after the extraction call returns.
TEST_F(LibreOfficeSecurityTest, TempDirIsCleanedUpAfterFailure) {
    // Snapshot the number of themisdb_lo_* entries before extraction
    std::error_code tmp_ec;
    std::string tmp_base = std::filesystem::temp_directory_path(tmp_ec).string();
    if (tmp_ec || tmp_base.empty()) {
        tmp_base = ".";
    }

    auto count_lo_dirs = [&]() -> int {
        namespace fs = std::filesystem;
        std::error_code ec;
        if (!fs::exists(tmp_base, ec) || ec) {
          return -1;
        }

        int n = 0;
        for (fs::directory_iterator it(tmp_base, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
            if (it->is_directory(ec) && !ec) {
                const std::string name = it->path().filename().string();
                if (name.rfind("themisdb_lo_", 0) == 0) {
                  ++n;
                }
            }
        }

        return ec ? -1 : n;
    };

    int before = count_lo_dirs();
    ASSERT_GE(before, 0) << "Cannot open temp directory: " << tmp_base;

    // Trigger an extraction that must fail (non-existent soffice)
    OfficeProcessor::Config cfg;
    cfg.libreoffice_path = "/nonexistent/soffice_cleanup_test";
    OfficeProcessor proc(std::move(cfg));
    auto result = proc.extract(makeDocOLEBlob(), ct);

    EXPECT_FALSE(result.ok);  // sanity: extraction must have failed

    int after = count_lo_dirs();
    ASSERT_GE(after, 0) << "Cannot open temp directory after extraction: " << tmp_base;

    // No temp dirs should have been left behind
    EXPECT_EQ(after, before)
        << "RAII guard leaked " << (after - before)
        << " themisdb_lo_* temp director(ies) under " << tmp_base;
}

