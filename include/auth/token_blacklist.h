/**
 * @file token_blacklist.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>
#include <utility>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

// ============================================================================
// ITokenBlacklist — abstract interface for JTI-based token revocation
//
// Implementations:
//   TokenBlacklist        – in-memory with Bloom filter pre-check (single node)
//   RedisTokenBlacklist   – Redis-backed for distributed deployments
//   RocksDBTokenBlacklist – RocksDB-backed for single-node persistence
// ============================================================================

class ITokenBlacklist {
public:
    /**
     * @brief IToken Blacklist.
     * @return Return value.
     */
    virtual ~ITokenBlacklist() = default;

    /**
     * @brief Add.
     * @param[in] jti Input parameter.
     * @param[in] expiry Input parameter.
     */
    virtual void add(const std::string& jti,
                     std::chrono::system_clock::time_point expiry) = 0;

    [[nodiscard]] virtual bool isRevoked(const std::string& jti) const = 0;

    /**
     * @brief Purge Expired.
     */
    virtual void purgeExpired() = 0;
};

// ============================================================================
// TokenBlacklist — in-memory implementation of ITokenBlacklist
// ============================================================================

class TokenBlacklist : public ITokenBlacklist {
public:
    struct Config {
        uint32_t cleanup_interval_seconds = 300;
        size_t max_entries = 1'000'000;
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };

    explicit TokenBlacklist(const Config& config = Config::defaults());
    ~TokenBlacklist() = default;

    // Non-copyable, non-movable
    TokenBlacklist(const TokenBlacklist&) = delete;
    TokenBlacklist& operator=(const TokenBlacklist&) = delete;
    TokenBlacklist(TokenBlacklist&&) noexcept = delete;
    TokenBlacklist& operator=(TokenBlacklist&&) noexcept = delete;

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    using RevocationCallback = std::function<void(const std::string& jti)>;

    /**
     * @brief Set On Revoke Callback.
     * @param[in] cb Input parameter.
     */
    void setOnRevokeCallback(RevocationCallback cb);

    /**
     * @brief Clear On Revoke Callback.
     */
    void clearOnRevokeCallback();

    // -----------------------------------------------------------------------
    // ITokenBlacklist interface
    // -----------------------------------------------------------------------

    void add(const std::string& jti,
             std::chrono::system_clock::time_point expiry) override;

    bool isRevoked(const std::string& jti) const override;

    void purgeExpired() override;

    // -----------------------------------------------------------------------
    // Extended in-memory API
    // -----------------------------------------------------------------------

    /**
     * @brief Revoke.
     * @param[in] jti Input parameter.
     * @param[in] expires_at Input parameter.
     */
    void revoke(const std::string& jti,
                std::chrono::system_clock::time_point expires_at);

    /**
     * @brief Unrevoke.
     * @param[in] jti Input parameter.
     * @return True when the operation succeeds.
     */
    bool unrevoke(const std::string& jti);

    /**
     * @brief Prune Expired.
     */
    void pruneExpired();

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;

    struct Statistics {
        size_t current_size          = 0;
        uint64_t total_revocations   = 0;
        uint64_t total_checks        = 0;
        uint64_t revoked_hits        = 0;  ///< isRevoked() returned true
        uint64_t pruned_entries      = 0;
        uint64_t bloom_negatives     = 0;  ///< isRevoked() short-circuited by Bloom filter
    };

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;

private:
    Config config_;

    struct Entry {
        std::chrono::system_clock::time_point expires_at;
    };

    // -----------------------------------------------------------------------
    // Hand-rolled Bloom filter for O(1) non-revoked token fast-path.
    //
    // Uses double-hashing (FNV-1a + djb2) with kNumHashes probes.
    // False positives cause a fall-through to the hash-map; false negatives
    // are impossible.  The filter is rebuilt after pruneExpired() / clear()
    // so it stays reasonably accurate.
    // -----------------------------------------------------------------------
    struct BloomFilter {
        static constexpr size_t kBitsPerEntry = 10;  ///< ~1 % false-positive rate
        static constexpr size_t kNumHashes    = 7;   ///< optimal for 10 bits/entry

        /**
         * @brief Bloom Filter.
         * @param[in] capacity Input parameter.
         * @return Return value.
         */
        explicit BloomFilter(size_t capacity)
            : bit_count_(std::max<size_t>(64, capacity * kBitsPerEntry))
            , bits_((bit_count_ + 7) / 8, 0)
        {}

        void add(const std::string& key) noexcept {
            auto [h1, h2] = hashes(key);
            for (size_t i = 0; i < kNumHashes; ++i) {
                size_t bit = (h1 + i * h2) % bit_count_;
                bits_[bit >> 3] |= static_cast<uint8_t>(1u << (bit & 7u));
            }
        }

        bool mayContain(const std::string& key) const noexcept {
            auto [h1, h2] = hashes(key);
            for (size_t i = 0; i < kNumHashes; ++i) {
                size_t bit = (h1 + i * h2) % bit_count_;
                if (!(bits_[bit >> 3] & (1u << (bit & 7u)))) {
                  return false;
                }
            }
            return true;
        }

        void reset() noexcept {
            std::fill(bits_.begin(), bits_.end(), uint8_t{0});
        }

    private:
        static std::pair<size_t, size_t> hashes(const std::string& key) noexcept {
            // FNV-1a 64-bit
            constexpr uint64_t kOffset = 14695981039346656037ULL;
            constexpr uint64_t kPrime  = 1099511628211ULL;
            uint64_t h1 = kOffset;
            for (unsigned char c : key) { h1 ^= c; h1 *= kPrime; }
            // djb2 for the second hash (forced odd to cover all bit positions)
            uint64_t h2 = 5381;
            for (unsigned char c : key) { h2 = ((h2 << 5) + h2) ^ c; }
            return {static_cast<size_t>(h1), static_cast<size_t>(h2 | 1u)};
        }

        size_t              bit_count_;
        std::vector<uint8_t> bits_;
    };

    std::unordered_map<std::string, Entry> blacklist_;
    BloomFilter                             bloom_;

    mutable Statistics stats_;
    mutable std::mutex mutex_;
    std::chrono::steady_clock::time_point last_cleanup_;
    utils::AuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.
    RevocationCallback on_revoke_callback_;      ///< Invoked outside mutex on revoke.

    /**
     * @brief Needs Cleanup.
     * @return True when the operation succeeds.
     */
    bool needsCleanup() const;
    /**
     * @brief Prune Expired Locked.
     */
    void pruneExpiredLocked();
};

} // namespace auth
} // namespace themis
