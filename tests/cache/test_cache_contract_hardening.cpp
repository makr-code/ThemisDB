// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "cache/cache_contract.h"
#include "cache/adaptive_query_cache.h"

using namespace themis::cache;
using themis::AdaptiveQueryCache;

// ─────────────────────────────────────────────────────────────────────────────
// Shared helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Minimal AdaptiveQueryCache config with L3 disabled (no RocksDB dependency).
static AdaptiveQueryCache::Config makeContractConfig() {
    AdaptiveQueryCache::Config cfg;
    cfg.l1_max_entries         = 64;
    cfg.l1_max_entry_size      = 65536;
    cfg.l2_max_entries         = 128;
    cfg.l2_max_entry_size      = 65536;
    cfg.l3_db_path             = "./cache_contract_hardening_l3";
    cfg.enable_circuit_breaker = false;
    cfg.enable_size_limits     = true;
    cfg.max_total_entry_size   = 67108864;              // 64 MiB hard cap
    cfg.enable_adaptive_ttl    = false;
    cfg.enable_rate_limiting   = false;
    cfg.enable_tenant_isolation = false;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// CCH-01: Key size constraint — kMaxCacheKeyBytes is positive and sane
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheContractHardening, CCH01_KeySizeConstraintIsSane) {
    // The key limit must be above any expected SHA-256 hex fingerprint length
    // (64 bytes) but bounded to prevent memory-exhaustion at inbound API.
    EXPECT_GT(kMaxCacheKeyBytes, std::size_t{64})
        << "Key limit must accommodate a full SHA-256 hex fingerprint";
    EXPECT_LE(kMaxCacheKeyBytes, std::size_t{65536})
        << "Key limit must be bounded to prevent memory-exhaustion";
    // Tenant prefix headroom: limit must leave room for at least
    // kMaxTenantIdBytes overhead.
    EXPECT_GE(kMaxCacheKeyBytes, kMaxTenantIdBytes + 64)
        << "Key limit must accommodate tenant prefix + fingerprint";
}

// ─────────────────────────────────────────────────────────────────────────────
// CCH-02: Value size constraint — kMaxCacheValueBytes is positive and sane
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheContractHardening, CCH02_ValueSizeConstraintIsSane) {
    // At least 1 MiB: covers any realistic JSON query result.
    EXPECT_GE(kMaxCacheValueBytes, std::size_t{1024 * 1024})
        << "Value limit must accommodate realistic JSON query results";
    // Hard cap to prevent a single entry consuming unreasonable memory.
    EXPECT_LE(kMaxCacheValueBytes, std::size_t{4ULL * 1024 * 1024 * 1024})
        << "Value limit must not allow single 4+ GiB entries";
}

// ─────────────────────────────────────────────────────────────────────────────
// CCH-03: Fail-closed classification — DegradedBackend and InternalError
//         are always fail-closed; other classes are not.
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheContractHardening, CCH03_FailClosedClassPredicate) {
    // Fail-closed classes: deny without fallback.
    EXPECT_TRUE(isFailClosedClass(CacheFailureClass::DegradedBackend))
        << "DegradedBackend must be fail-closed (§4)";
    EXPECT_TRUE(isFailClosedClass(CacheFailureClass::InternalError))
        << "InternalError must be fail-closed (§4)";

    // Non-fail-closed classes: may be handled with structured response.
    EXPECT_FALSE(isFailClosedClass(CacheFailureClass::MalformedKey));
    EXPECT_FALSE(isFailClosedClass(CacheFailureClass::MalformedValue));
    EXPECT_FALSE(isFailClosedClass(CacheFailureClass::InvalidOperation));
    EXPECT_FALSE(isFailClosedClass(CacheFailureClass::TenantViolation));
    EXPECT_FALSE(isFailClosedClass(CacheFailureClass::PartialDelivery));
}

// ─────────────────────────────────────────────────────────────────────────────
// CCH-04: Fail-closed predicate exhaustiveness — every enum value is covered
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheContractHardening, CCH04_FailClosedPredicateExhaustive) {
    // Enumerate all defined failure classes; classify each.
    constexpr CacheFailureClass all_classes[] = {
        CacheFailureClass::MalformedKey,
        CacheFailureClass::MalformedValue,
        CacheFailureClass::InvalidOperation,
        CacheFailureClass::TenantViolation,
        CacheFailureClass::DegradedBackend,
        CacheFailureClass::PartialDelivery,
        CacheFailureClass::InternalError,
    };

    int fail_closed_count = 0;
    int fail_open_count   = 0;
    for (auto fc : all_classes) {
        if (isFailClosedClass(fc)) {
            ++fail_closed_count;
        } else {
            ++fail_open_count;
        }
    }

    // Exactly 2 fail-closed classes are defined in §3/§4.
    EXPECT_EQ(fail_closed_count, 2)
        << "Exactly DegradedBackend and InternalError must be fail-closed";
    EXPECT_EQ(fail_open_count, 5)
        << "Remaining 5 classes must NOT be fail-closed";
}

// ─────────────────────────────────────────────────────────────────────────────
// CCH-05: Malformed input predicate — MalformedKey and MalformedValue only
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheContractHardening, CCH05_MalformedInputPredicate) {
    EXPECT_TRUE(isMalformedInputClass(CacheFailureClass::MalformedKey));
    EXPECT_TRUE(isMalformedInputClass(CacheFailureClass::MalformedValue));

    EXPECT_FALSE(isMalformedInputClass(CacheFailureClass::InvalidOperation));
    EXPECT_FALSE(isMalformedInputClass(CacheFailureClass::TenantViolation));
    EXPECT_FALSE(isMalformedInputClass(CacheFailureClass::DegradedBackend));
    EXPECT_FALSE(isMalformedInputClass(CacheFailureClass::PartialDelivery));
    EXPECT_FALSE(isMalformedInputClass(CacheFailureClass::InternalError));
}

// ─────────────────────────────────────────────────────────────────────────────
// CCH-06: CoordinatorCapability OR/AND/hasCoordinatorCapability semantics
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheContractHardening, CCH06_CoordinatorCapabilitySemantics) {
    // None is a zero bitmask.
    EXPECT_EQ(static_cast<unsigned int>(CoordinatorCapability::None), 0u);

    // Composite capability OR.
    const auto redis_tls = CoordinatorCapability::Redis | CoordinatorCapability::TLS;
    EXPECT_TRUE(hasCoordinatorCapability(redis_tls, CoordinatorCapability::Redis));
    EXPECT_TRUE(hasCoordinatorCapability(redis_tls, CoordinatorCapability::TLS));
    EXPECT_FALSE(hasCoordinatorCapability(redis_tls, CoordinatorCapability::gRPC));
    EXPECT_FALSE(hasCoordinatorCapability(redis_tls, CoordinatorCapability::LocalBus));

    // None satisfies nothing (except None itself via empty intersection).
    const auto none_set = CoordinatorCapability::None;
    EXPECT_FALSE(hasCoordinatorCapability(none_set, CoordinatorCapability::Redis));
    EXPECT_FALSE(hasCoordinatorCapability(none_set, CoordinatorCapability::gRPC));

    // All four flags are distinct (no aliasing).
    const auto all_flags = CoordinatorCapability::Redis
        | CoordinatorCapability::gRPC
        | CoordinatorCapability::LocalBus
        | CoordinatorCapability::TLS;
    EXPECT_TRUE(hasCoordinatorCapability(all_flags, CoordinatorCapability::Redis));
    EXPECT_TRUE(hasCoordinatorCapability(all_flags, CoordinatorCapability::gRPC));
    EXPECT_TRUE(hasCoordinatorCapability(all_flags, CoordinatorCapability::LocalBus));
    EXPECT_TRUE(hasCoordinatorCapability(all_flags, CoordinatorCapability::TLS));
}

// ─────────────────────────────────────────────────────────────────────────────
// CCH-07: Temporal constant ordering
//         The contract defines three latency bounds; their relative order must
//         be sensible and non-zero.
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheContractHardening, CCH07_TemporalConstantOrdering) {
    // All bounds must be positive.
    EXPECT_GT(kMaxL3RetrievalLatency.count(), 0LL)
        << "L3 retrieval latency bound must be positive";
    EXPECT_GT(kMaxCoordinatorTransitionTime.count(), 0LL)
        << "Coordinator transition time must be positive";
    EXPECT_GT(kMaxReplicationDeliveryMs.count(), 0LL)
        << "Replication delivery bound must be positive";

    // Async replication default must not exceed the hard maximum.
    EXPECT_LE(kAsyncReplicationDefaultTimeout.count(),
              kAsyncReplicationMaxTimeout.count())
        << "Default async timeout must not exceed the configured hard maximum";

    // Minimum retry backoff must be positive and smaller than the transition time.
    EXPECT_GT(kCoordinatorMinRetryBackoff.count(), 0LL);
    EXPECT_LT(kCoordinatorMinRetryBackoff,
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  kMaxCoordinatorTransitionTime))
        << "Min retry backoff must be smaller than the transition-time budget";

    // The staleness window must be positive.
    EXPECT_GT(kCoordinatorCacheMaxStaleness.count(), 0LL);
}

// ─────────────────────────────────────────────────────────────────────────────
// CCH-08: AdaptiveQueryCache round-trip compliant with contract
//         put() → get() succeeds for a valid entry within size bounds.
//         L3 is disabled; test verifies L1/L2 path complies with §1/§2.
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheContractHardening, CCH08_AdaptiveQueryCacheRoundTrip) {
    AdaptiveQueryCache cache(makeContractConfig());

    // A fingerprint well within kMaxCacheKeyBytes (SHA-256 hex = 64 chars).
    const std::string fp = cache.generateFingerprint("SELECT contract_test", {});
    ASSERT_LE(fp.size(), kMaxCacheKeyBytes)
        << "Generated fingerprint must respect kMaxCacheKeyBytes (§1)";

    // A small valid JSON object — well within kMaxCacheValueBytes.
    nlohmann::json result = {{"rows", 1}, {"status", "ok"}, {"contract", "v1"}};

    // put() must succeed for valid input.
    const bool inserted = cache.put(fp, {}, result);
    ASSERT_TRUE(inserted) << "put() of a valid entry must succeed (contract §1/§2)";

    // get() must return the same value.
    const auto entry = cache.get(fp, "");
    ASSERT_TRUE(entry.has_value()) << "get() after put() must return the entry";
    EXPECT_EQ(entry->result["contract"], "v1");
}
