/**
 * @file modality_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "training/auto_labeler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace training {

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Configuration for all modality parser components.
 */
struct ModalityParserConfig {
    /// ISO-639-1 language code of the document corpus ("de", "en", …)
    std::string language_code = "de";

    /// Minimum character length to accept a text clause as a training sample.
    size_t text_clause_min_length = 20;

    /// Maximum table rows to extract per table block (prevents runaway memory).
    size_t max_table_rows = 500;

    /// Maximum citations to extract per document.
    size_t max_citations_per_document = 200;

    /// Confidence score assigned to TEXT_CLAUSE samples by default.
    float text_clause_base_confidence = 0.80f;

    /// Confidence score assigned to TABLE samples by default.
    float table_base_confidence = 0.75f;

    /// Confidence score assigned to CITATION samples by default.
    float citation_base_confidence = 0.85f;

    /// Confidence score assigned to OCR_IMAGE samples by default.
    float ocr_base_confidence = 0.60f;

    /// Enable OCR extraction.  Requires THEMIS_ENABLE_OCR and a Tesseract
    /// installation at runtime; silently disabled otherwise.
    bool enable_ocr = false;

    ModalityParserConfig() = default;
};

// ============================================================================
// Result types
// ============================================================================

/**
 * @brief Per-modality extraction statistics for a single document or batch.
 */
struct ModalityParseStats {
    size_t documents_processed     = 0; ///< Documents fed to the parser
    size_t text_clauses_extracted  = 0; ///< TEXT_CLAUSE samples produced
    size_t tables_extracted        = 0; ///< TABLE samples produced
    size_t citations_extracted     = 0; ///< CITATION samples produced
    size_t ocr_pages_processed     = 0; ///< OCR_IMAGE samples produced
    size_t samples_total           = 0; ///< Sum of all extracted samples
    double elapsed_seconds         = 0.0;

    ModalityParseStats() = default;
};

/**
 * @brief Extraction result for a single document.
 */
struct ModalityParseResult {
    std::string document_id;
    std::vector<TrainingSample> samples; ///< All extracted samples (all modalities)
    ModalityParseStats stats;
    bool success = false;
    std::string error_message;

    ModalityParseResult() = default;
};

// ============================================================================
// Per-modality extractors
// ============================================================================

/**
 * @brief Extracts plain-text legal clauses from a document.
 *
 * Splits content at sentence and paragraph boundaries, filters tokens that
 * are clearly tabular or citation-like, and assigns TEXT_CLAUSE modality.
 * Each resulting sample represents a single legal clause or sentence that
 * meets the minimum length threshold.
 */
class TextClauseExtractor {
public:
    explicit TextClauseExtractor(const ModalityParserConfig& config);

    /**
     * @brief Extract text clauses from @p text.
     * @param text        Raw document text.
     * @param document_id Identifier propagated to sample.source_id.
     * @return Vector of TEXT_CLAUSE-typed TrainingSample records.
     *
     * Applies shared prompt-safety policy per extracted clause. Clauses matching
     * blocked prompt-injection patterns are dropped (fail-closed). Allowed clauses
     * are emitted with control-token redaction applied.
     */
    std::vector<TrainingSample> extract(const std::string& text,
                                        const std::string& document_id) const;

private:
    ModalityParserConfig config_;
};

// ----------------------------------------------------------------------------

/**
 * @brief Extracts structured tables from legal documents.
 *
 * Recognises pipe-delimited Markdown-style tables and whitespace-aligned
 * column grids (common in German court decisions and contracts).  Each
 * detected table block becomes one TABLE-modality TrainingSample whose
 * @c input is the raw table text and @c output is a JSON-like column summary.
 */
class TableExtractor {
public:
    explicit TableExtractor(const ModalityParserConfig& config);

    /**
     * @brief Extract table blocks from @p text.
     * @param text        Raw document text.
     * @param document_id Identifier propagated to sample.source_id.
     * @return Vector of TABLE-typed TrainingSample records.
     *
     * Applies shared prompt-safety policy per extracted table block. Blocks that
     * trigger a prompt-injection block rule are dropped; allowed blocks are emitted
     * with control-token redaction applied.
     */
    std::vector<TrainingSample> extract(const std::string& text,
                                        const std::string& document_id) const;

private:
    ModalityParserConfig config_;
};

// ----------------------------------------------------------------------------

/**
 * @brief Extracts statutory and case-law citations from legal documents.
 *
 * Matches German-law citation patterns:
 *   - Statutory references:  "§ 242 BGB", "Art. 14 Abs. 1 GG"
 *   - Court decisions:       "BGH, Urt. v. 14.12.2021, II ZR 93/21"
 *                             "BVerwG, Beschl. v. 3.5.2022 – 4 B 12/22"
 *   - EU/ECHR citations:     "EuGH, C-123/21", "EGMR 12345/20"
 *
 * Each matched citation becomes a CITATION-modality TrainingSample whose
 * @c input is the citation text and @c output is the inferred citation type
 * (e.g., "statutory", "case_law", "eu_regulation").
 */
class CitationExtractor {
public:
    explicit CitationExtractor(const ModalityParserConfig& config);

    /**
     * @brief Extract legal citations from @p text.
     * @param text        Raw document text.
     * @param document_id Identifier propagated to sample.source_id.
     * @return Vector of CITATION-typed TrainingSample records.
     *
     * Applies shared prompt-safety policy per citation text. Blocked payloads are
     * rejected (fail-closed); allowed payloads are emitted with control-token
     * redaction applied.
     */
    std::vector<TrainingSample> extract(const std::string& text,
                                        const std::string& document_id) const;

private:
    ModalityParserConfig config_;
};

// ----------------------------------------------------------------------------

/**
 * @brief Wraps optional OCR processing for scanned image pages.
 *
 * When THEMIS_ENABLE_OCR is set at build time and a Tesseract installation
 * is available at runtime, this extractor passes image data through OCR
 * and wraps the resulting text as an OCR_IMAGE-modality TrainingSample.
 * If OCR is unavailable the extractor compiles and links cleanly but
 * @c isAvailable() returns @c false and @c extract() returns an empty vector.
 *
 * @note The @p image_path argument is the filesystem path to a TIFF, PNG,
 *       or JPEG image of a scanned page.  Pass the originating document_id
 *       as the second argument so provenance can be traced back.
 */
class OCRExtractor {
public:
    explicit OCRExtractor(const ModalityParserConfig& config);

    /**
     * @brief Returns true when OCR support is compiled in and initialised.
     */
    bool isAvailable() const noexcept;

    /**
     * @brief Run OCR on an image file and return extracted samples.
     *
     * @param image_path  Path to TIFF / PNG / JPEG image.
     * @param document_id Source document identifier for provenance.
     * @return Vector of OCR_IMAGE-typed TrainingSample records, or empty if
     *         OCR is unavailable.
     */
    std::vector<TrainingSample> extract([[maybe_unused]] const std::string& image_path,
                                        [[maybe_unused]] const std::string& document_id) const;

private:
    ModalityParserConfig config_;
    bool available_;
};

// ============================================================================
// ModalityDetector – main orchestrator
// ============================================================================

/**
 * @brief Auto-detecting multi-modality document parser.
 *
 * Inspects document content using layout and pattern heuristics to determine
 * which modalities are present, then dispatches to the appropriate extractor.
 * The detector can also be driven in full-parse mode, which runs all
 * applicable extractors and aggregates their output.
 *
 * Thread-safety: @c ModalityDetector is immutable after construction;
 * @c parseDocument() and @c parseBatch() may be called concurrently.
 */
class ModalityDetector {
public:
    /**
     * @brief Construct the detector with the given configuration.
     * @param config Parser configuration (language, thresholds, OCR flag).
     */
    explicit ModalityDetector(const ModalityParserConfig& config);

    ~ModalityDetector();

    // Non-copyable, movable
    ModalityDetector(const ModalityDetector&)            = delete;
    ModalityDetector& operator=(const ModalityDetector&) = delete;
    ModalityDetector(ModalityDetector&&)                 noexcept = default;
    ModalityDetector& operator=(ModalityDetector&&)      noexcept = default;

    /**
     * @brief Heuristically detect the dominant modality of @p content.
     *
     * Checks, in order:
     *  1. @p mime_hint ("image/..." → OCR_IMAGE if enabled)
     *  2. Table density (pipe characters, aligned columns)
     *  3. Citation density (§, court-decision patterns)
     *  4. Falls back to TEXT_CLAUSE
     *
     * @param content   Raw document content (text or binary).
     * @param mime_hint Optional MIME-type string to guide detection.
     * @return Dominant ContentModality of the content.
     */
    ContentModality detectModality(const std::string& content,
                                   const std::string& mime_hint = "") const;

    /**
     * @brief Extract modality-typed training samples from a single document.
     *
     * Runs all applicable extractors (text, table, citation, OCR) and
     * aggregates results into a single @c ModalityParseResult.  Per-modality
     * extraction statistics are embedded in the result.
     *
     * @param content     Raw document text (or image path when mime_hint is
     *                    "image/tiff" etc. for OCR-only documents.
     * @param document_id Source document identifier for provenance tracing.
     * @param mime_hint   Optional MIME-type hint; pass "image/tiff" etc. for
     *                    OCR-only documents.
     * @return Parse result with extracted samples and statistics.
     */
    ModalityParseResult parseDocument(const std::string& content,
                                      const std::string& document_id,
                                      const std::string& mime_hint = "") const;

    /**
     * @brief Extract training samples from a batch of documents.
     *
     * Calls @c parseDocument() for each entry in @p documents and appends
     * all produced samples to @p out_samples.
     *
     * @param documents   Pairs of (content, document_id).
     * @param out_samples Output vector; samples are appended (not replaced).
     * @return Aggregated statistics across the entire batch.
     */
    ModalityParseStats parseBatch(
        const std::vector<std::pair<std::string, std::string>>& documents,
        std::vector<TrainingSample>& out_samples) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
