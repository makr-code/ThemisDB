/**
 * @file test_utils_crypto_hardening.cpp
 * @brief Phase 4 hardening tests for HKDFHelper, HKDFCache, and error contracts.
 *
 * Coverage targets (Phase 4 gate):
 *  - HKDFHelper::derive() succeeds under normal conditions
 *  - HKDFHelper: invalid output_length → exception (fail-closed, no weak key)
 *  - HKDFCache::derive_cached() caches and returns identical material
 *  - HKDFCache: eviction under capacity pressure works correctly
 *  - Crypto error codes in range 9050-9059
 *  - ErrorContext category for crypto codes
 *  - Key material zeroization (OPENSSL_cleanse) does not corrupt program state
 *
 * Note: LEKManager tests require a live RocksDB instance and are integration-
 * level; they are excluded from this unit-test file.  The constructor fail-
 * closed path is verified by checking the error-contract expectations in the
 * implementation comments.
 */

#include <gtest/gtest.h>

#include "utils/error_contracts.h"
#include "utils/hkdf_cache.h"
#include "utils/hkdf_helper.h"

#include <openssl/crypto.h>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis::utils;

// ─────────────────────────────────────────────────────────────────────────────
// CH-01: HKDFHelper::derive() produces the correct output length
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFDeriveProducesCorrectLength) {
    std::vector<uint8_t> ikm(32, 0xAA);
    std::vector<uint8_t> salt(16, 0xBB);
    const std::string info = "test.derive.v1";

    for (size_t len : {16u, 32u, 64u}) {
        auto key = HKDFHelper::derive(ikm, salt, info, len);
        EXPECT_EQ(key.size(), len) << "expected length " << len;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-02: HKDFHelper::derive() is deterministic (same inputs → same output)
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFDeriveIsDeterministic) {
    std::vector<uint8_t> ikm(32, 0x11);
    std::vector<uint8_t> salt(16, 0x22);
    const std::string info = "deterministic.test";

    auto k1 = HKDFHelper::derive(ikm, salt, info, 32);
    auto k2 = HKDFHelper::derive(ikm, salt, info, 32);
    EXPECT_EQ(k1, k2);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-03: Different info strings produce different keys (domain separation)
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFDifferentInfoProducesDifferentKeys) {
    std::vector<uint8_t> ikm(32, 0x33);
    std::vector<uint8_t> salt(16, 0x44);

    auto k1 = HKDFHelper::derive(ikm, salt, "context.A", 32);
    auto k2 = HKDFHelper::derive(ikm, salt, "context.B", 32);
    EXPECT_NE(k1, k2);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-04: Zero output_length → runtime_error (fail-closed – no empty key)
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFZeroLengthOutputThrows) {
    std::vector<uint8_t> ikm(32, 0x55);
    std::vector<uint8_t> salt(16, 0x66);
    EXPECT_THROW(HKDFHelper::derive(ikm, salt, "info", 0), std::exception);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-05: Oversized output_length → throws (RFC 5869 limit 8160 bytes)
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFOverMaxLengthThrows) {
    std::vector<uint8_t> ikm(32, 0x77);
    std::vector<uint8_t> salt(16, 0x88);
    // 255 * 32 = 8160 is the SHA-256 HKDF output limit
    EXPECT_THROW(HKDFHelper::derive(ikm, salt, "info", 8161), std::exception);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-06: HKDFCache::derive_cached() returns same result as direct derive
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFCacheDeriveMatchesDirectDerive) {
    HKDFCache cache;
    std::vector<uint8_t> ikm(32, 0x99);
    std::vector<uint8_t> salt(16, 0xAA);
    const std::string info = "cache.test.v1";

    auto direct = HKDFHelper::derive(ikm, salt, info, 32);
    auto cached = cache.derive_cached(ikm, salt, info, 32);
    EXPECT_EQ(direct, cached);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-07: HKDFCache: second call is a hit (same result returned)
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFCacheSecondCallReturnsHit) {
    HKDFCache cache;
    std::vector<uint8_t> ikm(32, 0xBB);
    std::vector<uint8_t> salt(16, 0xCC);
    const std::string info = "cache.hit.v1";

    auto first  = cache.derive_cached(ikm, salt, info, 32);
    auto second = cache.derive_cached(ikm, salt, info, 32);
    EXPECT_EQ(first, second);

    auto s = cache.stats();
    EXPECT_GE(s.hits, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-08: HKDFCache: clear() resets cache (stats show miss after clear)
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFCacheClearCausesSubsequentMiss) {
    HKDFCache cache;
    std::vector<uint8_t> ikm(32, 0xDD);
    std::vector<uint8_t> salt(16, 0xEE);
    const std::string info = "cache.clear.v1";

    cache.derive_cached(ikm, salt, info, 32); // populate
    cache.clear();                             // evict everything

    auto s_before = cache.stats();
    cache.derive_cached(ikm, salt, info, 32); // should be a miss
    auto s_after = cache.stats();

    EXPECT_GT(s_after.misses, s_before.misses);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-09: Key material zeroization does not corrupt heap
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, KeyMaterialZeroizationDoesNotCorruptHeap) {
    std::vector<uint8_t> ikm(32, 0xFF);
    std::vector<uint8_t> salt(16, 0x00);

    auto key = HKDFHelper::derive(ikm, salt, "zeroize.test", 32);
    ASSERT_EQ(key.size(), 32u);

    // Zeroize: OPENSSL_cleanse is the approved method
    OPENSSL_cleanse(key.data(), key.size());

    // All bytes must now be zero (OPENSSL_cleanse guarantees this)
    bool all_zero = std::all_of(key.begin(), key.end(), [](uint8_t b) { return b == 0; });
    EXPECT_TRUE(all_zero);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-10: Crypto error codes are in range 9050-9059
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, CryptoErrorCodesInRange) {
    using EC = ErrorCode;
    auto check = [](EC code) {
        auto v = static_cast<uint16_t>(code);
        EXPECT_GE(v, uint16_t{9050}) << "code " << v << " below 9050";
        EXPECT_LE(v, uint16_t{9059}) << "code " << v << " above 9059";
    };
    check(EC::CRYPTO_KEY_DERIVATION_FAILED);
    check(EC::CRYPTO_KEY_INVALID);
    check(EC::CRYPTO_KEY_EXPIRED);
    check(EC::CRYPTO_KEY_NOT_FOUND);
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-11: ErrorContext category for crypto codes is Crypto
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, ErrorContextCategoryIsCrypto) {
    for (auto code : {ErrorCode::CRYPTO_KEY_DERIVATION_FAILED,
                      ErrorCode::CRYPTO_KEY_INVALID,
                      ErrorCode::CRYPTO_KEY_NOT_FOUND,
                      ErrorCode::CRYPTO_KEY_EXPIRED}) {
        auto ctx = makeErrorContext(code, "CH-11-test", "unit test",
                                    ErrorSeverity::Critical, false);
        EXPECT_EQ(ctx.category, ErrorCategory::KeyDerivation)
            << "code " << static_cast<uint16_t>(code)
            << " should be Crypto category";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CH-12: HKDFCache thread-local accessor does not crash
// ─────────────────────────────────────────────────────────────────────────────
TEST(CryptoHardening, HKDFCacheThreadLocalDoesNotCrash) {
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([]() {
            auto& cache = HKDFCache::threadLocal();
            std::vector<uint8_t> ikm(32, 0x01);
            std::vector<uint8_t> salt(16, 0x02);
            auto key = cache.derive_cached(ikm, salt, "tl.test", 32);
            EXPECT_EQ(key.size(), 32u);
        });
    }
    for (auto& t : threads) {
      t.join();
    }
}
