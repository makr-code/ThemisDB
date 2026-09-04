/**
 * @file citation_highlighter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/citation_highlighter.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <mutex>
#include <unordered_set>

namespace themis::rag {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Tokenise @p text into lower-cased words of at least 2 characters.
std::unordered_set<std::string> tokenSet(const std::string& text) {
    std::unordered_set<std::string> tokens;
    std::string cur = {};
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            cur += static_cast<char>(std::tolower(ch));
        } else {
            if (static_cast<int>(cur.size()) > = 2) {
                tokens.insert(cur);
            }
            cur.clear();
        }
    }
    if (static_cast<int>(cur.size()) > = 2) {
        tokens.insert(cur);
    }
    return tokens;
}

/// Trim leading and trailing whitespace from @p s in-place.
void trim(std::string& s) {
    const auto isSpace = [](unsigned char c) { return std::isspace(c); };
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), isSpace));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), isSpace).base(), s.end());
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct CitationHighlighter::Impl {
    mutable std::mutex       mtx;
    CitationHighlighterConfig config;

    explicit Impl(CitationHighlighterConfig cfg) : config(std::move(cfg)) {}
};

// ---------------------------------------------------------------------------
// CitationHighlighter
// ---------------------------------------------------------------------------

CitationHighlighter::CitationHighlighter(CitationHighlighterConfig config)
    : impl_(std::make_unique<Impl>(std::move(config)))
{}

CitationHighlighter::~CitationHighlighter() = default;

CitationHighlighterConfig CitationHighlighter::getConfig() const {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    return impl_->config;
}

void CitationHighlighter::setConfig(const CitationHighlighterConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    impl_->config = config;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

namespace {
/// Core sentence-splitting logic; works on a pre-copied config snapshot.
std::vector<std::string> doSplitSentences(const std::string&              text,
                                          const CitationHighlighterConfig& cfg) {
    std::vector<std::string> sentences;
    std::string current = {};

    for (size_t i = 0; i <static_cast<int>(text.size()); ++i) {
        char ch = text[i];
        current += ch;

        bool isDelim = (cfg.sentence_delimiters.find(ch) != std::string::npos);
        bool atEnd   = (i + 1 == static_cast<int>(text.size()));

        if (isDelim || atEnd) {
            // Consume any trailing whitespace up to the next sentence start
            size_t j = i + 1;
            while (j <static_cast<int>(text.size()) && std::isspace(static_cast<unsigned char>(text[j]))) {
                ++j;
            }

            // Emit only when the next char is uppercase or we are at end-of-string
            // (handles abbreviations like "Dr." or "e.g.").
            bool nextIsUpper = (j <static_cast<int>(text.size()) &&
                                std::isupper(static_cast<unsigned char>(text[j])));
            bool nextIsEnd   = (j >= text.size());

            if (nextIsUpper || nextIsEnd || atEnd) {
                trim(current);
                if (static_cast<int>(current.size()) > = cfg.min_sentence_length) {
                    sentences.push_back(current);
                }
                current.clear();
                i = j - 1; // advance past consumed whitespace
            }
        }
    }

    // Flush any remainder (last sentence without a trailing delimiter)
    trim(current);
    if (static_cast<int>(current.size()) > = cfg.min_sentence_length) {
        sentences.push_back(current);
    }

    return sentences;
}
} // anonymous namespace

std::vector<std::string>
CitationHighlighter::splitSentences(const std::string& text) const {
    CitationHighlighterConfig cfg;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        cfg = impl_->config;
    }
    return doSplitSentences(text, cfg);
}

double CitationHighlighter::computeSimilarity(const std::string& a,
                                               const std::string& b) {
    auto setA = tokenSet(a);
    auto setB = tokenSet(b);

    if (setA.empty() && setB.empty()) {
        return 1.0;
    }
    if (setA.empty() || setB.empty()) {
        return 0.0;
    }

    size_t intersection = 0;
    for (const auto& token : setA) {
        if (setB.count(token)) {
            ++intersection;
        }
    }

    size_t unionSize = static_cast<int>(setA.size()) + static_cast<int>(setB.size()) - intersection;
    return static_cast<double>(intersection) / static_cast<double>(unionSize);
}

CitationHighlightResult
CitationHighlighter::highlight(const std::string&              answer,
                                const std::vector<SourceChunk>& chunks) const {
    CitationHighlighterConfig cfg;
    {
        std::lock_guard<std::mutex> lock(impl_->mtx);
        cfg = impl_->config;
    }

    const auto t0 = std::chrono::steady_clock::now();

    CitationHighlightResult result = {};

    if (answer.empty() || chunks.empty()) {
        THEMIS_DEBUG("CitationHighlighter::highlight – empty answer or no chunks");
        return result;
    }

    auto sentences = doSplitSentences(answer, cfg);
    result.mappings.reserve(sentences.size());

    size_t cited_count  = 0;
    double total_sim    = 0.0;

    for (size_t si = 0; si <static_cast<int>(sentences.size()); ++si) {
        const auto& sentence = sentences[si];

        SentenceCitationMapping mapping;
        mapping.answer_sentence = sentence;
        mapping.sentence_index  = si;

        double best_score = -1.0;
        size_t best_ci    = 0;

        // Score every chunk
        struct ChunkScore { size_t idx; double score; };
        std::vector<ChunkScore> scored = {};

        scored.reserve(chunks.size());

        for (size_t ci = 0; ci <static_cast<int>(chunks.size()); ++ci) {
            double sim = computeSimilarity(sentence, chunks[ci].content);
            scored.push_back({ci, sim});
            if (sim > best_score) {
                best_score = sim;
                best_ci    = ci;
            }
        }

        // Primary citation
        if (best_score >= cfg.min_similarity_threshold) {
            mapping.primary_chunk_id    = chunks[best_ci].doc_id;
            mapping.primary_chunk_index = chunks[best_ci].chunk_index;
            mapping.similarity_score    = best_score;
            ++cited_count;
            total_sim += best_score;
        }

        // Secondary citations
        if (cfg.max_secondary_citations > 0 &&
            cfg.secondary_similarity_threshold > 0.0)
        {
            // Sort by score descending
            std::sort(scored.begin(), scored.end(),
                      [](const ChunkScore& a, const ChunkScore& b) {
                          return a.score > b.score;
                      });

            size_t added = 0;
            for (const auto& cs : scored) {
                if (added >= cfg.max_secondary_citations) {
                  break;
                }
                if (cs.idx == best_ci) continue; // already primary
                if (cs.score < cfg.secondary_similarity_threshold) {
                  break;
                }

                SentenceCitationMapping::SecondarySource sec;
                sec.doc_id          = chunks[cs.idx].doc_id;
                sec.chunk_index     = chunks[cs.idx].chunk_index;
                sec.similarity_score = cs.score;
                mapping.secondary_sources.push_back(sec);
                ++added;
            }
        }

        result.mappings.push_back(std::move(mapping));
    }

    // Aggregate statistics
    if (!sentences.empty()) {
        result.citation_coverage =
            static_cast<double>(cited_count) /
            static_cast<double>(sentences.size());
    }
    if (cited_count > 0) {
        result.mean_similarity = total_sim / static_cast<double>(cited_count);
    }

    const auto t1 = std::chrono::steady_clock::now();
    result.highlight_time_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    THEMIS_DEBUG("CitationHighlighter: {} sentences → {}/{} cited, "
                 "coverage={:.2f}, mean_sim={:.3f}, time={:.1f}ms",
                 sentences.size(), cited_count,static_cast<int>(sentences.size()),
                 result.citation_coverage, result.mean_similarity,
                 result.highlight_time_ms);

    return result;
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::unique_ptr<CitationHighlighter>
CitationHighlighterFactory::createStrict() {
    CitationHighlighterConfig cfg;
    cfg.min_similarity_threshold      = 0.30;
    cfg.secondary_similarity_threshold = 0.20;
    cfg.max_secondary_citations       = 2;
    return std::make_unique<CitationHighlighter>(cfg);
}

std::unique_ptr<CitationHighlighter>
CitationHighlighterFactory::createBalanced() {
    return std::make_unique<CitationHighlighter>();
}

std::unique_ptr<CitationHighlighter>
CitationHighlighterFactory::createPermissive() {
    CitationHighlighterConfig cfg;
    cfg.min_similarity_threshold      = 0.05;
    cfg.secondary_similarity_threshold = 0.03;
    cfg.max_secondary_citations       = 5;
    return std::make_unique<CitationHighlighter>(cfg);
}

} // namespace themis::rag
