/**
 * @file test_kdf_argon2_bridge.cpp
 * @brief Unit tests for Argon2idKeyDerivationService injectable bridge callback (STUB #31).
 *
 * Tests verify the single injectable callback slot:
 *   KDF-BRIDGE-01  no fn injected → built-in Argon2id/SHA-256 implementation is used
 *   KDF-BRIDGE-02  fn injected → the injected fn is called (not the built-in)
 *   KDF-BRIDGE-03  fn throws → execution falls through to built-in impl (NOT fail-closed)
 */

#include <gtest/gtest.h>
#include "user_storage_encrypted/key_derivation_service.hpp"

using themis::plugins::user_storage::Argon2idKeyDerivationService;
using themis::plugins::user_storage::Result;

// ── Fixture ───────────────────────────────────────────────────────────────────

class Argon2BridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        Argon2idKeyDerivationService::setDeriveKeyFn({});
    }
};

// ── KDF-BRIDGE-01 ─────────────────────────────────────────────────────────────
// With no fn injected the built-in path runs; result is non-empty for valid inputs.
TEST_F(Argon2BridgeTest, NoFnUsesBuiltIn) {
    Argon2idKeyDerivationService kdf;
    const std::vector<uint8_t> master(32, 0xAB);
    const std::vector<uint8_t> salt(16, 0xCD);

    auto result = kdf.deriveKey(master, salt);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value().size(), 32u);
}

// ── KDF-BRIDGE-02 ─────────────────────────────────────────────────────────────
// With fn injected, it is called instead of the built-in.
TEST_F(Argon2BridgeTest, InjectedFnIsCalled) {
    bool fn_called = false;
    const std::vector<uint8_t> sentinel(32, 0x55);

    Argon2idKeyDerivationService::setDeriveKeyFn(
        [&](const std::vector<uint8_t>& mk,
            const std::vector<uint8_t>& s) -> Result<std::vector<uint8_t>> {
            fn_called = true;
            EXPECT_FALSE(mk.empty());
            EXPECT_FALSE(s.empty());
            return Result<std::vector<uint8_t>>(sentinel);
        });

    Argon2idKeyDerivationService kdf;
    const std::vector<uint8_t> master(32, 0xAB);
    const std::vector<uint8_t> salt(16, 0xCD);

    auto result = kdf.deriveKey(master, salt);
    EXPECT_TRUE(fn_called);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_EQ(result.value(), sentinel);
}

// ── KDF-BRIDGE-03 ─────────────────────────────────────────────────────────────
// When fn throws, execution falls through to the built-in implementation
// (not fail-closed — the built-in may succeed).
TEST_F(Argon2BridgeTest, ThrowingFnFallsThrough) {
    Argon2idKeyDerivationService::setDeriveKeyFn(
        [](const std::vector<uint8_t>&,
           const std::vector<uint8_t>&) -> Result<std::vector<uint8_t>> {
            throw std::runtime_error("simulated kdf error");
        });

    Argon2idKeyDerivationService kdf;
    const std::vector<uint8_t> master(32, 0xAB);
    const std::vector<uint8_t> salt(16, 0xCD);

    // Should not throw — fallthrough to built-in must succeed or return error Result.
    EXPECT_NO_THROW({
        auto result = kdf.deriveKey(master, salt);
        // The built-in path will produce a valid key (Argon2id or SHA-256 fallback).
        EXPECT_TRUE(result.isSuccess());
    });
}
