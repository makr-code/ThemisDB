/**
 * @file streaming_retriever.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/streaming_retriever.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <unordered_set>
#include <mutex>

namespace themis::rag::streaming {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

/**
 * Compute Jaccard similarity between two token sets derived from
 * lowercased whitespace-split words.  Used for MMR deduplication.
 */
double jaccardSimilarity(const std::string& a, const std::string& b) {
    auto tokenize = [](const std::string& text) {
        std::unordered_set<std::string> tokens;
        std::istringstream stream(text);
        std::string word = {};
        while (stream >> word) {
            // Lowercase
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            tokens.insert(std::move(word));
        }
        return tokens;
    };

    auto ta = tokenize(a);
    auto tb = tokenize(b);

    if (ta.empty() && tb.empty()) {
      return 1.0;
    }
    if (ta.empty() || tb.empty()) {
      return 0.0;
    }

    size_t intersection = 0;
    for (const auto& t : ta) {
        if (tb.count(t)) {
          ++intersection;
        }
    }
    const size_t unionSize = static_cast<int>(ta.size()) + static_cast<int>(tb.size()) - intersection;
    return unionSize == 0 ? 0.0 : static_cast<double>(intersection) / static_cast<double>(unionSize);
}

} // anonymous namespace

// ===========================================================================
// ContextWindowFiller
// ===========================================================================

ContextWindowFiller::ContextWindowFiller(size_t max_tokens, double chars_per_token)
    : max_tokens_(max_tokens)
    , chars_per_token_(chars_per_token > 0.0 ? chars_per_token : 4.0) {
}

size_t ContextWindowFiller::estimateTokens(const std::string& text) const {
    if (text.empty()) {
      return 0;
    }
    // Use pre-computed count if the caller set it to non-zero, otherwise
    // estimate from character count.
    const double estimated = static_cast<double>(text.size()) / chars_per_token_;
    return static_cast<size_t>(std::ceil(estimated));
}

bool ContextWindowFiller::tryAdd(const StreamedDocument& doc) {
    const size_t needed = doc.token_count > 0
        ? doc.token_count
        : estimateTokens(doc.content);

    if (tokens_used_ + needed > max_tokens_) {
        return false;
    }

    // Build a mutable copy so we can back-fill token_count if needed.
    StreamedDocument entry = doc;
    if (entry.token_count == 0) {
        entry.token_count = needed;
    }

    tokens_used_ += entry.token_count;
    documents_.push_back(std::move(entry));
    return true;
}

bool ContextWindowFiller::hasCapacity([[maybe_unused]] size_t min_tokens) const {
    return (max_tokens_ >= tokens_used_) &&
           (max_tokens_ - tokens_used_ >= min_tokens);
}

ContextWindowState ContextWindowFiller::snapshot() const {
    ContextWindowState state;
    state.documents        = documents_;
    state.total_tokens_used = tokens_used_;
    state.max_tokens       = max_tokens_;
    state.fill_ratio       = max_tokens_ > 0
        ? static_cast<double>(tokens_used_) / static_cast<double>(max_tokens_)
        : 1.0;
    state.is_full          = !hasCapacity(1);
    return state;
}

const std::vector<StreamedDocument>& ContextWindowFiller::documents() const {
    return documents_;
}

void ContextWindowFiller::reset() {
    documents_.clear();
    tokens_used_ = 0;
}

// ===========================================================================
// StreamingRetriever::Impl
// ===========================================================================

struct StreamingRetriever::Impl {
    StreamingRetrieverConfig config;

    DocumentAcceptedCallback on_accepted;
    DocumentSkippedCallback  on_skipped;
    WindowFullCallback       on_window_full;

    std::atomic<bool> streaming{false};
    std::atomic<bool> cancel_requested{false};
    
    // Synchronization for thread-safe callback access
    mutable std::mutex callback_mutex;

    /**
     * Check whether @p candidate is too similar to any already-selected
     * document.  Returns true when the document should be skipped.
     */
    bool isDuplicate(const StreamedDocument& candidate,
                     const std::vector<StreamedDocument>& selected) const {
        if (!config.enable_mmr_deduplication) {
          return false;
        }
        for (const auto& doc : selected) {
            if (jaccardSimilarity(candidate.content, doc.content) >=
                config.mmr_similarity_threshold) {
                return true;
            }
        }
        return false;
    }
};

// ===========================================================================
// StreamingRetriever
// ===========================================================================

StreamingRetriever::StreamingRetriever(const StreamingRetrieverConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    THEMIS_DEBUG("StreamingRetriever created: max_context_tokens={}, "
                 "sort_by_relevance={}, mmr={}",
                 config.max_context_tokens,
                 config.sort_by_relevance,
                 config.enable_mmr_deduplication);
}

StreamingRetriever::~StreamingRetriever() = default;

void StreamingRetriever::setDocumentAcceptedCallback([[maybe_unused]] DocumentAcceptedCallback cb) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] impl_->callback_mutex);
    impl_->on_accepted = std::move(cb);
}

void StreamingRetriever::setDocumentSkippedCallback([[maybe_unused]] DocumentSkippedCallback cb) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] impl_->callback_mutex);
    impl_->on_skipped = std::move(cb);
}

void StreamingRetriever::setWindowFullCallback([[maybe_unused]] WindowFullCallback cb) {
    std::lock_guard<std::mutex> lock([[maybe_unused]] impl_->callback_mutex);
    impl_->on_window_full = std::move(cb);
}

void StreamingRetriever::cancel() {
    impl_->cancel_requested.store(true, std::memory_order_relaxed);
}

bool StreamingRetriever::isStreaming() const {
    return impl_->streaming.load(std::memory_order_relaxed);
}

StreamingRetrieverConfig StreamingRetriever::getConfig() const {
    return impl_->config;
}

void StreamingRetriever::setConfig(const StreamingRetrieverConfig& config) {
    impl_->config = config;
}

StreamingResult StreamingRetriever::stream(const std::string& query,
                                           std::vector<StreamedDocument> candidates) {
    const auto t_start = std::chrono::steady_clock::now();

    impl_->streaming.store(true, std::memory_order_relaxed);
    impl_->cancel_requested.store(false, std::memory_order_relaxed);

    THEMIS_INFO("StreamingRetriever::stream started: query='{}', candidates={}",
                query,static_cast<int>(candidates.size()));

    StreamingResult result{};
    result.documents_considered = candidates.size();
    result.cancelled = false;

    ContextWindowFiller filler(impl_->config.max_context_tokens,
                               impl_->config.chars_per_token);

    // ------------------------------------------------------------------
    // 1. Apply relevance filter
    // ------------------------------------------------------------------
    if (impl_->config.min_relevance_score > 0.0) {
        candidates.erase(
            std::remove_if(candidates.begin(), candidates.end(),
                [threshold = impl_->config.min_relevance_score](const StreamedDocument& d) {
                    return d.relevance_score < threshold;
                }),
            candidates.end());
        THEMIS_DEBUG("After relevance filter: {} candidates remain",static_cast<int>(candidates.size()));
    }

    // ------------------------------------------------------------------
    // 2. Cap number of documents to consider
    // ------------------------------------------------------------------
    if (impl_->config.max_documents_to_consider > 0 &&
        static_cast<int>(candidates.size()) > impl_->config.max_documents_to_consider) {
        candidates.resize(impl_->config.max_documents_to_consider);
    }

    // ------------------------------------------------------------------
    // 3. Sort by relevance (highest first)
    // ------------------------------------------------------------------
    if (impl_->config.sort_by_relevance) {
        std::stable_sort(candidates.begin(), candidates.end(),
            [](const StreamedDocument& a, const StreamedDocument& b) {
                return a.relevance_score > b.relevance_score;
            });
    }

    // ------------------------------------------------------------------
    // 4. Incremental fill loop
    // ------------------------------------------------------------------
    bool window_full_notified = false;

    for (auto& doc : candidates) {
        if (impl_->cancel_requested.load(std::memory_order_relaxed)) {
            THEMIS_INFO("StreamingRetriever::stream cancelled after {} documents added",
                        result.documents_added);
            result.cancelled = true;
            break;
        }

        // Skip if window is already full and the document cannot fit
        if (!filler.hasCapacity(1)) {
            if (!window_full_notified) {
                THEMIS_INFO("Context window full after {} documents ({} tokens)",
                            result.documents_added, filler.snapshot().total_tokens_used);
                WindowFullCallback cb;
                {
                    std::lock_guard<std::mutex> lock([[maybe_unused]] impl_->callback_mutex);
                    cb = impl_->on_window_full;
                }
                if (cb) {
                    cb(filler.snapshot());
                }
                window_full_notified = true;
            }
            result.skipped_documents.push_back(doc);
            continue;
        }

        // MMR deduplication check
        if (impl_->isDuplicate(doc, filler.documents())) {
            THEMIS_DEBUG("Skipping duplicate document: id={}", doc.id);
            result.skipped_documents.push_back(doc);
            DocumentSkippedCallback cb;
            {
                std::lock_guard<std::mutex> lock([[maybe_unused]] impl_->callback_mutex);
                cb = impl_->on_skipped;
            }
            if (cb) {
                cb(doc, filler.snapshot());
            }
            continue;
        }

        // Attempt to add to the context window
        if (filler.tryAdd(doc)) {
            ++result.documents_added;
            THEMIS_DEBUG("Accepted document: id={}, score={:.3f}, tokens_used={}",
                         doc.id, doc.relevance_score,
                         filler.snapshot().total_tokens_used);
            DocumentAcceptedCallback cb;
            {
                std::lock_guard<std::mutex> lock([[maybe_unused]] impl_->callback_mutex);
                cb = impl_->on_accepted;
            }
            if (cb) {
                cb(doc, filler.snapshot());
            }
        } else {
            // Document did not fit
            THEMIS_DEBUG("Skipped document (budget): id={}, score={:.3f}",
                         doc.id, doc.relevance_score);
            result.skipped_documents.push_back(doc);
            DocumentSkippedCallback cb;
            {
                std::lock_guard<std::mutex> lock([[maybe_unused]] impl_->callback_mutex);
                cb = impl_->on_skipped;
            }
            if (cb) {
                cb(doc, filler.snapshot());
            }
        }
    }

    // ------------------------------------------------------------------
    // 5. Finalise result
    // ------------------------------------------------------------------
    result.selected_documents = filler.documents();
    result.total_tokens_used  = filler.snapshot().total_tokens_used;

    const auto t_end = std::chrono::steady_clock::now();
    result.elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    impl_->streaming.store(false, std::memory_order_relaxed);

    THEMIS_INFO("StreamingRetriever::stream complete: selected={}/{}, tokens={}/{}, "
                "elapsed={:.1f}ms, cancelled={}",
                result.documents_added,
                result.documents_considered,
                result.total_tokens_used,
                impl_->config.max_context_tokens,
                result.elapsed_ms,
                result.cancelled);

    return result;
}

} // namespace themis::rag::streaming
