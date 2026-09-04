/**
 * @file token_blacklist.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Abstract interface for JTI-based JWT token blacklist backends.
 *
 * All public methods must be thread-safe in every implementation.
 */
class ITokenBlacklist {
public:
    virtual ~ITokenBlacklist() = default;

    /**
     * @brief Add a JTI to the blacklist with its expiry time.
     *
     * @param jti    JWT ID claim of the token to revoke.
     * @param expiry Time at which the token naturally expires; implementations
     *               may use this to bound storage and auto-purge stale entries.
     */
    virtual void add(const std::string& jti,
                     std::chrono::system_clock::time_point expiry) = 0;

    /**
     * @brief Check whether a JTI is currently on the blacklist.
     *
     * @param jti JWT ID claim to check.
     * @return true if the token has been revoked and has not yet expired.
     */
    [[nodiscard]] virtual bool isRevoked(const std::string& jti) const = 0;

    /**
     * @brief Remove all entries whose token expiry is in the past.
     *
     * Safe to call at any time; implementations may be no-ops when the
     * backend handles expiry automatically (e.g. Redis TTL).
     */
    virtual void purgeExpired() = 0;
};

// ============================================================================
// TokenBlacklist — in-memory implementation of ITokenBlacklist
// ============================================================================

/**
 * @brief JTI-based JWT token blacklist for token revocation
 *
 * Stores the JTI (JWT ID) of revoked tokens together with their
 * expiry timestamps.  Expired entries are pruned automatically so
 * memory consumption stays bounded even under sustained revocation load.
 *
 * A Bloom filter pre-check on the hot path (isRevoked returning false for
 * non-revoked tokens) avoids the hash-map lookup in the common case, keeping
 * latency well within the ≤ 1 µs target for a warm filter.
 *
 * Usage:
 *   TokenBlacklist bl;
 *   // Revoke a token at logout or key-compromise:
 *   bl.revoke(claims.jti, claims.expiration);
 *
 *   // On every incoming request, after signature verification:
 *   if (bl.isRevoked(claims.jti)) { reject(); }
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Distributed deployments should back this with a shared cache (Redis)
 * and synchronise revocations across nodes.  This implementation provides
 * the single-node in-memory baseline; use RedisTokenBlacklist or
 * RocksDBTokenBlacklist for persistence and distribution.
 */
class TokenBlacklist : public ITokenBlacklist {
public:
    struct Config {
        /// How often expired entries are pruned automatically (seconds).
        uint32_t cleanup_interval_seconds = 300;
        /// Hard cap on number of stored JTIs (prevents unbounded growth).
        size_t max_entries = 1'000'000;
        static Config defaults() { return {}; }
    };

    explicit TokenBlacklist(const Config& config = Config::defaults());
    ~TokenBlacklist() = default;

    // Non-copyable, movable
    TokenBlacklist(const TokenBlacklist&) = delete;
    TokenBlacklist& operator=(const TokenBlacklist&) = delete;
    TokenBlacklist(TokenBlacklist&&) noexcept = default;
    TokenBlacklist& operator=(TokenBlacklist&&) noexcept = default;

    /**
     * @brief Attach an AuditLogger to receive TOKEN_REVOKED events.
     * Pass nullptr to detach.  The blacklist does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Callback type invoked immediately after a JTI is added to the
     *        blacklist.  Enables real-time push-style invalidation: callers
     *        can react to a revocation event without polling isRevoked().
     *
     * The callback is invoked outside the internal mutex so that it is safe to
     * call any TokenBlacklist method from within the callback body.
     *
     * Signature: void(const std::string& jti)
     */
    using RevocationCallback = std::function<void(const std::string& jti)>;

    /**
     * @brief Register a callback to be invoked synchronously when a JTI is
     *        revoked via revoke().  Only one callback can be registered at a
     *        time; a subsequent call replaces the previous one.
     *
     * @param cb  Callable to invoke on revocation.  Pass an empty function or
     *            call clearOnRevokeCallback() to detach.
     */
    void setOnRevokeCallback(RevocationCallback cb);

    /**
     * @brief Remove a previously registered revocation callback.
     */
    void clearOnRevokeCallback();

    // -----------------------------------------------------------------------
    // ITokenBlacklist interface
    // -----------------------------------------------------------------------

    /**
     * @brief Add a JTI to the blacklist (ITokenBlacklist interface).
     *
     * Equivalent to revoke(jti, expiry).
     */
    void add(const std::string& jti,
             std::chrono::system_clock::time_point expiry) override;

    /**
     * @brief Check whether a JTI is currently on the blacklist.
     *
     * Uses a Bloom filter pre-check: if the filter returns false the token is
     * definitively not revoked (≤ 1 µs for non-revoked tokens on a warm filter).
     *
     * @param jti JWT ID claim to check.
     * @return true if the token has been revoked (and has not yet expired).
     */
    bool isRevoked(const std::string& jti) const override;

    /**
     * @brief Remove all entries whose token expiry is in the past.
     *
     * Alias for pruneExpired() — satisfies the ITokenBlacklist interface.
     */
    void purgeExpired() override;

    // -----------------------------------------------------------------------
    // Extended in-memory API
    // -----------------------------------------------------------------------

    /**
     * @brief Revoke a token by its JTI (backward-compatible alias for add()).
     *
     * @param jti        JWT ID claim of the token to revoke.
     * @param expires_at Token expiry; entries are pruned after this time so the
     *                   blacklist does not grow without bound.
     */
    void revoke(const std::string& jti,
                std::chrono::system_clock::time_point expires_at);

    /**
     * @brief Remove a JTI from the blacklist (e.g. on admin un-revoke).
     *
     * @param jti JWT ID to remove.
     * @return true if the entry existed and was removed.
     */
    bool unrevoke(const std::string& jti);

    /**
     * @brief Remove all entries whose token expiry is in the past.
     *
     * Called automatically inside revoke() / isRevoked() based on
     * cleanup_interval_seconds, but can also be invoked manually.
     * The Bloom filter is rebuilt from the surviving entries after pruning.
     */
    void pruneExpired();

    /**
     * @brief Remove all entries (testing / maintenance).
     */
    void clear();

    /**
     * @brief Current number of blacklisted JTIs (including not-yet-pruned
     *        expired ones).
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

    bool needsCleanup() const;
    /// Prune expired entries and rebuild the Bloom filter (lock must be held).
    void pruneExpiredLocked();
};

} // namespace auth
} // namespace themis
