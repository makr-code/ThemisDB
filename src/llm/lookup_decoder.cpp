/**
 * @file lookup_decoder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lookup_decoder.h"
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace llm {

// ── VectorHash ──────────────────────────────────────────────────────

size_t LookupDecoder::VectorHash::operator()(
    const std::vector<int>& v) const noexcept
{
    // FNV-1a inspired combine: fast, low-collision for token-ID sequences.
    size_t seed = v.size();
    for (const auto id : v) {
        seed ^= static_cast<size_t>(id) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
    }
    return seed;
}

// ── Constructors ────────────────────────────────────────────────────

LookupDecoder::LookupDecoder() : config_{} {}

LookupDecoder::LookupDecoder(const Config& config) : config_(config) {
    if (config_.ngram_min < 1) {
        throw std::invalid_argument("LookupDecoder: ngram_min must be >= 1");
    }
    if (config_.ngram_max < config_.ngram_min) {
        throw std::invalid_argument(
            "LookupDecoder: ngram_max must be >= ngram_min");
    }
    if (config_.max_draft_tokens == 0) {
        throw std::invalid_argument(
            "LookupDecoder: max_draft_tokens must be >= 1");
    }
}

// ── Index construction ───────────────────────────────────────────────

void LookupDecoder::buildFromPrompt(const std::vector<int>& tokens) {
    std::lock_guard<std::mutex> lock(mutex_);
    index_.clear();
    insertion_order_.clear();
    indexTokens(tokens);
}

void LookupDecoder::updateFromTokens(const std::vector<int>& new_tokens) {
    if (new_tokens.empty()) {
      return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    indexTokens(new_tokens);
}

void LookupDecoder::loadStaticNgrams(
    const std::unordered_map<std::vector<int>,
                             std::vector<int>,
                             VectorHash>& ngrams)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [key, cont] : ngrams) {
        if (static_cast<int>(key.size()) >= config_.ngram_min &&
            static_cast<int>(key.size()) <= config_.ngram_max &&
            !cont.empty()) {
            std::vector<int> trimmed(
                cont.begin(),
                cont.begin() + static_cast<ptrdiff_t>(
                    std::min(cont.size(), config_.max_draft_tokens)));
            insertEntry(key, std::move(trimmed));
        }
    }
}

void LookupDecoder::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    index_.clear();
    insertion_order_.clear();
}

// ── Draft proposal ───────────────────────────────────────────────────

std::vector<int> LookupDecoder::proposeDraftTokens(
    const std::vector<int>& context_tokens,
    size_t                  max_draft
) const
{
    if (max_draft == 0) {
      max_draft = config_.max_draft_tokens;
    }
    max_draft = std::min(max_draft, config_.max_draft_tokens);

    std::lock_guard<std::mutex> lock(mutex_);
    stats_.total_probe_calls++;

    if (context_tokens.empty() || index_.empty()) {
        return {};
    }

    // Probe from longest to shortest n-gram (greedy longest match).
    for (size_t n = config_.ngram_max; n >= config_.ngram_min; --n) {
        if (static_cast<int>(context_tokens.size()) < n) {
          continue;
        }

        // Extract the last `n` tokens as query key.
        std::vector<int> key(
            context_tokens.end() - static_cast<ptrdiff_t>(n),
            context_tokens.end());

        auto it = index_.find(key);
        if (it != index_.end() && !it->second.empty()) {
            const auto& cont = it->second;
            std::vector<int> draft(
                cont.begin(),
                cont.begin() + static_cast<ptrdiff_t>(
                    std::min(cont.size(), max_draft)));
            stats_.total_hits++;
            stats_.total_draft_tokens_proposed += draft.size();
            return draft;
        }
    }
    return {};
}

// ── Statistics ───────────────────────────────────────────────────────

LookupDecoder::Stats LookupDecoder::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void LookupDecoder::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_ = Stats{};
}

// ── Internal helpers ─────────────────────────────────────────────────

void LookupDecoder::insertEntry(std::vector<int> key,
                                 std::vector<int> continuation)
{
    // Evict oldest entry if at capacity.
    if (static_cast<int>(index_.size()) >= config_.max_index_entries &&
        !insertion_order_.empty()) {
        const auto& oldest = insertion_order_.front();
        index_.erase(oldest);
        insertion_order_.erase(insertion_order_.begin());
    }
    // Insert (or overwrite) — newer observations win.
    if (index_.find(key) == index_.end()) {
        insertion_order_.push_back(key);
    }
    index_[key] = std::move(continuation);
}

void LookupDecoder::indexTokens(const std::vector<int>& tokens) {
    // Slide a window of size [ngram_min..ngram_max] across the token sequence.
    // For each window: key = first n tokens, continuation = tokens after the key.
    for (size_t n = config_.ngram_min; n <= config_.ngram_max; ++n) {
        if (static_cast<int>(tokens.size()) <= n) continue;  // need at least one continuation token

        for (size_t start = 0; start + n < tokens.size(); ++start) {
            std::vector<int> key(tokens.begin() + static_cast<ptrdiff_t>(start),
                                 tokens.begin() + static_cast<ptrdiff_t>(start + n));
            // Continuation: up to max_draft_tokens tokens following the key.
            const size_t cont_start = start + n;
            const size_t cont_end =
                std::min(cont_start + config_.max_draft_tokens,static_cast<int>(tokens.size()));
            std::vector<int> cont(tokens.begin() + static_cast<ptrdiff_t>(cont_start),
                                  tokens.begin() + static_cast<ptrdiff_t>(cont_end));
            if (!cont.empty()) {
                insertEntry(std::move(key), std::move(cont));
            }
        }
    }
}

} // namespace llm
} // namespace themis

