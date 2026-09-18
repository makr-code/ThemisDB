/**
 * @file token_quota_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>

namespace themis {
namespace llm {

struct QuotaCheckResult {
    bool allowed = true;         ///< false if the request was denied by quota
    std::string reason;          ///< human-readable explanation when denied
    size_t tokens_used = 0;          ///< tokens consumed in the current window
    size_t tokens_limit = 0;         ///< configured limit for the window
};

class TokenQuotaManager {
public:
    static constexpr std::chrono::seconds WINDOW{60};

    TokenQuotaManager() = default;
    ~TokenQuotaManager() = default;

    // Thread-safe, non-moveable (due to std::mutex).
    // Copy operations are deleted because of unique resource ownership.
    // Move operations are deleted because std::mutex is non-moveable.
    // To pass this object across thread/function boundaries, use std::shared_ptr or std::unique_ptr.
    TokenQuotaManager(const TokenQuotaManager&) = delete;
    TokenQuotaManager& operator=(const TokenQuotaManager&) = delete;
    TokenQuotaManager(TokenQuotaManager&&) = delete;  // Deleted: mutex is non-moveable
    TokenQuotaManager& operator=(TokenQuotaManager&&) = delete;  // Deleted: mutex is non-moveable
    


    /**
     * @brief Set Quota.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @param[in] limit Input parameter.
     */
    void setQuota(const std::string& user_id,
                  const std::string& model_id,
                  size_t limit);

    /**
     * @brief Remove Quota.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @return True when the operation succeeds.
     */
    bool removeQuota(const std::string& user_id, const std::string& model_id);

    /**
     * @brief Check.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @param[in] estimated_tokens Input parameter.
     * @return Return value.
     */
    QuotaCheckResult check(const std::string& user_id,
                           const std::string& model_id,
                           size_t estimated_tokens) const;

    /**
     * @brief Consume.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @param[in] tokens Input parameter.
     */
    void consume(const std::string& user_id,
                 const std::string& model_id,
                 size_t tokens);

    /**
     * @brief Current Usage.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    size_t currentUsage(const std::string& user_id,
                        const std::string& model_id) const;

    size_t tokensConsumed(const std::string& user_id,
                          const std::string& model_id) const
    {
        return currentUsage(user_id, model_id);
    }

    /**
     * @brief Get Limit.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    std::optional<size_t> getLimit(const std::string& user_id,
                                   const std::string& model_id) const;

    // -----------------------------------------------------------------------
    // Tenant-level quota helpers (P2.2 — Multi-tenant Isolation)
    // -----------------------------------------------------------------------

    [[nodiscard]] static std::string tenantKey(const std::string& tenant_id,
                                               const std::string& model_id)
    {
        // Prefix avoids collision with normal user IDs.
        return "__tenant__:" + tenant_id + '\0' + model_id;
    }

private:
    struct Event {
        std::chrono::steady_clock::time_point at;
        size_t tokens = 0;
    };

    struct QuotaEntry {
        size_t limit = 0;               ///< tokens per window (0 = unlimited)
        mutable std::vector<Event> events; ///< sliding-window log
    };

    /**
     * @brief Make Key.
     * @param[in] user_id Identifier of the user.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     * @details Implements makeKey without additional internal calls.
     */
    static std::string makeKey(const std::string& user_id,
                               const std::string& model_id) {
        return user_id + '\0' + model_id;
    }

    /**
     * @brief Discard events older than WINDOW.
     * @param[in,out] entry Input/output parameter.
     * @details Caller must hold mutex_.
     */
    static void prune(QuotaEntry& entry);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, QuotaEntry> entries_;
};

} // namespace llm
} // namespace themis
