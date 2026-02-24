/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            citation_highlighter.cpp                           ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-24                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     341                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file citation_highlighter.cpp
 * @brief Implementation of citation highlighting (map answer sentences to
 *        source chunks) for RAG Phase 3.
 *
 * Algorithm overview:
 *   1. Split the answer into sentences (punctuation-aware).
 *   2. For each sentence, tokenise into lower-cased unigrams and bigrams.
 *   3. For each source chunk, tokenise identically and compute a weighted
 *      term-overlap fraction (unigrams: weight 1, bigrams: weight 2).
 *   4. Collect all (sentence, chunk) pairs whose score >= min_support_score.
 *   5. Per sentence, keep the top-max_chunks_per_sentence pairs sorted by
 *      descending score.
 *   6. Return the flat list ordered by sentence_index then score.
 */

#include "rag/citation_highlighter.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace themis::rag {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Lowercase a single ASCII character.
inline char toLower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

/// Strip leading and trailing ASCII whitespace from @p s.
std::string trim(const std::string& s) {
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

/// Tokenise @p text into lower-cased words, stripping non-alphanumeric chars.
std::vector<std::string> tokenise(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            current += toLower(c);
        } else {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

/// Build a frequency map of unigrams from a token list.
std::unordered_map<std::string, size_t> unigramFreq(
    const std::vector<std::string>& tokens)
{
    std::unordered_map<std::string, size_t> freq;
    for (const auto& t : tokens) {
        ++freq[t];
    }
    return freq;
}

/// Build a frequency map of bigrams ("w1 w2") from a token list.
std::unordered_map<std::string, size_t> bigramFreq(
    const std::vector<std::string>& tokens)
{
    std::unordered_map<std::string, size_t> freq;
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        freq[tokens[i] + ' ' + tokens[i + 1]]++;
    }
    return freq;
}

/// Weighted overlap between two frequency maps.
/// Returns sum of min(freqA[t], freqB[t]) over all shared terms,
/// divided by the total weight of terms in freqA.
double weightedOverlap(
    const std::unordered_map<std::string, size_t>& freqA,
    const std::unordered_map<std::string, size_t>& freqB,
    double weight)
{
    if (freqA.empty()) return 0.0;
    double shared = 0.0;
    double total  = 0.0;
    for (const auto& [term, cntA] : freqA) {
        total += static_cast<double>(cntA) * weight;
        auto it = freqB.find(term);
        if (it != freqB.end()) {
            shared += static_cast<double>(std::min(cntA, it->second)) * weight;
        }
    }
    return total > 0.0 ? shared / total : 0.0;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// CitationHighlighter::Impl
// ─────────────────────────────────────────────────────────────────────────────

struct CitationHighlighter::Impl {
    CitationHighlighterConfig config;
};

// ─────────────────────────────────────────────────────────────────────────────
// CitationHighlighter – construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

CitationHighlighter::CitationHighlighter()
    : impl_(std::make_unique<Impl>())
{}

CitationHighlighter::CitationHighlighter(const CitationHighlighterConfig& config)
    : impl_(std::make_unique<Impl>())
{
    if (config.min_support_score < 0.0 || config.min_support_score > 1.0) {
        throw std::invalid_argument(
            "CitationHighlighter: min_support_score must be in [0, 1]");
    }
    impl_->config = config;
}

CitationHighlighter::~CitationHighlighter() = default;

// ─────────────────────────────────────────────────────────────────────────────
// CitationHighlighter::splitSentences
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> CitationHighlighter::splitSentences(
    const std::string& text)
{
    std::vector<std::string> sentences;
    if (text.empty()) return sentences;

    std::string current;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        current += c;
        // Split on sentence-ending punctuation followed by whitespace or EOS
        if (c == '.' || c == '!' || c == '?') {
            const bool atEnd = (i + 1 == text.size());
            const bool nextIsSpace = (!atEnd && std::isspace(
                static_cast<unsigned char>(text[i + 1])));
            if (atEnd || nextIsSpace) {
                auto s = trim(current);
                if (!s.empty()) {
                    sentences.push_back(std::move(s));
                }
                current.clear();
            }
        }
    }
    // Remaining text without terminal punctuation
    auto tail = trim(current);
    if (!tail.empty()) {
        sentences.push_back(std::move(tail));
    }
    return sentences;
}

// ─────────────────────────────────────────────────────────────────────────────
// CitationHighlighter::scoreSentenceChunk
// ─────────────────────────────────────────────────────────────────────────────

double CitationHighlighter::scoreSentenceChunk(
    const std::string& sentence,
    const std::string& chunk)
{
    if (sentence.empty() || chunk.empty()) return 0.0;

    const auto sentTokens  = tokenise(sentence);
    const auto chunkTokens = tokenise(chunk);

    if (sentTokens.empty() || chunkTokens.empty()) return 0.0;

    const auto sentUni  = unigramFreq(sentTokens);
    const auto chunkUni = unigramFreq(chunkTokens);
    const auto sentBi   = bigramFreq(sentTokens);
    const auto chunkBi  = bigramFreq(chunkTokens);

    // Weighted overlap: unigrams weight 1, bigrams weight 2
    const double uniScore = weightedOverlap(sentUni, chunkUni, 1.0);
    const double biScore  = weightedOverlap(sentBi,  chunkBi,  2.0);

    // Combine: bigrams receive additional weight when they exist
    if (sentBi.empty()) {
        return uniScore;
    }
    return (uniScore + 2.0 * biScore) / 3.0;
}

// ─────────────────────────────────────────────────────────────────────────────
// CitationHighlighter::highlight
// ─────────────────────────────────────────────────────────────────────────────

CitationHighlightResult CitationHighlighter::highlight(
    const std::string& answer,
    const std::vector<SourceChunk>& chunks) const
{
    const auto startTime = std::chrono::steady_clock::now();

    CitationHighlightResult result;

    if (answer.empty() || chunks.empty()) {
        THEMIS_DEBUG("CitationHighlighter::highlight called with empty answer or chunks");
        result.coverage = 0.0;
        return result;
    }

    result.sentences = splitSentences(answer);
    const auto& cfg  = impl_->config;

    size_t mappedSentences = 0;

    for (size_t si = 0; si < result.sentences.size(); ++si) {
        const std::string& sent = result.sentences[si];

        if (sent.size() < cfg.min_sentence_length) {
            continue;
        }

        // Score this sentence against every chunk
        std::vector<std::pair<double, size_t>> scored; // (score, chunk_idx)
        scored.reserve(chunks.size());
        for (size_t ci = 0; ci < chunks.size(); ++ci) {
            const double s = scoreSentenceChunk(sent, chunks[ci].content);
            if (s >= cfg.min_support_score) {
                scored.emplace_back(s, ci);
            }
        }

        if (scored.empty()) continue;

        // Sort descending by score
        std::sort(scored.begin(), scored.end(),
            [](const auto& a, const auto& b) { return a.first > b.first; });

        // Truncate to max_chunks_per_sentence (0 = no limit)
        const size_t limit = (cfg.max_chunks_per_sentence == 0)
            ? scored.size()
            : std::min(scored.size(), cfg.max_chunks_per_sentence);

        for (size_t k = 0; k < limit; ++k) {
            SentenceChunkMapping m;
            m.sentence_index = si;
            m.sentence_text  = sent;
            m.chunk_id       = chunks[scored[k].second].id;
            m.chunk_text     = chunks[scored[k].second].content;
            m.support_score  = scored[k].first;
            result.mappings.push_back(std::move(m));
        }
        ++mappedSentences;
    }

    // Coverage = fraction of non-short sentences that received a mapping
    const size_t eligibleSentences = [&]() {
        size_t n = 0;
        for (const auto& s : result.sentences) {
            if (s.size() >= cfg.min_sentence_length) ++n;
        }
        return n;
    }();
    result.coverage = eligibleSentences > 0
        ? static_cast<double>(mappedSentences) / static_cast<double>(eligibleSentences)
        : 0.0;

    const auto endTime = std::chrono::steady_clock::now();
    result.elapsed_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime);

    THEMIS_DEBUG("CitationHighlighter: {} sentences, {} mappings, coverage={:.2f}",
        result.sentences.size(), result.mappings.size(), result.coverage);

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// CitationHighlighter::getConfig
// ─────────────────────────────────────────────────────────────────────────────

const CitationHighlighterConfig& CitationHighlighter::getConfig() const {
    return impl_->config;
}

// ─────────────────────────────────────────────────────────────────────────────
// CitationHighlighterFactory
// ─────────────────────────────────────────────────────────────────────────────

std::unique_ptr<CitationHighlighter> CitationHighlighterFactory::createStrict() {
    CitationHighlighterConfig cfg;
    cfg.min_support_score      = 0.3;
    cfg.max_chunks_per_sentence = 2;
    return std::make_unique<CitationHighlighter>(cfg);
}

std::unique_ptr<CitationHighlighter> CitationHighlighterFactory::createBalanced() {
    return std::make_unique<CitationHighlighter>();  // default config
}

std::unique_ptr<CitationHighlighter> CitationHighlighterFactory::createPermissive() {
    CitationHighlighterConfig cfg;
    cfg.min_support_score      = 0.0;
    cfg.max_chunks_per_sentence = 0;  // no limit
    return std::make_unique<CitationHighlighter>(cfg);
}

} // namespace themis::rag
