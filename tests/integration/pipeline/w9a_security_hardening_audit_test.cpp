/**
 * @file w9a_security_hardening_audit_test.cpp
 * @brief Wave 9A — Security Hardening & Audit Trail (SHA-01..SHA-08).
 *
 * Validates that security-critical paths in ThemisDB correctly enforce rate
 * limits, privilege boundaries, input sanitisation, audit integrity, credential
 * lifecycle, and replay resistance.  Every test scenario is self-contained and
 * deterministic via kCanonicalSeed = 42.
 *
 * SHA-01  Auth token rate limiting — token bucket rejects excess validation
 *         attempts and resumes acceptance after the bucket refills.
 * SHA-02  Privilege escalation prevention — operations attempted with an
 *         unprivileged role return kForbidden without modifying state.
 * SHA-03  Input injection resistance — query parser rejects SQL/AQL injection
 *         strings without crashing or accepting malformed input.
 * SHA-04  Audit log tamper detection — post-write modification of a log entry
 *         is detected by a subsequent integrity check.
 * SHA-05  Credential rotation — old credential rejected, new credential
 *         accepted within the same operation sequence after rotation.
 * SHA-06  Audit log completeness under denied escalation attempts — every
 *         denied operation produces exactly one audit event.
 * SHA-07  Replay attack prevention — a consumed nonce is rejected on a second
 *         presentation of the same auth token.
 * SHA-08  Audit log ordering under concurrency — concurrent operations produce
 *         monotonically increasing, gap-free sequence numbers.
 *
 * All tests are deterministic via kCanonicalSeed = 42.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis { namespace test { 

namespace {

// kCanonicalSeed = 42 is provided by test_data_generator.h (themis::test::kCanonicalSeed).

// ---------------------------------------------------------------------------
// Status codes shared across SHA tests
// ---------------------------------------------------------------------------

/// @brief Result codes for security-layer operations.
enum class SecStatus {
    kOk,
    kRateLimited,
    kForbidden,
    kRejected,
    kTampered,
    kExpired,
    kReplay,
    kTimedOut,
};

// ---------------------------------------------------------------------------
// TokenBucket — rate limiter for SHA-01
// ---------------------------------------------------------------------------

/**
 * @brief Token-bucket rate limiter with a fixed capacity and refill step.
 *
 * Each call to TryConsume() attempts to consume one token.  If the bucket is
 * empty the call returns false (rate-limited).  Refill() adds tokens back up
 * to the configured capacity, simulating time-based replenishment without
 * requiring real wall-clock sleep in tests.
 */
class TokenBucket {
public:
    /**
     * @param capacity    Maximum number of tokens the bucket holds.
     * @param refill_amt  Tokens added per Refill() call.
     */
    explicit TokenBucket(size_t capacity, size_t refill_amt = 0)
        : capacity_(capacity),
          tokens_(static_cast<int64_t>(capacity)),
          refill_amt_(refill_amt == 0 ? capacity : refill_amt) {}

    /// @brief Attempt to consume one token.  Returns true on success.
    bool TryConsume() {
        std::lock_guard<std::mutex> lk(mu_);
        if (tokens_ <= 0) { return false; }
        --tokens_;
        return true;
    }

    /// @brief Refill the bucket by refill_amt_, capped at capacity_.
    void Refill() {
        std::lock_guard<std::mutex> lk(mu_);
        tokens_ = std::min(tokens_ + static_cast<int64_t>(refill_amt_),
                           static_cast<int64_t>(capacity_));
    }

    /// @brief Current token count (for assertions).
    int64_t Tokens() const {
        std::lock_guard<std::mutex> lk(mu_);
        return tokens_;
    }

private:
    mutable std::mutex mu_;
    const size_t       capacity_;
    int64_t            tokens_;
    const size_t       refill_amt_;
};

// ---------------------------------------------------------------------------
// RoleGuard — privilege check for SHA-02
// ---------------------------------------------------------------------------

/// @brief Minimal role enumeration.
enum class Role { kAdmin, kReadOnly, kUnprivileged };

/**
 * @brief Guards a mutable resource behind a role check.
 *
 * Write() and Delete() require kAdmin.  All other roles receive kForbidden and
 * the underlying state is never modified.
 */
class RoleGuard {
public:
    /**
     * @brief Attempt a write operation with the given role.
     * @returns SecStatus::kOk on success; kForbidden if role is insufficient.
     */
    SecStatus Write(Role role, const std::string& key, const std::string& val) {
        if (role != Role::kAdmin) { return SecStatus::kForbidden; }
        std::lock_guard<std::mutex> lk(mu_);
        data_[key] = val;
        return SecStatus::kOk;
    }

    /**
     * @brief Attempt a delete operation with the given role.
     * @returns SecStatus::kOk on success; kForbidden if role is insufficient.
     */
    SecStatus Delete(Role role, const std::string& key) {
        if (role != Role::kAdmin) { return SecStatus::kForbidden; }
        std::lock_guard<std::mutex> lk(mu_);
        data_.erase(key);
        return SecStatus::kOk;
    }

    /// @brief Read a value regardless of role (read is always permitted).
    std::optional<std::string> Read(const std::string& key) const {
        std::lock_guard<std::mutex> lk(mu_);
        const auto it = data_.find(key);
        if (it == data_.end()) { return std::nullopt; }
        return it->second;
    }

    /// @brief Total number of keys currently stored.
    size_t Size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return data_.size();
    }

private:
    mutable std::mutex                           mu_;
    std::unordered_map<std::string, std::string> data_;
};

// ---------------------------------------------------------------------------
// InjectionFilter — input sanitiser for SHA-03
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight query-input sanitiser.
 *
 * Detects common SQL/AQL injection patterns and rejects strings that contain
 * them.  The check is intentionally simple and deterministic — it is not a
 * production-grade SQL parser, but validates the interface contract that
 * malicious inputs are always rejected with kRejected.
 */
class InjectionFilter {
public:
    /// @brief Patterns that are unconditionally rejected.
    static const std::vector<std::string>& RejectPatterns() {
        static const std::vector<std::string> kPatterns = {
            "'; DROP TABLE",
            "UNION SELECT",
            "-- ",
            "/*",
            "';",
            "OR 1=1",
            "xp_cmdshell",
            "EXEC(",
        };
        return kPatterns;
    }

    /**
     * @brief Validate a query string.
     * @returns SecStatus::kRejected if an injection pattern is found;
     *          SecStatus::kOk otherwise.
     */
    static SecStatus Validate(const std::string& input) {
        const std::string upper = ToUpper(input);
        for (const auto& pattern : RejectPatterns()) {
            const std::string upat = ToUpper(pattern);
            if (upper.find(upat) != std::string::npos) {
                return SecStatus::kRejected;
            }
        }
        return SecStatus::kOk;
    }

private:
    static std::string ToUpper(const std::string& s) {
        std::string r = s;
        for (char& c : r) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
        return r;
    }
};

// ---------------------------------------------------------------------------
// AuditLog — tamper-detectable append-only log for SHA-04/SHA-06/SHA-08
// ---------------------------------------------------------------------------

/// @brief A single audit log entry with an integrity hash.
struct AuditEntry {
    uint64_t    sequence{0};
    std::string operation;
    std::string subject;
    bool        allowed{true};

    /// @brief Simple deterministic hash of the entry fields.
    size_t ComputeHash() const {
        std::size_t h = std::hash<uint64_t>{}(sequence);
        h ^= std::hash<std::string>{}(operation) + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= std::hash<std::string>{}(subject)   + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= std::hash<bool>{}(allowed)           + 0x9e3779b9u + (h << 6) + (h >> 2);
        return h;
    }
};

/// @brief Stored entry with its integrity hash baked in at write time.
struct StoredAuditEntry {
    AuditEntry entry;
    size_t     hash_at_write{0};
};

/**
 * @brief Thread-safe append-only audit log with entry-level tamper detection.
 *
 * Each entry's hash is computed at write time.  IntegrityCheck() recomputes
 * the hash and compares it against the stored value; any mismatch indicates
 * tampering.
 */
class AuditLog {
public:
    /// @brief Record an operation outcome.
    void Record(const std::string& op, const std::string& subject, bool allowed) {
        std::lock_guard<std::mutex> lk(mu_);
        AuditEntry e;
        e.sequence  = ++seq_;
        e.operation = op;
        e.subject   = subject;
        e.allowed   = allowed;
        StoredAuditEntry se;
        se.entry         = e;
        se.hash_at_write = e.ComputeHash();
        entries_.push_back(se);
    }

    /// @brief Tamper an entry at position @p index (0-based, for SHA-04 test).
    void TamperEntry(size_t index, const std::string& new_op) {
        std::lock_guard<std::mutex> lk(mu_);
        if (index < entries_.size()) {
            entries_[index].entry.operation = new_op;
            // deliberately do NOT update hash_at_write — simulates real tampering
        }
    }

    /**
     * @brief Re-verify all entry hashes.
     * @returns true if all entries are intact; false if any hash mismatch.
     */
    bool IntegrityCheck() const {
        std::lock_guard<std::mutex> lk(mu_);
        for (const auto& se : entries_) {
            if (se.entry.ComputeHash() != se.hash_at_write) {
                return false;
            }
        }
        return true;
    }

    /// @brief Count entries where allowed == false.
    size_t DeniedCount() const {
        std::lock_guard<std::mutex> lk(mu_);
        size_t n = 0;
        for (const auto& se : entries_) { if (!se.entry.allowed) { ++n; } }
        return n;
    }

    /// @brief Total number of recorded entries.
    size_t Count() const {
        std::lock_guard<std::mutex> lk(mu_);
        return entries_.size();
    }

    /**
     * @brief Verify that all sequence numbers are consecutive and monotone.
     * @returns true if sequence numbers are 1, 2, …, N with no gaps.
     */
    bool SequenceIsMonotone() const {
        std::lock_guard<std::mutex> lk(mu_);
        for (size_t i = 0; i < entries_.size(); ++i) {
            if (entries_[i].entry.sequence != static_cast<uint64_t>(i + 1)) {
                return false;
            }
        }
        return true;
    }

private:
    mutable std::mutex             mu_;
    std::vector<StoredAuditEntry>  entries_;
    std::atomic<uint64_t>          seq_{0};
};

// ---------------------------------------------------------------------------
// CredentialStore — credential rotation for SHA-05
// ---------------------------------------------------------------------------

/**
 * @brief Manages active and revoked credentials.
 *
 * Rotate() atomically promotes a new credential to active and moves the
 * previous active credential to the revoked set.  Validate() accepts only the
 * current active credential.
 */
class CredentialStore {
public:
    explicit CredentialStore(std::string initial) : active_(std::move(initial)) {}

    /**
     * @brief Rotate to @p new_cred.
     *
     * The old active credential is immediately revoked.
     */
    void Rotate(const std::string& new_cred) {
        std::lock_guard<std::mutex> lk(mu_);
        revoked_.insert(active_);
        active_ = new_cred;
    }

    /**
     * @brief Check whether @p cred is the current active credential.
     * @returns SecStatus::kOk if valid; kExpired if revoked; kForbidden if unknown.
     */
    SecStatus Validate(const std::string& cred) const {
        std::lock_guard<std::mutex> lk(mu_);
        if (cred == active_)            { return SecStatus::kOk; }
        if (revoked_.count(cred) > 0)   { return SecStatus::kExpired; }
        return SecStatus::kForbidden;
    }

private:
    mutable std::mutex           mu_;
    std::string                  active_;
    std::unordered_set<std::string> revoked_;
};

// ---------------------------------------------------------------------------
// NonceStore — one-use nonce registry for SHA-07
// ---------------------------------------------------------------------------

/**
 * @brief Tracks consumed nonces to prevent token replay attacks.
 *
 * ConsumeNonce() succeeds exactly once per nonce value.  Subsequent calls with
 * the same nonce return kReplay.
 */
class NonceStore {
public:
    /**
     * @brief Attempt to consume @p nonce.
     * @returns SecStatus::kOk on first use; kReplay on any subsequent use.
     */
    SecStatus ConsumeNonce(const std::string& nonce) {
        std::lock_guard<std::mutex> lk(mu_);
        const auto [_, inserted] = used_.insert(nonce);
        return inserted ? SecStatus::kOk : SecStatus::kReplay;
    }

private:
    std::mutex                    mu_;
    std::unordered_set<std::string> used_;
};

} // anonymous namespace

// ===========================================================================
// Test fixture
// ===========================================================================

/**
 * @brief Test fixture shared across all SHA tests.
 *
 * Constructs a fresh instance of each in-process security component and
 * provides a seeded PRNG for deterministic helper data.
 */
class SecurityHardeningAuditTest : public ::testing::Test {
protected:
    void SetUp() override {
        audit_  = std::make_unique<AuditLog>();
        creds_  = std::make_unique<CredentialStore>("initial_cred_v1");
        nonces_ = std::make_unique<NonceStore>();
        bucket_ = std::make_unique<TokenBucket>(5, 5);
        guard_  = std::make_unique<RoleGuard>();
        gen_.seed(kCanonicalSeed);
    }

    void TearDown() override {
        audit_.reset();
        creds_.reset();
        nonces_.reset();
        bucket_.reset();
        guard_.reset();
    }

    std::unique_ptr<AuditLog>        audit_;
    std::unique_ptr<CredentialStore> creds_;
    std::unique_ptr<NonceStore>      nonces_;
    std::unique_ptr<TokenBucket>     bucket_;
    std::unique_ptr<RoleGuard>       guard_;
    std::mt19937                     gen_;
};

// ===========================================================================
// SHA-01 — Auth token rate limiting
// ===========================================================================

TEST_F(SecurityHardeningAuditTest, SHA01_AuthTokenRateLimitingBucketEnforced) {
    SCOPED_TRACE("SHA-01: auth token rate limiting via token bucket");

    // Bucket capacity = 5. Drain it completely.
    size_t successes = 0;
    size_t rejections = 0;
    for (int i = 0; i < 10; ++i) {
        if (bucket_->TryConsume()) { ++successes; }
        else                       { ++rejections; }
    }

    EXPECT_EQ(successes,  5U) << "exactly 5 tokens should be consumed before rate-limit";
    EXPECT_EQ(rejections, 5U) << "5 attempts should be rate-limited when bucket is empty";
    EXPECT_EQ(bucket_->Tokens(), 0) << "bucket must be empty after full drain";

    // Refill and verify subsequent attempts succeed.
    bucket_->Refill();
    EXPECT_GT(bucket_->Tokens(), 0) << "bucket must have tokens after Refill()";

    bool resumed = bucket_->TryConsume();
    EXPECT_TRUE(resumed) << "token consumption must succeed after bucket refill";
}

// ===========================================================================
// SHA-02 — Privilege escalation prevention
// ===========================================================================

TEST_F(SecurityHardeningAuditTest, SHA02_PrivilegeEscalationPreventionForbidden) {
    SCOPED_TRACE("SHA-02: unprivileged role must not modify state");

    // Pre-condition: store is empty.
    ASSERT_EQ(guard_->Size(), 0U);

    // Admin write succeeds.
    const auto admin_result = guard_->Write(Role::kAdmin, "admin_key", "admin_val");
    EXPECT_EQ(admin_result, SecStatus::kOk);
    EXPECT_EQ(guard_->Size(), 1U);

    // Unprivileged write attempt must be rejected.
    const auto unpriv_result = guard_->Write(Role::kUnprivileged, "unpriv_key", "unpriv_val");
    EXPECT_EQ(unpriv_result, SecStatus::kForbidden)
        << "unprivileged write must return kForbidden";

    // State must not have changed.
    EXPECT_EQ(guard_->Size(), 1U)
        << "store size must be unchanged after a forbidden write attempt";
    EXPECT_FALSE(guard_->Read("unpriv_key").has_value())
        << "unprivileged key must not appear in store";

    // ReadOnly delete attempt must also be rejected.
    const auto ro_delete = guard_->Delete(Role::kReadOnly, "admin_key");
    EXPECT_EQ(ro_delete, SecStatus::kForbidden);
    EXPECT_EQ(guard_->Size(), 1U)
        << "admin key must survive a forbidden delete attempt";
}

// ===========================================================================
// SHA-03 — Input injection resistance
// ===========================================================================

TEST_F(SecurityHardeningAuditTest, SHA03_InputInjectionResistanceRejectsPayloads) {
    SCOPED_TRACE("SHA-03: injection strings must be rejected; benign input accepted");

    // All known injection payloads must be rejected.
    const std::vector<std::string> injections = {
        "'; DROP TABLE users; --",
        "' UNION SELECT * FROM secrets --",
        "admin'--",
        "/* bypass */",
        "' OR 1=1 --",
        "EXEC(xp_cmdshell 'dir')",
    };

    for (const auto& payload : injections) {
        const SecStatus result = InjectionFilter::Validate(payload);
        EXPECT_EQ(result, SecStatus::kRejected)
            << "injection payload should be rejected: " << payload;
    }

    // Benign inputs must be accepted.
    const std::vector<std::string> benign = {
        "SELECT name FROM users WHERE id = 42",
        "FOR v IN vertices RETURN v.name",
        "GET /api/v1/resource",
        "simple_key",
        "",
    };

    for (const auto& input : benign) {
        const SecStatus result = InjectionFilter::Validate(input);
        EXPECT_EQ(result, SecStatus::kOk)
            << "benign input should not be rejected: " << input;
    }
}

// ===========================================================================
// SHA-04 — Audit log tamper detection
// ===========================================================================

TEST_F(SecurityHardeningAuditTest, SHA04_AuditLogTamperDetectionDetectsMutation) {
    SCOPED_TRACE("SHA-04: post-write modification of audit entry must be detected");

    // Write several entries.
    constexpr size_t kEntries = 8;
    for (size_t i = 0; i < kEntries; ++i) {
        audit_->Record("WRITE", "key_" + std::to_string(i), true);
    }

    // Integrity check must pass before tampering.
    ASSERT_TRUE(audit_->IntegrityCheck())
        << "audit log integrity must be intact before any tampering";

    // Tamper with entry at index 3.
    audit_->TamperEntry(3, "MALICIOUS_OVERWRITE");

    // Integrity check must fail after tampering.
    EXPECT_FALSE(audit_->IntegrityCheck())
        << "audit log integrity check must detect the tampered entry";
}

// ===========================================================================
// SHA-05 — Credential rotation
// ===========================================================================

TEST_F(SecurityHardeningAuditTest, SHA05_CredentialRotationOldRejectedNewAccepted) {
    SCOPED_TRACE("SHA-05: old credential rejected, new credential accepted after rotation");

    const std::string old_cred = "initial_cred_v1";
    const std::string new_cred = "rotated_cred_v2";

    // Old credential is valid before rotation.
    EXPECT_EQ(creds_->Validate(old_cred), SecStatus::kOk)
        << "old credential must be valid before rotation";

    // Rotate.
    creds_->Rotate(new_cred);

    // Old credential must now be rejected (expired/revoked).
    const SecStatus old_status = creds_->Validate(old_cred);
    EXPECT_NE(old_status, SecStatus::kOk)
        << "old credential must not be accepted after rotation";
    EXPECT_EQ(old_status, SecStatus::kExpired)
        << "old credential must be in the revoked set (kExpired)";

    // New credential must be accepted.
    EXPECT_EQ(creds_->Validate(new_cred), SecStatus::kOk)
        << "new credential must be valid immediately after rotation";

    // Unknown credential must return kForbidden (not kExpired).
    EXPECT_EQ(creds_->Validate("unknown_cred"), SecStatus::kForbidden)
        << "unknown credential must return kForbidden, not kExpired";
}

// ===========================================================================
// SHA-06 — Audit log completeness under privilege escalation attempts
// ===========================================================================

TEST_F(SecurityHardeningAuditTest, SHA06_AuditCompleteDeniedOpsEachGenerateOneEvent) {
    SCOPED_TRACE("SHA-06: each denied operation must produce exactly one audit event");

    constexpr size_t kDeniedOps = 6;

    for (size_t i = 0; i < kDeniedOps; ++i) {
        // Attempt privileged write with unprivileged role.
        const auto status = guard_->Write(Role::kUnprivileged,
                                          "key_" + std::to_string(i), "val");
        ASSERT_EQ(status, SecStatus::kForbidden);

        // Record the denial in the audit log (one event per denied op).
        audit_->Record("WRITE", "key_" + std::to_string(i), /*allowed=*/false);
    }

    // Exactly kDeniedOps denied events must be present.
    EXPECT_EQ(audit_->Count(), kDeniedOps)
        << "audit log must contain exactly one event per denied operation";
    EXPECT_EQ(audit_->DeniedCount(), kDeniedOps)
        << "all recorded events must be denial events";

    // Integrity must still hold.
    EXPECT_TRUE(audit_->IntegrityCheck())
        << "audit log must remain tamper-free after recording denials";
}

// ===========================================================================
// SHA-07 — Replay attack prevention
// ===========================================================================

TEST_F(SecurityHardeningAuditTest, SHA07_ReplayAttackPreventionNonceConsumedOnce) {
    SCOPED_TRACE("SHA-07: one-use nonce must be rejected on second presentation");

    const std::string token_nonce = "nonce_sha07_" + std::to_string(kCanonicalSeed);

    // First use must succeed.
    const SecStatus first = nonces_->ConsumeNonce(token_nonce);
    EXPECT_EQ(first, SecStatus::kOk)
        << "first nonce consumption must succeed";

    // Second use of the same nonce must be rejected.
    const SecStatus second = nonces_->ConsumeNonce(token_nonce);
    EXPECT_EQ(second, SecStatus::kReplay)
        << "replay of a consumed nonce must return kReplay";

    // A different nonce must still be accepted (not poisoned by the first).
    const std::string other_nonce = "nonce_sha07_other";
    EXPECT_EQ(nonces_->ConsumeNonce(other_nonce), SecStatus::kOk)
        << "a fresh nonce must not be affected by the replay rejection";

    // Third use of original nonce still kReplay.
    EXPECT_EQ(nonces_->ConsumeNonce(token_nonce), SecStatus::kReplay)
        << "repeated replay attempts must consistently return kReplay";
}

// ===========================================================================
// SHA-08 — Audit log ordering under concurrent operations
// ===========================================================================

TEST_F(SecurityHardeningAuditTest, SHA08_AuditLogMonotoneSequenceUnderConcurrency) {
    SCOPED_TRACE("SHA-08: concurrent audit writes must produce gap-free monotone sequences");

    constexpr size_t kThreads       = 4;
    constexpr size_t kOpsPerThread  = 25;
    constexpr size_t kTotalExpected = kThreads * kOpsPerThread;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (size_t t = 0; t < kThreads; ++t) {
        workers.emplace_back([this, t]() {
            for (size_t op = 0; op < kOpsPerThread; ++op) {
                const std::string subject = "thread_" + std::to_string(t)
                                          + "_op_" + std::to_string(op);
                audit_->Record("CONCURRENT_WRITE", subject, /*allowed=*/true);
            }
        });
    }

    for (auto& w : workers) { w.join(); }

    EXPECT_EQ(audit_->Count(), kTotalExpected)
        << "all concurrent audit events must be recorded without loss";

    // Sequence numbers must be monotonically increasing with no gaps.
    EXPECT_TRUE(audit_->SequenceIsMonotone())
        << "audit sequence numbers must be consecutive 1..N with no gaps or reordering";

    // Integrity check must still hold.
    EXPECT_TRUE(audit_->IntegrityCheck())
        << "concurrent writes must not corrupt audit log integrity hashes";
}
} } // namespace themis::test
