/*
 * ThemisDB | File: test_cache_tenant_isolation_hardening.cpp | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY
 * Status: Phase 4b — Cache Tenant Isolation Hardening Tests
 *
 * Validates that the AdaptiveQueryCache tenant-isolation paths comply with
 * the contract in include/cache/cache_contract.h §1/§3/§4:
 *
 *   CTI-01 – With tenant isolation disabled, global namespace is shared
 *   CTI-02 – With tenant isolation enabled, tenants have separate namespaces
 *   CTI-03 – Cross-tenant key collision does NOT leak data across tenants
 *   CTI-04 – Empty tenant ID maps to the global namespace (opt-out semantics)
 *   CTI-05 – Tenant invalidation evicts only the targeted tenant's entries
 *   CTI-06 – Distinct fingerprints are generated for different tenants
 *   CTI-07 – put() with large valid result complies with §2 size contract
 *   CTI-08 – Repeated get/put cycles on the same fingerprint remain stable
 *
 * All tests use L3 disabled (no RocksDB dependency).
 * Tenant isolation is exercised via enable_tenant_isolation = true/false.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "cache/cache_contract.h"
#include "cache/adaptive_query_cache.h"

using namespace themis;
using namespace themis::cache;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static AdaptiveQueryCache::Config makeIsolationConfig(bool tenant_iso) {
    AdaptiveQueryCache::Config cfg;
    cfg.l1_max_entries          = 128;
    cfg.l1_max_entry_size       = 65536;
    cfg.l2_max_entries          = 256;
    cfg.l2_max_entry_size       = 65536;
    cfg.l3_db_path              = "";         // L3 disabled
    cfg.enable_circuit_breaker  = false;
    cfg.enable_size_limits      = true;
    cfg.max_total_entry_size    = 67108864;   // 64 MiB
    cfg.enable_adaptive_ttl     = false;
    cfg.enable_rate_limiting    = false;
    cfg.enable_tenant_isolation = tenant_iso;
    return cfg;
}

static nlohmann::json makeResult(const std::string& payload) {
    return {{"payload", payload}, {"status", "ok"}};
}

// ─────────────────────────────────────────────────────────────────────────────
// CTI-01: Tenant isolation OFF — global namespace is shared across callers
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTenantIsolationHardening, CTI01_GlobalNamespaceSharedWhenIsolationOff) {
    AdaptiveQueryCache cache(makeIsolationConfig(false));

    const std::string fp = cache.generateFingerprint("SELECT 1", {});
    ASSERT_TRUE(cache.put(fp, {}, makeResult("global_value")));

    // Same fingerprint, different tenant IDs — still retrieves the same entry
    // because tenant isolation is off.
    const auto e1 = cache.get(fp, "");
    const auto e2 = cache.get(fp, "tenant_a");
    ASSERT_TRUE(e1.has_value()) << "Global get must find the entry";
    ASSERT_TRUE(e2.has_value()) << "Tenant-prefixed get must find the entry when isolation is off";
    EXPECT_EQ(e1->result["payload"], "global_value");
    EXPECT_EQ(e2->result["payload"], "global_value");
}

// ─────────────────────────────────────────────────────────────────────────────
// CTI-02: Tenant isolation ON — tenants have separate namespaces
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTenantIsolationHardening, CTI02_TenantsHaveSeparateNamespacesWhenEnabled) {
    AdaptiveQueryCache cache(makeIsolationConfig(true));

    // Fingerprint is the same query string; tenant keying differentiates the
    // namespace when isolation is enabled.
    const std::string query = "SELECT tenant_test";
    const std::string fp_a = cache.generateFingerprint(query, {}, "tenant_alpha");
    const std::string fp_b = cache.generateFingerprint(query, {}, "tenant_beta");

    ASSERT_TRUE(cache.put(fp_a, {}, makeResult("alpha_result"), "tenant_alpha"));
    ASSERT_TRUE(cache.put(fp_b, {}, makeResult("beta_result"),  "tenant_beta"));

    const auto ea = cache.get(fp_a, "tenant_alpha");
    const auto eb = cache.get(fp_b, "tenant_beta");
    ASSERT_TRUE(ea.has_value());
    ASSERT_TRUE(eb.has_value());
    EXPECT_EQ(ea->result["payload"], "alpha_result");
    EXPECT_EQ(eb->result["payload"], "beta_result");
}

// ─────────────────────────────────────────────────────────────────────────────
// CTI-03: Cross-tenant data leakage — tenant A's entry must not be accessible
//         via tenant B's get() when isolation is enabled.
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTenantIsolationHardening, CTI03_CrossTenantLeakageNotPossible) {
    AdaptiveQueryCache cache(makeIsolationConfig(true));

    const std::string query = "SELECT secret_data";
    const std::string fp_owner = cache.generateFingerprint(query, {}, "owner_tenant");

    // Insert under owner_tenant.
    ASSERT_TRUE(cache.put(fp_owner, {}, makeResult("secret"), "owner_tenant"));

    // A different tenant using the same fingerprint generation — they must
    // generate a different fingerprint due to tenant isolation keying.
    const std::string fp_attacker = cache.generateFingerprint(query, {}, "attacker_tenant");

    // The fingerprints must differ when tenant isolation is on.
    EXPECT_NE(fp_owner, fp_attacker)
        << "Tenant-namespaced fingerprints must differ (§1 tenant-ID keying)";

    // Attacker's namespace must not find owner's entry.
    const auto leaked = cache.get(fp_owner, "attacker_tenant");
    // Either the entry is not found, or the returned result must not contain
    // the secret payload (implementation-dependent miss semantics).
    if (leaked.has_value()) {
        // If returned, the result must not carry the owner's payload
        // (this would be a cross-tenant data leak).
        EXPECT_NE(leaked->result.value("payload", std::string{}), "secret")
            << "Cross-tenant data leakage detected — tenant isolation failure";
    }
    // Most likely: attacker simply gets a miss.
}

// ─────────────────────────────────────────────────────────────────────────────
// CTI-04: Empty tenant ID maps to the global namespace
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTenantIsolationHardening, CTI04_EmptyTenantIsGlobalNamespace) {
    AdaptiveQueryCache cache(makeIsolationConfig(true));

    const std::string fp_global = cache.generateFingerprint("SELECT global", {}, "");
    ASSERT_TRUE(cache.put(fp_global, {}, makeResult("global_entry"), ""));

    const auto e = cache.get(fp_global, "");
    ASSERT_TRUE(e.has_value()) << "Global-namespace entry must be retrievable with empty tenant";
    EXPECT_EQ(e->result["payload"], "global_entry");
}

// ─────────────────────────────────────────────────────────────────────────────
// CTI-05: Tenant invalidation evicts only the targeted tenant
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTenantIsolationHardening, CTI05_TenantInvalidationEvictsOnlyTargetTenant) {
    AdaptiveQueryCache cache(makeIsolationConfig(true));

    const std::string fp_a = cache.generateFingerprint("SELECT evict_test", {}, "evict_tenant");
    const std::string fp_b = cache.generateFingerprint("SELECT evict_test", {}, "safe_tenant");

    ASSERT_TRUE(cache.put(fp_a, {}, makeResult("will_be_evicted"), "evict_tenant"));
    ASSERT_TRUE(cache.put(fp_b, {}, makeResult("must_survive"),   "safe_tenant"));

    // Invalidate all entries for evict_tenant.
    cache.invalidateTenant("evict_tenant");

    // evict_tenant entry must be gone.
    const auto evicted = cache.get(fp_a, "evict_tenant");
    EXPECT_FALSE(evicted.has_value())
        << "Evicted tenant entry must be gone after invalidateTenant()";

    // safe_tenant entry must survive.
    const auto survivor = cache.get(fp_b, "safe_tenant");
    ASSERT_TRUE(survivor.has_value())
        << "safe_tenant entry must survive eviction of a different tenant";
    EXPECT_EQ(survivor->result["payload"], "must_survive");
}

// ─────────────────────────────────────────────────────────────────────────────
// CTI-06: Distinct fingerprints for different tenants (same query)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTenantIsolationHardening, CTI06_DistinctFingerprintsPerTenant) {
    AdaptiveQueryCache cache(makeIsolationConfig(true));

    const std::string query  = "SELECT fp_test FROM table";
    const std::string params = "";

    const std::string fp_t1 = cache.generateFingerprint(query, {}, "tenant_one");
    const std::string fp_t2 = cache.generateFingerprint(query, {}, "tenant_two");
    const std::string fp_global = cache.generateFingerprint(query, {}, "");

    EXPECT_NE(fp_t1, fp_t2)
        << "Different tenants must produce distinct fingerprints (§1)";
    EXPECT_NE(fp_t1, fp_global)
        << "Tenant fingerprint must differ from global fingerprint";
    EXPECT_NE(fp_t2, fp_global)
        << "Tenant fingerprint must differ from global fingerprint";

    // Fingerprints must be non-empty and within the key size limit.
    EXPECT_FALSE(fp_t1.empty());
    EXPECT_LE(fp_t1.size(), kMaxCacheKeyBytes)
        << "Fingerprint must respect kMaxCacheKeyBytes contract (§1)";
}

// ─────────────────────────────────────────────────────────────────────────────
// CTI-07: put() with large but valid result complies with §2 size contract
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTenantIsolationHardening, CTI07_LargeValidResultAcceptedByContract) {
    AdaptiveQueryCache cache(makeIsolationConfig(false));

    // Build a JSON array with many elements — large but within L2 tier limits
    // and below kMaxCacheValueBytes.
    nlohmann::json large_array = nlohmann::json::array();
    for (int i = 0; i < 1000; ++i) {
        large_array.push_back({{"id", i}, {"value", "x"}});
    }

    const std::string fp = cache.generateFingerprint("SELECT large_result", {});
    const bool ok = cache.put(fp, {}, large_array);
    // The array is an accepted value type (§2: objects and arrays are valid).
    EXPECT_TRUE(ok) << "Large JSON array within size limits must be accepted";

    const auto entry = cache.get(fp, "");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->result.size(), std::size_t{1000});
}

// ─────────────────────────────────────────────────────────────────────────────
// CTI-08: Repeated get/put cycles remain stable (no internal state corruption)
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheTenantIsolationHardening, CTI08_RepeatedCyclesAreStable) {
    AdaptiveQueryCache cache(makeIsolationConfig(false));

    constexpr int kIterations = 50;
    for (int i = 0; i < kIterations; ++i) {
        const std::string query = "SELECT stable_test_" + std::to_string(i);
        const std::string fp    = cache.generateFingerprint(query, {});
        nlohmann::json    result = {{"iter", i}, {"status", "ok"}};

        const bool ok = cache.put(fp, {}, result);
        ASSERT_TRUE(ok) << "put() must succeed on iteration " << i;

        const auto entry = cache.get(fp, "");
        ASSERT_TRUE(entry.has_value()) << "get() must find entry on iteration " << i;
        EXPECT_EQ(entry->result["iter"], i);
    }
}
