/**
 * @file test_rag_evaluation_report_exporter.cpp
 * @brief Unit tests for EvaluationReportExporter (per-query JSON / HTML export)
 *
 * Covers:
 *  - JSON output: required top-level keys present
 *  - JSON output: score values serialised correctly
 *  - JSON output: special characters escaped
 *  - JSON output: empty arrays when no claims/improvements/violations
 *  - HTML output: DOCTYPE / html / head / body structure
 *  - HTML output: dimension score labels present
 *  - HTML output: PASSED / FAILED badge
 *  - HTML output: HTML-special characters escaped in query / answer
 *  - HTML output: retrieved documents rendered
 *  - exportJSON: writes parseable JSON to a file
 *  - exportHTML: writes HTML to a file
 *  - exportJSON: returns false for invalid path
 *  - Factory: create() returns non-null
 *  - Round-trip: score values survive JSON serialisation within epsilon
 */

#include "rag/evaluation_report_exporter.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

using namespace themis::rag::judge;

// ============================================================================
// Helpers
// ============================================================================

static EvaluationResult makeResult(double overall  = 0.85,
                                   double faith    = 0.90,
                                   double rel      = 0.80,
                                   double comp     = 0.75,
                                   double coh      = 0.85,
                                   double ethical  = 0.95,
                                   bool   passed   = true)
{
    EvaluationResult r{};
    r.faithfulness_score       = faith;
    r.relevance_score          = rel;
    r.completeness_score       = comp;
    r.coherence_score          = coh;
    r.ethical_compliance_score = ethical;
    r.overall_score            = overall;

    r.explanation              = "Good answer overall.";
    r.verified_claims          = {"Paris is the capital of France.",
                                  "The Eiffel Tower was built in 1889."};
    r.unverified_claims        = {"France has 70 million people."};
    r.improvements             = {"Add population citation."};

    r.ethical_violations       = {};
    r.respects_human_autonomy  = true;
    r.shows_moral_diversity    = false;
    r.has_ethical_citations    = true;

    r.passed_quality_threshold = passed;
    r.confidence               = 0.92;
    r.evaluation_time          = std::chrono::milliseconds(123);
    r.judge_model              = "gpt-4";

    return r;
}

static EvaluationInput makeInput() {
    EvaluationInput inp;
    inp.query            = "What is the capital of France?";
    inp.generated_answer = "Paris is the capital of France.";

    RetrievedDocument doc1;
    doc1.id               = "doc1";
    doc1.content          = "Paris is the capital of France.";
    doc1.similarity_score = 0.97;

    RetrievedDocument doc2;
    doc2.id               = "doc2";
    doc2.content          = "The Eiffel Tower is in Paris.";
    doc2.similarity_score = 0.82;

    inp.documents = {doc1, doc2};
    inp.metadata  = {{"source", "test"}, {"version", "1.0"}};
    return inp;
}

static PerQueryReport makeReport(const std::string& id = "test-01") {
    PerQueryReport r;
    r.input     = makeInput();
    r.result    = makeResult();
    r.report_id = id;
    return r;
}

// ============================================================================
// JSON output tests
// ============================================================================

TEST(EvaluationReportExporterJSONTest, HasRequiredTopLevelKeys) {
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(makeReport());

    EXPECT_NE(json.find("\"report_id\""),        std::string::npos);
    EXPECT_NE(json.find("\"timestamp_ms\""),      std::string::npos);
    EXPECT_NE(json.find("\"query\""),             std::string::npos);
    EXPECT_NE(json.find("\"generated_answer\""),  std::string::npos);
    EXPECT_NE(json.find("\"documents\""),         std::string::npos);
    EXPECT_NE(json.find("\"metadata\""),          std::string::npos);
    EXPECT_NE(json.find("\"scores\""),            std::string::npos);
    EXPECT_NE(json.find("\"quality\""),           std::string::npos);
    EXPECT_NE(json.find("\"verified_claims\""),   std::string::npos);
    EXPECT_NE(json.find("\"unverified_claims\""), std::string::npos);
    EXPECT_NE(json.find("\"improvements\""),      std::string::npos);
    EXPECT_NE(json.find("\"ethical\""),           std::string::npos);
    EXPECT_NE(json.find("\"explanation\""),       std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, ScoreValuesPresent) {
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(makeReport());

    EXPECT_NE(json.find("\"faithfulness\""),       std::string::npos);
    EXPECT_NE(json.find("\"relevance\""),          std::string::npos);
    EXPECT_NE(json.find("\"completeness\""),       std::string::npos);
    EXPECT_NE(json.find("\"coherence\""),          std::string::npos);
    EXPECT_NE(json.find("\"ethical_compliance\""), std::string::npos);
    EXPECT_NE(json.find("\"overall\""),            std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, ReportIdInOutput) {
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(makeReport("my-run-99"));
    EXPECT_NE(json.find("my-run-99"), std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, SpecialCharactersEscaped) {
    PerQueryReport r = makeReport();
    r.input.query            = "What is \"France\"?";
    r.result.explanation     = "Line1\nLine2\tTabbed";

    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(r);

    // Double-quote characters must be escaped as \"
    EXPECT_NE(json.find("\\\"France\\\""), std::string::npos);
    // Newline and tab inside strings must be escaped
    EXPECT_NE(json.find("\\n"),            std::string::npos);
    EXPECT_NE(json.find("\\t"),            std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, EmptyClaimsProduceEmptyArrays) {
    PerQueryReport r = makeReport();
    r.result.verified_claims   = {};
    r.result.unverified_claims = {};
    r.result.improvements      = {};

    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(r);

    // The serialisation format ("key": [\n  ]) is part of the stable public
    // output contract — callers that parse the JSON rely on well-formed arrays.
    EXPECT_NE(json.find("\"verified_claims\": [\n  ]"), std::string::npos);
    EXPECT_NE(json.find("\"unverified_claims\": [\n  ]"), std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, PassedThresholdTrue) {
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(makeReport());
    EXPECT_NE(json.find("\"passed_threshold\": true"), std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, PassedThresholdFalse) {
    PerQueryReport r = makeReport();
    r.result.passed_quality_threshold = false;
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(r);
    EXPECT_NE(json.find("\"passed_threshold\": false"), std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, EthicalViolationsPresent) {
    PerQueryReport r = makeReport();
    r.result.ethical_violations = {"Patronising language detected.",
                                   "Absolute moral claim without citation."};
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(r);
    EXPECT_NE(json.find("Patronising language detected."), std::string::npos);
    EXPECT_NE(json.find("Absolute moral claim without citation."), std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, DocumentsSerialised) {
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(makeReport());
    EXPECT_NE(json.find("\"doc1\""), std::string::npos);
    EXPECT_NE(json.find("\"doc2\""), std::string::npos);
    EXPECT_NE(json.find("\"similarity_score\""), std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, MetadataSerialised) {
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(makeReport());
    EXPECT_NE(json.find("\"source\""), std::string::npos);
    EXPECT_NE(json.find("\"version\""), std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, ScoreRoundTrip) {
    // Verify that faithfulness score 0.9 appears correctly in the JSON
    PerQueryReport r = makeReport();
    r.result.faithfulness_score = 0.9;
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(r);
    // The value "0.9" should be present (serialised as fixed-precision float)
    EXPECT_NE(json.find("0.9"), std::string::npos);
}

TEST(EvaluationReportExporterJSONTest, OutputNotEmpty) {
    EvaluationReportExporter exporter;
    EXPECT_FALSE(exporter.toJSON(makeReport()).empty());
}

// ============================================================================
// HTML output tests
// ============================================================================

TEST(EvaluationReportExporterHTMLTest, HasDoctype) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_EQ(html.substr(0, 15), "<!DOCTYPE html>");
}

TEST(EvaluationReportExporterHTMLTest, HasHtmlHeadBody) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_NE(html.find("<html"),  std::string::npos);
    EXPECT_NE(html.find("<head>"), std::string::npos);
    EXPECT_NE(html.find("<body>"), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, DimensionLabelsPresent) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_NE(html.find("Faithfulness"),       std::string::npos);
    EXPECT_NE(html.find("Relevance"),          std::string::npos);
    EXPECT_NE(html.find("Completeness"),       std::string::npos);
    EXPECT_NE(html.find("Coherence"),          std::string::npos);
    EXPECT_NE(html.find("Ethical Compliance"), std::string::npos);
    EXPECT_NE(html.find("Overall"),            std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, PassedBadgePresent) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_NE(html.find("PASSED"), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, FailedBadgePresent) {
    PerQueryReport r = makeReport();
    r.result.passed_quality_threshold = false;
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(r);
    EXPECT_NE(html.find("FAILED"), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, QueryAndAnswerPresent) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_NE(html.find("What is the capital of France?"), std::string::npos);
    EXPECT_NE(html.find("Paris is the capital of France."), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, HTMLSpecialCharsEscaped) {
    PerQueryReport r = makeReport();
    r.input.query            = "<script>alert('xss')</script>";
    r.input.generated_answer = "A & B are important.";

    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(r);

    // Raw unescaped form must not appear
    EXPECT_EQ(html.find("<script>"), std::string::npos);
    // Escaped forms must be present
    EXPECT_NE(html.find("&lt;script&gt;"), std::string::npos);
    EXPECT_NE(html.find("A &amp; B"),      std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, ReportIdInTitle) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport("unique-id-42"));
    EXPECT_NE(html.find("unique-id-42"), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, DocumentsRendered) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_NE(html.find("doc1"), std::string::npos);
    EXPECT_NE(html.find("doc2"), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, VerifiedClaimsRendered) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_NE(html.find("Verified Claims"), std::string::npos);
    EXPECT_NE(html.find("The Eiffel Tower was built in 1889."), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, UnverifiedClaimsRendered) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_NE(html.find("Unverified Claims"), std::string::npos);
    EXPECT_NE(html.find("France has 70 million people."), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, ImprovementsRendered) {
    EvaluationReportExporter exporter;
    const std::string html = exporter.toHTML(makeReport());
    EXPECT_NE(html.find("Suggested Improvements"), std::string::npos);
    EXPECT_NE(html.find("Add population citation."), std::string::npos);
}

TEST(EvaluationReportExporterHTMLTest, OutputNotEmpty) {
    EvaluationReportExporter exporter;
    EXPECT_FALSE(exporter.toHTML(makeReport()).empty());
}

// ============================================================================
// File export tests
// ============================================================================

TEST(EvaluationReportExporterFileTest, ExportJSONWritesFile) {
    EvaluationReportExporter exporter;
    const std::string path = "/tmp/test_rag_eval_report.json";
    ASSERT_TRUE(exporter.exportJSON(makeReport(), path));

    std::ifstream ifs(path);
    ASSERT_TRUE(ifs.is_open());
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("\"query\""), std::string::npos);
}

TEST(EvaluationReportExporterFileTest, ExportHTMLWritesFile) {
    EvaluationReportExporter exporter;
    const std::string path = "/tmp/test_rag_eval_report.html";
    ASSERT_TRUE(exporter.exportHTML(makeReport(), path));

    std::ifstream ifs(path);
    ASSERT_TRUE(ifs.is_open());
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("<!DOCTYPE html>"), std::string::npos);
}

TEST(EvaluationReportExporterFileTest, ExportJSONReturnsFalseOnBadPath) {
    EvaluationReportExporter exporter;
    EXPECT_FALSE(exporter.exportJSON(makeReport(),
                                     "/nonexistent_dir_xyz/report.json"));
}

TEST(EvaluationReportExporterFileTest, ExportHTMLReturnsFalseOnBadPath) {
    EvaluationReportExporter exporter;
    EXPECT_FALSE(exporter.exportHTML(makeReport(),
                                     "/nonexistent_dir_xyz/report.html"));
}

// ============================================================================
// Factory test
// ============================================================================

TEST(EvaluationReportExporterFactoryTest, CreateReturnsNonNull) {
    auto exporter = EvaluationReportExporterFactory::create();
    ASSERT_NE(exporter, nullptr);
    // Smoke-test: the created exporter should produce valid output
    EXPECT_FALSE(exporter->toJSON(makeReport()).empty());
    EXPECT_FALSE(exporter->toHTML(makeReport()).empty());
}

// ============================================================================
// Edge-case tests
// ============================================================================

TEST(EvaluationReportExporterEdgeCaseTest, EmptyQueryAndAnswer) {
    PerQueryReport r = makeReport();
    r.input.query            = "";
    r.input.generated_answer = "";
    EvaluationReportExporter exporter;
    // Must not throw; output should still be valid structures
    EXPECT_FALSE(exporter.toJSON(r).empty());
    EXPECT_FALSE(exporter.toHTML(r).empty());
}

TEST(EvaluationReportExporterEdgeCaseTest, NoDocuments) {
    PerQueryReport r = makeReport();
    r.input.documents.clear();
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(r);
    EXPECT_NE(json.find("\"documents\": [\n  ]"), std::string::npos);
}

TEST(EvaluationReportExporterEdgeCaseTest, ZeroScores) {
    PerQueryReport r         = makeReport();
    r.result.faithfulness_score       = 0.0;
    r.result.overall_score            = 0.0;
    r.result.passed_quality_threshold = false;
    EvaluationReportExporter exporter;
    EXPECT_FALSE(exporter.toJSON(r).empty());
    const std::string html = exporter.toHTML(r);
    EXPECT_NE(html.find("FAILED"), std::string::npos);
}

TEST(EvaluationReportExporterEdgeCaseTest, EmptyReportId) {
    PerQueryReport r = makeReport("");
    EvaluationReportExporter exporter;
    const std::string json = exporter.toJSON(r);
    EXPECT_NE(json.find("\"report_id\": \"\""), std::string::npos);
}
