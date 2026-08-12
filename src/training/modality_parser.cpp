/**
 * @file modality_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "training/modality_parser.h"
#include "llm/prompt_safety_utils.h"
#include "utils/string_utils.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <utils/string_utils.h>

namespace themis {
namespace training {

// ============================================================================
// Internal helpers
// ============================================================================
namespace detail {

static bool sanitizeTrainingPromptSurface(
    const std::string& input,
    std::string& sanitized,
    std::string* blocked_rule,
    std::string* blocked_reason)
{
    return llm::prompt_safety::sanitizePromptWithSharedPolicy(
        input,
        sanitized,
        blocked_rule,
        blocked_reason);
}

// Count occurrences of a character in a string view window
static size_t countChar(const std::string& s, char c) noexcept {
    size_t n = 0;
    for (char ch : s) {
        if (ch == c) ++n;
    }
    return n;
}

// Split a string by a delimiter character
static std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(std::move(line));
    }
    return lines;
}

// Trim leading/trailing ASCII whitespace
// Using themis::utils::trim() from string_utils.h (Phase 1 consolidation)

// Return true if the line looks like part of a pipe-delimited table row
static bool isPipeTableRow(const std::string& line) {
    std::string t = themis::utils::trim(line);
    if (t.empty()) return false;
    return (t.front() == '|' || t.back() == '|') && countChar(t, '|') >= 2;
}

// Return true if the line consists primarily of dashes (table separator)
static bool isTableSeparator(const std::string& line) {
    std::string t = themis::utils::trim(line);
    if (t.empty()) return false;
    for (char c : t) {
        if (c != '-' && c != '|' && c != ' ' && c != '+') return false;
    }
    return countChar(t, '-') >= 3;
}

// Check whether a line looks like a whitespace-aligned table line:
// multiple consecutive-space runs of ≥3 characters separating words
static bool isAlignedTableRow(const std::string& line) {
    const std::string& t = line;
    if (t.size() < 10) return false;
    size_t space_runs = 0;
    bool in_spaces = false;
    size_t run_len = 0;
    for (char c : t) {
        if (c == ' ') {
            if (!in_spaces) { in_spaces = true; run_len = 0; }
            ++run_len;
        } else {
            if (in_spaces && run_len >= 3) ++space_runs;
            in_spaces = false;
        }
    }
    return space_runs >= 2;
}

// ============================================================================
// German legal citation regex patterns
// ============================================================================

// Matches:
//   §  242 BGB
//   § 14 Abs. 1 GG
//   Art. 14 GG
//   §§ 123, 124 BGB  (plural)
static const std::regex RE_STATUTORY(
    R"((?:(?:§§?|\xC2\xA7(?:\xC2\xA7)?|\xA7\xA7?)\s*\d+(?:\s*Abs\.\s*\d+)?(?:\s+(?:BGB|HGB|StGB|ZPO|GG|VwGO|AO|UStG|InsO|GmbHG|AktG|WpHG|KWG|SGB|UrhG|BRAO|BBodSchG|TKG|TMG|GDPR|DSGVO|MarkenG|PatG|[A-Z]{2,10}))?)|(?:Art\.\s*\d+(?:\s*Abs\.\s*\d+)?\s+[A-Z]{2,10}))",
    std::regex_constants::optimize | std::regex_constants::ECMAScript);

// Matches German court decision citations:
//   BGH, Urt. v. 14.12.2021, II ZR 93/21
//   BVerwG, Beschl. v. 3.5.2022 – 4 B 12/22
//   BAG 14.9.2023 – 2 AZR 345/22
//   OLG München, Urt. v. 10.01.2024 – 7 U 123/23
static const std::regex RE_COURT_DECISION(
    R"((?:BGH|BVerwG|BAG|BSG|BFH|BVerfG|OLG|LG|AG|VG|OVG|VGH|LAG|FG|FGH|LSG|SGG?)\b[,\s]*(?:Urt\.|Beschl\.|Bes\.|Beschluss|Urteil)?[,\s]*(?:v\.|vom)?\s*\d{1,2}\.\d{1,2}\.\d{2,4}[,\s–-]+[A-Z0-9 /]+)",
    std::regex_constants::optimize | std::regex_constants::ECMAScript);

// Matches EU / ECHR citations:
//   EuGH, C-123/21   |  EGMR 12345/20  |  Rs. C-123/21
static const std::regex RE_EU_CITATION(
    R"((?:EuGH|EuG|EGMR|ECtHR)\b[,\s]*(?:Rs\.\s*)?[CT]-?\d+/\d{2,4})",
    std::regex_constants::optimize | std::regex_constants::ECMAScript);

// ============================================================================
// Sentence splitter for German legal text
// ============================================================================

// Split text into sentences using common German legal sentence boundaries.
// Avoids splitting on abbreviations ("Abs.", "Nr.", "Art.", numbers).
static std::vector<std::string> splitSentences(const std::string& text) {
    std::vector<std::string> sentences;
    if (text.empty()) return sentences;

    // Known abbreviations that should NOT terminate a sentence
    static const std::regex RE_ABBREV(
        R"((?:Abs|Nr|Art|Ziff|Rn|Fn|ggf|bzw|vgl|z\.B|i\.d\.F|s\.o|s\.u|v\.a|u\.a|etc|i\.e|e\.g|Dr|Prof|S|S\.)\.\s*$)",
        std::regex_constants::ECMAScript);

    std::string current;
    current.reserve(256);

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        current += c;

        bool is_boundary = false;
        if ((c == '.' || c == '!' || c == '?') && i + 1 < text.size()) {
            char next = text[i + 1];
            // Sentence ends when followed by whitespace and uppercase or digit
            if ((next == ' ' || next == '\n') &&
                i + 2 < text.size() &&
                (std::isupper(static_cast<unsigned char>(text[i + 2])) ||
                 c == '!' || c == '?'))
            {
                // Don't split on known abbreviations
                std::string tail = themis::utils::trim(current);
                if (!std::regex_search(tail, RE_ABBREV)) {
                    is_boundary = true;
                }
            }
        }
        if (c == '\n' && i + 1 < text.size() && text[i + 1] == '\n') {
            is_boundary = true;
        }

        if (is_boundary) {
            std::string s = themis::utils::trim(current);
            if (!s.empty()) sentences.push_back(std::move(s));
            current.clear();
        }
    }
    std::string tail = themis::utils::trim(current);
    if (!tail.empty()) sentences.push_back(std::move(tail));
    return sentences;
}

// ============================================================================
// Table detector: identify contiguous table blocks in a document
// ============================================================================

struct TableBlock {
    size_t first_line;   ///< First line index (inclusive)
    size_t last_line;    ///< Last line index (inclusive)
    std::string content; ///< Raw table text
};

static std::vector<TableBlock> detectTableBlocks(
    const std::vector<std::string>& lines)
{
    std::vector<TableBlock> blocks;
    size_t i = 0;
    while (i < lines.size()) {
        bool is_table = isPipeTableRow(lines[i]) || isAlignedTableRow(lines[i]);
        if (is_table) {
            size_t start = i;
            std::string content;
            while (i < lines.size() &&
                   (isPipeTableRow(lines[i]) ||
                    isAlignedTableRow(lines[i]) ||
                    isTableSeparator(lines[i])))
            {
                content += lines[i] + "\n";
                ++i;
            }
            if (i - start >= 2) { // At least header + one data row
                blocks.push_back({start, i - 1, std::move(content)});
            }
        } else {
            ++i;
        }
    }
    return blocks;
}

} // namespace detail

// ============================================================================
// TextClauseExtractor
// ============================================================================

TextClauseExtractor::TextClauseExtractor(const ModalityParserConfig& config)
    : config_(config) {}

std::vector<TrainingSample>
TextClauseExtractor::extract(const std::string& text,
                             const std::string& document_id) const
{
    std::vector<TrainingSample> samples;
    if (text.empty()) return samples;

    auto lines = detail::splitLines(text);

    // Build a non-table, non-image copy of the text for clause extraction.
    // Skip lines that belong to a detected table block.
    auto table_blocks = detail::detectTableBlocks(lines);
    std::set<size_t> table_lines;
    for (const auto& blk : table_blocks) {
        for (size_t l = blk.first_line; l <= blk.last_line; ++l)
            table_lines.insert(l);
    }

    std::string clean_text;
    clean_text.reserve(text.size());
    for (size_t i = 0; i < lines.size(); ++i) {
        if (table_lines.count(i) == 0) {
            clean_text += lines[i];
            clean_text += '\n';
        }
    }

    auto sentences = detail::splitSentences(clean_text);

    for (const auto& sentence : sentences) {
        if (sentence.size() < config_.text_clause_min_length) continue;

        std::string sanitized_input;
        std::string blocked_rule;
        std::string blocked_reason;
        if (!detail::sanitizeTrainingPromptSurface(sentence,
                                                   sanitized_input,
                                                   &blocked_rule,
                                                   &blocked_reason)) {
            continue;
        }

        TrainingSample s;
        s.input      = std::move(sanitized_input);
        s.output     = "text_clause";
        s.category   = "legal_clause";
        s.confidence = config_.text_clause_base_confidence;
        s.source_id  = document_id;
        s.modality   = ContentModality::TEXT_CLAUSE;
        // Store document context as minimal metadata
        s.metadata   = "{\"modality\":\"text_clause\",\"source\":\"" + document_id + "\"}";
        samples.push_back(std::move(s));
    }
    return samples;
}

// ============================================================================
// TableExtractor
// ============================================================================

TableExtractor::TableExtractor(const ModalityParserConfig& config)
    : config_(config) {}

std::vector<TrainingSample>
TableExtractor::extract(const std::string& text,
                        const std::string& document_id) const
{
    std::vector<TrainingSample> samples;
    if (text.empty()) return samples;

    auto lines       = detail::splitLines(text);
    auto table_blocks = detail::detectTableBlocks(lines);

    size_t count = 0;
    for (const auto& blk : table_blocks) {
        if (count >= config_.max_table_rows) break;

        std::string sanitized_input;
        std::string blocked_rule;
        std::string blocked_reason;
        if (!detail::sanitizeTrainingPromptSurface(blk.content,
                                                   sanitized_input,
                                                   &blocked_rule,
                                                   &blocked_reason)) {
            continue;
        }

        // Build a column-count summary from the first row
        std::string first_row = themis::utils::trim(lines[blk.first_line]);
        size_t col_count = detail::countChar(first_row, '|');
        if (col_count > 0) col_count = (col_count + 1) / 2; // pipe-delimited

        std::string output_label = "table[cols=" + std::to_string(col_count)
                                 + ",rows=" + std::to_string(blk.last_line - blk.first_line + 1)
                                 + "]";

        TrainingSample s;
        s.input      = std::move(sanitized_input);
        s.output     = output_label;
        s.category   = "table";
        s.confidence = config_.table_base_confidence;
        s.source_id  = document_id;
        s.modality   = ContentModality::TABLE;
        s.metadata   = "{\"modality\":\"table\",\"columns\":" + std::to_string(col_count)
                      + ",\"source\":\"" + document_id + "\"}";
        samples.push_back(std::move(s));
        ++count;
    }
    return samples;
}

// ============================================================================
// CitationExtractor
// ============================================================================

CitationExtractor::CitationExtractor(const ModalityParserConfig& config)
    : config_(config) {}

std::vector<TrainingSample>
CitationExtractor::extract(const std::string& text,
                           const std::string& document_id) const
{
    std::vector<TrainingSample> samples;
    if (text.empty()) return samples;

    auto addMatch = [&](const std::string& matched, const std::string& type) {
        if (samples.size() >= config_.max_citations_per_document) return;
        std::string m = themis::utils::trim(matched);
        if (m.empty()) return;

        std::string sanitized_input;
        std::string blocked_rule;
        std::string blocked_reason;
        if (!detail::sanitizeTrainingPromptSurface(m,
                                                   sanitized_input,
                                                   &blocked_rule,
                                                   &blocked_reason)) {
            return;
        }

        TrainingSample s;
        s.input      = std::move(sanitized_input);
        s.output     = type;
        s.category   = "citation";
        s.confidence = config_.citation_base_confidence;
        s.source_id  = document_id;
        s.modality   = ContentModality::CITATION;
        s.metadata   = "{\"modality\":\"citation\",\"type\":\"" + type
                      + "\",\"source\":\"" + document_id + "\"}";
        samples.push_back(std::move(s));
    };

    // Statutory references
    {
        auto begin = std::sregex_iterator(text.begin(), text.end(),
                                          detail::RE_STATUTORY);
        auto end   = std::sregex_iterator();
        for (auto it = begin; it != end && samples.size() < config_.max_citations_per_document; ++it) {
            addMatch((*it)[0].str(), "statutory");
        }
    }

    // Court decision citations
    {
        auto begin = std::sregex_iterator(text.begin(), text.end(),
                                          detail::RE_COURT_DECISION);
        auto end   = std::sregex_iterator();
        for (auto it = begin; it != end && samples.size() < config_.max_citations_per_document; ++it) {
            addMatch((*it)[0].str(), "case_law");
        }
    }

    // EU / ECHR citations
    {
        auto begin = std::sregex_iterator(text.begin(), text.end(),
                                          detail::RE_EU_CITATION);
        auto end   = std::sregex_iterator();
        for (auto it = begin; it != end && samples.size() < config_.max_citations_per_document; ++it) {
            addMatch((*it)[0].str(), "eu_regulation");
        }
    }

    return samples;
}

// ============================================================================
// OCRExtractor
// ============================================================================

OCRExtractor::OCRExtractor(const ModalityParserConfig& config)
    : config_(config)
    , available_(false)
{
#ifdef THEMIS_ENABLE_OCR
    // Runtime availability check: attempt to locate Tesseract data
    // (production: call TessBaseAPI::Init and check return code)
    available_ = config_.enable_ocr;
#endif
    // suppress unused-variable warning when OCR is disabled
}

bool OCRExtractor::isAvailable() const noexcept {
    return available_;
}

std::vector<TrainingSample>
OCRExtractor::extract([[maybe_unused]] const std::string& image_path,
                      [[maybe_unused]] const std::string& document_id) const
{
    std::vector<TrainingSample> samples;

#ifdef THEMIS_ENABLE_OCR
    if (!available_ || image_path.empty()) return samples;

    // Production: integrate Tesseract TessBaseAPI here.
    //   TessBaseAPI api;
    //   api.Init(nullptr, "deu");
    //   Pix* pix = pixRead(image_path.c_str());
    //   api.SetImage(pix);
    //   std::string ocr_text = api.GetUTF8Text();
    //   pixDestroy(&pix);
    //   api.End();
    //
    // For now emit one placeholder sample so the pipeline can account for
    // OCR-sourced samples in provenance records.
    std::string sanitized_input;
    std::string blocked_rule;
    std::string blocked_reason;
    if (!detail::sanitizeTrainingPromptSurface(image_path,
                                               sanitized_input,
                                               &blocked_rule,
                                               &blocked_reason)) {
        return samples;
    }

    TrainingSample s;
    s.input      = std::move(sanitized_input); // real: replaced by OCR text
    s.output     = "ocr_image";
    s.category   = "ocr";
    s.confidence = config_.ocr_base_confidence;
    s.source_id  = document_id;
    s.modality   = ContentModality::OCR_IMAGE;
    s.metadata   = "{\"modality\":\"ocr\",\"image_path\":\"" + image_path
                  + "\",\"source\":\"" + document_id + "\"}";
    samples.push_back(std::move(s));
#else
#endif

    return samples;
}

// ============================================================================
// ModalityDetector::Impl
// ============================================================================

/** @brief ModalityDetector::Impl. */
class ModalityDetector::Impl {
public:
    explicit Impl(const ModalityParserConfig& config)
        : config_(config)
        , text_extractor_(config)
        , table_extractor_(config)
        , citation_extractor_(config)
        , ocr_extractor_(config)
    {}

    // -------------------------------------------------------------------------
    ContentModality detectModality(const std::string& content,
                                   const std::string& mime_hint) const
    {
        // 1. MIME-hint takes precedence for image types
        if (!mime_hint.empty() &&
            mime_hint.find("image/") == 0)
        {
            return ContentModality::OCR_IMAGE;
        }

        if (content.empty()) return ContentModality::UNKNOWN;

        // 2. Table density heuristic
        auto lines = detail::splitLines(content);
        size_t table_lines = 0;
        for (const auto& l : lines) {
            if (detail::isPipeTableRow(l) || detail::isAlignedTableRow(l))
                ++table_lines;
        }
        double table_ratio = lines.empty() ? 0.0
                           : static_cast<double>(table_lines) / lines.size();
        if (table_ratio >= 0.30) return ContentModality::TABLE;

        // 3. Citation density heuristic
        auto stat_begin = std::sregex_iterator(content.begin(), content.end(),
                                               detail::RE_STATUTORY);
        auto court_begin = std::sregex_iterator(content.begin(), content.end(),
                                                detail::RE_COURT_DECISION);
        size_t citation_count = std::distance(stat_begin,  std::sregex_iterator{})
                              + std::distance(court_begin, std::sregex_iterator{});

        // More than 1 citation per 500 characters → treat as CITATION document
        double citation_density = content.size() > 0
                                ? static_cast<double>(citation_count) / (content.size() / 500.0)
                                : 0.0;
        if (citation_density >= 1.0) return ContentModality::CITATION;

        // 4. Default: plain text clause
        return ContentModality::TEXT_CLAUSE;
    }

    // -------------------------------------------------------------------------
    ModalityParseResult parseDocument(const std::string& content,
                                      const std::string& document_id,
                                      const std::string& mime_hint) const
    {
        ModalityParseResult result;
        result.document_id = document_id;

        auto t0 = std::chrono::steady_clock::now();

        try {
            // Handle image-only documents first
            const bool is_image = !mime_hint.empty() &&
                                  mime_hint.find("image/") == 0;
            if (is_image) {
                if (ocr_extractor_.isAvailable()) {
                    auto ocr = ocr_extractor_.extract(content, document_id);
                    result.stats.ocr_pages_processed += ocr.size();
                    for (auto& s : ocr) result.samples.push_back(std::move(s));
                }
            } else {
                // Text-clause extraction
                auto clauses = text_extractor_.extract(content, document_id);
                result.stats.text_clauses_extracted += clauses.size();
                for (auto& s : clauses) result.samples.push_back(std::move(s));

                // Table extraction
                auto tables = table_extractor_.extract(content, document_id);
                result.stats.tables_extracted += tables.size();
                for (auto& s : tables) result.samples.push_back(std::move(s));

                // Citation extraction
                auto citations = citation_extractor_.extract(content, document_id);
                result.stats.citations_extracted += citations.size();
                for (auto& s : citations) result.samples.push_back(std::move(s));
            }

            result.stats.documents_processed = 1;
            result.stats.samples_total       = result.samples.size();
            auto t1 = std::chrono::steady_clock::now();
            result.stats.elapsed_seconds =
                std::chrono::duration<double>(t1 - t0).count();
            result.success = true;

        } catch (const std::exception& ex) {
            result.success       = false;
            result.error_message = ex.what();
        }

        return result;
    }

    // -------------------------------------------------------------------------
    ModalityParseStats parseBatch(
        const std::vector<std::pair<std::string, std::string>>& documents,
        std::vector<TrainingSample>& out_samples) const
    {
        ModalityParseStats total;
        auto t0 = std::chrono::steady_clock::now();

        for (const auto& [content, doc_id] : documents) {
            auto res = parseDocument(content, doc_id, "");
            total.documents_processed     += res.stats.documents_processed;
            total.text_clauses_extracted  += res.stats.text_clauses_extracted;
            total.tables_extracted        += res.stats.tables_extracted;
            total.citations_extracted     += res.stats.citations_extracted;
            total.ocr_pages_processed     += res.stats.ocr_pages_processed;
            total.samples_total           += res.stats.samples_total;
            for (auto& s : res.samples) out_samples.push_back(std::move(s));
        }

        auto t1 = std::chrono::steady_clock::now();
        total.elapsed_seconds = std::chrono::duration<double>(t1 - t0).count();
        return total;
    }

private:
    ModalityParserConfig    config_;
    TextClauseExtractor     text_extractor_;
    TableExtractor          table_extractor_;
    CitationExtractor       citation_extractor_;
    OCRExtractor            ocr_extractor_;
};

// ============================================================================
// ModalityDetector – public API (delegates to Impl)
// ============================================================================

ModalityDetector::ModalityDetector(const ModalityParserConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

ModalityDetector::~ModalityDetector() = default;

ContentModality ModalityDetector::detectModality(const std::string& content,
                                                  const std::string& mime_hint) const
{
    return impl_->detectModality(content, mime_hint);
}

ModalityParseResult ModalityDetector::parseDocument(const std::string& content,
                                                     const std::string& document_id,
                                                     const std::string& mime_hint) const
{
    return impl_->parseDocument(content, document_id, mime_hint);
}

ModalityParseStats ModalityDetector::parseBatch(
    const std::vector<std::pair<std::string, std::string>>& documents,
    std::vector<TrainingSample>& out_samples) const
{
    return impl_->parseBatch(documents, out_samples);
}

} // namespace training
} // namespace themis
