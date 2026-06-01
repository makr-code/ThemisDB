/*
 * ThemisDB | File: token_quota_manager.h | Version: 0.0.47 | Last Modified: 2026-05-28 20:56:02
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 165
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>

namespace themis {
namespace llm {

/**
 * @brief Result of a quota admission check.
 */
struct QuotaCheckResult {
    bool allowed = true;         ///< false if the request was denied by quota
    std::string reason;          ///< human-readable explanation when denied
    size_t tokens_used = 0;          ///< tokens consumed in the current window
    size_t tokens_limit = 0;         ///< configured limit for the window
};

/**
 * @brief Token-per-minute quota manager (sliding-window, per key).
 *
 * Enforces an independent token budget for each (user_id, model_id) key.
 * Quotas are tracked in a 60-second sliding window: tokens consumed more
 * than 60 seconds ago are automatically discarded.
 *
 * ### Usage (Q1 minimum viable path)
 * @code
 *   TokenQuotaManager quota;
 *   quota.setQuota("alice", "mistral-7b", 50000);  // 50k tokens/min
 *
 *   // Before submitting to scheduler:
 *   auto result = quota.check("alice", "mistral-7b", estimated_tokens);
 *   if (!result.allowed) {
 *       // Reject with 429-like response; log result.reason
 *   }
 *   // After inference completes:
 *   quota.consume("alice", "mistral-7b", actual_tokens_used);
 * @endcode
 *
 * ### Thread safety
 * All methods are thread-safe (guarded by an internal mutex).
 *
 * @see docs/llm_roadmap.md — Q1 Safety/Policy checklist
 * @see docs/operations/llm/QUOTA_TUNING.md
 */
class TokenQuotaManager {
public:
    /// Sliding window length — fixed at 60 seconds.
    static constexpr std::chrono::seconds WINDOW{60};

    TokenQuotaManager() = default;
    ~TokenQuotaManager() = default;

    // Thread-safe, move-only.
    TokenQuotaManager(const TokenQuotaManager&) = delete;
    TokenQuotaManager& operator=(const TokenQuotaManager&) = delete;
    TokenQuotaManager(TokenQuotaManager&&) = default;
    TokenQuotaManager& operator=(TokenQuotaManager&&) = default;

    /**
     * @brief Set (or replace) the token-per-minute limit for a (user, model) pair.
     *
     * @param user_id   User or service account identifier.
     * @param model_id  Model identifier (e.g. "mistral-7b").
     * @param limit     Maximum tokens per 60-second window.  0 = unlimited.
     */
    void setQuota(const std::string& user_id,
                  const std::string& model_id,
                  size_t limit);

    /**
     * @brief Remove the quota for a (user, model) pair.
     *
     * After removal the pair is treated as unlimited.
     *
     * @return true if a quota entry was found and removed.
     */
    bool removeQuota(const std::string& user_id, const std::string& model_id);

    /**
     * @brief Check whether a request with @p estimated_tokens would be within
     *        quota WITHOUT recording the consumption.
     *
     * Use this before scheduling; call consume() once the request is accepted
     * to record the actual token count.
     *
     * If no quota has been set for (user_id, model_id) the check always
     * returns allowed=true (quota is opt-in).
     *
     * @param user_id          User identifier.
     * @param model_id         Model identifier.
     * @param estimated_tokens Estimated prompt + completion tokens.
     */
    QuotaCheckResult check(const std::string& user_id,
                           const std::string& model_id,
                           size_t estimated_tokens) const;

    /**
     * @brief Record @p tokens as consumed for (user_id, model_id).
     *
     * Should be called after a request completes (or at submission time if
     * a conservative pre-charge approach is preferred).
     *
     * No-op if no quota is set for this pair.
     *
     * @param user_id  User identifier.
     * @param model_id Model identifier.
     * @param tokens   Actual tokens consumed.
     */
    void consume(const std::string& user_id,
                 const std::string& model_id,
                 size_t tokens);

    /**
     * @brief Return the number of tokens consumed in the current window for
     *        (user_id, model_id).
     *
     * Returns 0 if no quota entry exists or if all events have expired.
     */
    size_t currentUsage(const std::string& user_id,
                        const std::string& model_id) const;

    /**
     * @brief Return the configured limit for (user_id, model_id), or
     *        std::nullopt if no quota is set.
     */
    std::optional<size_t> getLimit(const std::string& user_id,
                                   const std::string& model_id) const;

private:
    struct Event {
        std::chrono::steady_clock::time_point at;
        size_t tokens = 0;
    };

    struct QuotaEntry {
        size_t limit = 0;               ///< tokens per window (0 = unlimited)
        mutable std::vector<Event> events; ///< sliding-window log
    };

    static std::string makeKey(const std::string& user_id,
                               const std::string& model_id) {
        return user_id + '\0' + model_id;
    }

    // Discard events older than WINDOW.  Caller must hold mutex_.
    static void prune(QuotaEntry& entry);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, QuotaEntry> entries_;
};

} // namespace llm
} // namespace themis
