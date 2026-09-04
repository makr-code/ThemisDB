/**
 * @file document_summarizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=7, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/document_summarizer.h"
#include "rag/llm_integration.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace themis::rag {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Tokenise @p text into lower-cased words, stripping punctuation.
std::vector<std::string> tokeniseWords(const std::string& text) {
    std::vector<std::string> tokens;
    std::string cur = {};
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            cur += static_cast<char>(std::tolower(ch));
        } else if (!cur.empty()) {
            tokens.push_back(std::move(cur));
        }
    }
    if (!cur.empty()) {
        tokens.push_back(std::move(cur));
    }
    return tokens;
}

/// Compute a relevance score for @p sentence relative to @p query_terms.
/// Score = (number of distinct query terms appearing in the sentence) /
///         (number of distinct query terms).
/// Falls back to sentence length heuristic when the query is empty.
double scoreSentence(const std::string& sentence,
                     const std::unordered_set<std::string>& query_terms)
{
    if (query_terms.empty()) {
        // Without a query, prefer longer (more informative) sentences,
        // clamped to 1.0 at 200 chars.
        return std::min(1.0, static_cast<double>(sentence.size()) / 200.0);
    }
    const auto words = tokeniseWords(sentence);
    size_t hits = 0;
    std::unordered_set<std::string> seen = {};

    for (const auto& w : words) {
        if (query_terms.count(w) && !seen.count(w)) {
            ++hits;
            seen.insert(w);
        }
    }
    return static_cast<double>(hits) / static_cast<double>(query_terms.size());
}

/// Split @p text into sentences (split on '.', '!', '?').
std::vector<std::string> splitSentencesSimple(const std::string& text) {
    std::vector<std::string> sentences;
    std::string current = {};
    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        current += ch;
        if ((ch == '.' || ch == '!' || ch == '?') &&
            i + 1 < text.size() &&
            (text[i + 1] == ' ' || text[i + 1] == '\n')) {
            const std::string trimmed = [&]() {
                const auto start = current.find_first_not_of(" \t\n\r");
                const auto end   = current.find_last_not_of(" \t\n\r");
                if (start == std::string::npos) return std::string{};
                return current.substr(start, end - start + 1);
            }();
            if (!trimmed.empty()) {
                sentences.push_back(trimmed);
            }
            current.clear();
        }
    }
    // Include any trailing fragment without terminal punctuation
    const auto start = current.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
        sentences.push_back(current.substr(start));
    }
    return sentences;
}

/// Build an extractive summary of @p content by picking up to
/// @p max_sentences sentences that best match @p query_terms.
/// The returned string has total length <= @p budget_chars (0 = unlimited).
std::string extractiveSummary(
    const std::string& content,
    const std::unordered_set<std::string>& query_terms,
    size_t max_sentences,
    size_t min_sentence_chars,
    size_t budget_chars)
{
    const auto all_sentences = splitSentencesSimple(content);

    // Score each sentence
    std::vector<std::pair<double, size_t>> scored; // (score, index)
    scored.reserve(all_sentences.size());
    for (size_t i = 0; i < all_sentences.size(); ++i) {
        if (all_sentences[i].size() >= min_sentence_chars) {
            scored.emplace_back(
                scoreSentence(all_sentences[i], query_terms), i);
        }
    }

    // Sort descending by score, then by original position as tiebreaker
    std::stable_sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    // Select up to max_sentences, respecting budget
    std::vector<size_t> selected_indices;
    size_t chars_used = 0;
    for (const auto& [score, idx] : scored) {
        if (selected_indices.size() >= max_sentences) {
          break;
        }
        const size_t len = all_sentences[idx].size();
        if (budget_chars > 0 && chars_used + len > budget_chars) {
          continue;
        }
        selected_indices.push_back(idx);
        chars_used += len + 1; // +1 for separator space
    }

    // Re-sort selected sentences back into document order
    std::sort(selected_indices.begin(), selected_indices.end());

    // Optimization: Use stringstream for efficient string building in loop
    // Complexity: O(n) linear time, avoids O(n²) reallocation behavior
    std::ostringstream ss = {};
    bool first = true;
    for (size_t idx : selected_indices) {
        if (!first) {
          ss << ' ';
        }
        ss << all_sentences[idx];
        first = false;
    }
    return ss.str();
}

/// Build an abstractive (LLM-based) summary prompt for one document.
std::string buildSingleDocPrompt(const std::string& query,
                                  const std::string& document_id,
                                  const std::string& content,
                                  size_t max_chars)
{
    std::ostringstream oss = {};
    oss << "Summarize the following document";
    if (!query.empty()) {
        oss << " to answer the query: \"" << query << "\"";
    }
    oss << ".\nKeep the summary under " << max_chars
        << " characters.\n\n"
        << "Document [" << document_id << "]:\n"
        << content
        << "\n\nSummary:";
    return oss.str();
}

/// Build an abstractive prompt for multiple documents.
std::string buildMultiDocPrompt(
    const std::string& query,
    const std::vector<std::pair<std::string, std::string>>& id_content,
    size_t max_chars)
{
    std::ostringstream oss = {};
    oss << "Summarize the following " << id_content.size() << " document(s)";
    if (!query.empty()) {
        oss << " to answer the query: \"" << query << "\"";
    }
    oss << ".\nKeep the combined summary under " << max_chars
        << " characters.\n\n";
    for (size_t i = 0; i < id_content.size(); ++i) {
        oss << "Document " << (i + 1) << " [" << id_content[i].first << "]:\n"
            << id_content[i].second << "\n\n";
    }
    oss << "Combined Summary:";
    return oss.str();
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Impl
// ─────────────────────────────────────────────────────────────────────────────

struct DocumentSummarizer::Impl {
    DocumentSummarizerConfig config;

    /// Determine the effective strategy at runtime.
    DocumentSummarizerConfig::Strategy effectiveStrategy() const {
        if (config.strategy != DocumentSummarizerConfig::Strategy::AUTO) {
            return config.strategy;
        }
        // AUTO: use abstractive only when an LLM engine is wired up
        return LLMIntegration::getInferenceEngine()
            ? DocumentSummarizerConfig::Strategy::ABSTRACTIVE
            : DocumentSummarizerConfig::Strategy::EXTRACTIVE;
    }

    /// Build the set of query terms for extractive scoring.
    std::unordered_set<std::string> queryTerms(const std::string& query) const {
        std::unordered_set<std::string> terms = {};

        for (auto& w : tokeniseWords(query)) {
            if (w.size() > 2) { // skip stop-word candidates
                terms.insert(w);
            }
        }
        return terms;
    }

    /// Produce a DocumentSummary for one (id, content) pair.
    DocumentSummary summarizeOne(const std::string& id,
                                  const std::string& content,
                                  const std::string& query,
                                  size_t budget_chars) const
    {
        DocumentSummary ds;
        ds.document_id    = id;
        ds.used_llm       = false;
        ds.coverage_score = 0.0;

        if (content.empty()) {
            return ds;
        }

        const auto strategy = effectiveStrategy();

        if (strategy == DocumentSummarizerConfig::Strategy::ABSTRACTIVE) {
            LLMGenerationOptions opts;
            opts.temperature  = config.temperature;
            opts.max_tokens   = config.max_output_tokens;
            const std::string prompt = buildSingleDocPrompt(
                query, id, content, budget_chars > 0 ? budget_chars : config.max_summary_chars);
            ds.summary  = LLMIntegration::generate(prompt, opts);
            ds.used_llm = true;
        } else {
            // EXTRACTIVE
            const auto qterms   = queryTerms(query);
            ds.summary = extractiveSummary(
                content, qterms,
                config.max_sentences_per_doc,
                config.min_sentence_chars,
                budget_chars > 0 ? budget_chars : config.max_summary_chars);
        }

        // coverage_score: distinct sentences in summary / total sentences
        const auto total_sents   = splitSentencesSimple(content).size();
        const auto summary_sents = splitSentencesSimple(ds.summary).size();
        ds.coverage_score = total_sents > 0
            ? std::min(1.0, static_cast<double>(summary_sents) /
                            static_cast<double>(total_sents))
            : 1.0;

        return ds;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DocumentSummarizer – construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

DocumentSummarizer::DocumentSummarizer()
    : impl_(std::make_unique<Impl>()) {}

DocumentSummarizer::DocumentSummarizer(const DocumentSummarizerConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

DocumentSummarizer::~DocumentSummarizer() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

const DocumentSummarizerConfig& DocumentSummarizer::getConfig() const {
    return impl_->config;
}

void DocumentSummarizer::setConfig(const DocumentSummarizerConfig& config) {
    impl_->config = config;
}

// ─────────────────────────────────────────────────────────────────────────────
// Single-document summarization
// ─────────────────────────────────────────────────────────────────────────────

DocumentSummary DocumentSummarizer::summarize(const std::string& document_id,
                                               const std::string& content,
                                               const std::string& query) const
{
    THEMIS_DEBUG("DocumentSummarizer::summarize doc={} chars={}",
                 document_id, content.size());
    return impl_->summarizeOne(document_id, content, query,
                               impl_->config.max_summary_chars);
}

// ─────────────────────────────────────────────────────────────────────────────
// Multi-document summarization (RetrievedDocument)
// ─────────────────────────────────────────────────────────────────────────────

MultiDocumentSummary DocumentSummarizer::summarizeMultiple(
    const std::vector<judge::RetrievedDocument>& documents,
    const std::string& query) const
{
    const auto t_start = std::chrono::steady_clock::now();

    THEMIS_INFO("DocumentSummarizer::summarizeMultiple docs={} query='{}'",
                documents.size(), query);

    MultiDocumentSummary result = {};
    if (documents.empty()) {
        return result;
    }

    // Measure total input size
    for (const auto& d : documents) {
        result.total_input_chars += d.content.size();
    }

    const auto strategy = impl_->effectiveStrategy();
    result.used_llm =
        (strategy == DocumentSummarizerConfig::Strategy::ABSTRACTIVE);

    if (strategy == DocumentSummarizerConfig::Strategy::ABSTRACTIVE) {
        // Single batched LLM call for all documents
        std::vector<std::pair<std::string, std::string>> id_content;
        id_content.reserve(documents.size());
        for (const auto& d : documents) {
            id_content.emplace_back(d.id, d.content);
        }
        const std::string prompt = buildMultiDocPrompt(
            query, id_content, impl_->config.max_summary_chars);

        LLMGenerationOptions opts;
        opts.temperature = impl_->config.temperature;
        opts.max_tokens  = impl_->config.max_output_tokens;
        result.combined_summary = LLMIntegration::generate(prompt, opts);
        
        // Validate LLM response
        if (result.combined_summary.empty()) {
            THEMIS_WARN("DocumentSummarizer: Empty response from LLM for multi-document summary");
            // Fall back to extractive summary instead of empty result
            for (const auto& d : documents) {
                const auto qterms = impl_->queryTerms(query);
                const std::string extractive = extractiveSummary(
                    d.content, qterms,
                    impl_->config.max_sentences_per_doc,
                    impl_->config.min_sentence_chars,
                    impl_->config.max_summary_chars / documents.size());
                result.combined_summary += extractive + "\n";
            }
        }
        
        // Per-document breakdowns using extractive (no extra LLM calls)
        const size_t per_doc_budget =
            impl_->config.max_summary_chars / documents.size();
        const auto qterms = impl_->queryTerms(query);
        for (const auto& d : documents) {
            DocumentSummary ds;
            ds.document_id  = d.id;
            ds.used_llm     = false;
            ds.summary = extractiveSummary(
                d.content, qterms,
                impl_->config.max_sentences_per_doc,
                impl_->config.min_sentence_chars,
                per_doc_budget);
            const auto total_sents   = splitSentencesSimple(d.content).size();
            const auto summary_sents = splitSentencesSimple(ds.summary).size();
            ds.coverage_score = total_sents > 0
                ? std::min(1.0, static_cast<double>(summary_sents) /
                                static_cast<double>(total_sents))
                : 1.0;
            result.per_document_summaries.push_back(std::move(ds));
        }
    } else {
        // EXTRACTIVE: per-document budgets, then concatenate
        const size_t per_doc_budget = documents.empty() ? 0
            : impl_->config.max_summary_chars / documents.size();

        std::ostringstream combined = {};
        for (const auto& d : documents) {
            auto ds = impl_->summarizeOne(d.id, d.content, query, per_doc_budget);

            if (!ds.summary.empty()) {
                if (impl_->config.include_source_attribution) {
                    combined << "[Source: " << d.id << "] ";
                }
                combined << ds.summary << '\n';
            }
            result.per_document_summaries.push_back(std::move(ds));
        }

        result.combined_summary = combined.str();
        // Trim trailing newline
        if (!result.combined_summary.empty() &&
            result.combined_summary.back() == '\n') {
            result.combined_summary.pop_back();
        }
    }

    result.summary_chars =
        result.combined_summary.size();
    result.compression_ratio = result.total_input_chars > 0
        ? static_cast<double>(result.summary_chars) /
          static_cast<double>(result.total_input_chars)
        : 0.0;

    const auto t_end = std::chrono::steady_clock::now();
    result.elapsed_ms =
        std::chrono::duration<double, std::milli>(t_end - t_start).count();

    THEMIS_INFO("DocumentSummarizer complete: input={} chars, summary={} chars, "
                "ratio={:.2f}, elapsed={:.1f}ms, llm={}",
                result.total_input_chars, result.summary_chars,
                result.compression_ratio, result.elapsed_ms, result.used_llm);

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Multi-document summarization (StreamedDocument)
// ─────────────────────────────────────────────────────────────────────────────

MultiDocumentSummary DocumentSummarizer::summarizeMultiple(
    const std::vector<streaming::StreamedDocument>& documents,
    const std::string& query) const
{
    // Convert to RetrievedDocument then delegate
    std::vector<judge::RetrievedDocument> converted = {};

    converted.reserve(documents.size());
    for (const auto& sd : documents) {
        judge::RetrievedDocument rd;
        rd.id               = sd.id;
        rd.content          = sd.content;
        rd.similarity_score = sd.relevance_score;
        converted.push_back(std::move(rd));
    }
    return summarizeMultiple(converted, query);
}

// ─────────────────────────────────────────────────────────────────────────────
// DocumentSummarizerFactory
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<DocumentSummarizer> DocumentSummarizerFactory::createExtractive(
    size_t max_sentences)
{
    DocumentSummarizerConfig cfg;
    cfg.strategy              = DocumentSummarizerConfig::Strategy::EXTRACTIVE;
    cfg.max_sentences_per_doc = max_sentences;
    return std::make_unique<DocumentSummarizer>(cfg);
}

std::unique_ptr<DocumentSummarizer> DocumentSummarizerFactory::createAbstractive(
    size_t max_summary_chars)
{
    DocumentSummarizerConfig cfg;
    cfg.strategy          = DocumentSummarizerConfig::Strategy::ABSTRACTIVE;
    cfg.max_summary_chars = max_summary_chars;
    return std::make_unique<DocumentSummarizer>(cfg);
}

std::unique_ptr<DocumentSummarizer> DocumentSummarizerFactory::createAuto() {
    return std::make_unique<DocumentSummarizer>();
}

} // namespace themis::rag
