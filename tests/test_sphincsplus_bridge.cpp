/**
 * @file test_sphincsplus_bridge.cpp
 * @brief Unit tests for SphincsPlus injectable bridge callbacks (STUB #14).
 *
 * Tests verify the three injectable callback slots:
 *   SPHINCS-BRIDGE-01  no fn injected → Ed25519 simulation is used (non-empty key pair)
 *   SPHINCS-BRIDGE-02  fn injected → the injected fn is called (not the simulation)
 *   SPHINCS-BRIDGE-03  fn throws → sign/verify return empty/false (fail-closed)
 */

#include <gtest/gtest.h>
#include "security/post_quantum_crypto.h"

using themis::security::SphincsPlus;

// ── Fixture ───────────────────────────────────────────────────────────────────

class SphincsPlusBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Always clear all global bridge slots after each test.
        SphincsPlus::setGenerateKeyPairFn({});
        SphincsPlus::setSignFn({});
        SphincsPlus::setVerifyFn({});
    }
};

// ── SPHINCS-BRIDGE-01 ─────────────────────────────────────────────────────────
// With no fn injected the Ed25519 simulation runs: generateKeyPair returns
// non-empty keys, sign returns a non-empty signature, verify returns true.
TEST_F(SphincsPlusBridgeTest, NoFnUsesSimulation) {
    SphincsPlus sp;
    auto kp = sp.generateKeyPair();
    EXPECT_FALSE(kp.public_key.empty());
    EXPECT_FALSE(kp.secret_key.empty());

    const std::vector<uint8_t> msg = {0x01, 0x02, 0x03};
    auto sig = sp.sign(msg, kp.secret_key);
    EXPECT_FALSE(sig.empty());

    EXPECT_TRUE(sp.verify(msg, sig, kp.public_key));
}

// ── SPHINCS-BRIDGE-02 ─────────────────────────────────────────────────────────
// With fns injected, they are called instead of the simulation.
TEST_F(SphincsPlusBridgeTest, InjectedFnIsCalled) {
    bool gen_called  = false;
    bool sign_called = false;
    bool verify_called = false;

    const std::vector<uint8_t> sentinel_pub  = {0xAA};
    const std::vector<uint8_t> sentinel_sec  = {0xBB};
    const std::vector<uint8_t> sentinel_sig  = {0xCC};

    SphincsPlus::setGenerateKeyPairFn([&]() -> SphincsPlus::KeyPair {
        gen_called = true;
        return {sentinel_pub, sentinel_sec};
    });
    SphincsPlus::setSignFn(
        [&](const std::vector<uint8_t>&,
            const std::vector<uint8_t>&) -> std::vector<uint8_t> {
            sign_called = true;
            return sentinel_sig;
        });
    SphincsPlus::setVerifyFn(
        [&](const std::vector<uint8_t>&,
            const std::vector<uint8_t>&,
            const std::vector<uint8_t>&) -> bool {
            verify_called = true;
            return true;
        });

    SphincsPlus sp;
    auto kp = sp.generateKeyPair();
    EXPECT_TRUE(gen_called);
    EXPECT_EQ(kp.public_key, sentinel_pub);
    EXPECT_EQ(kp.secret_key, sentinel_sec);

    const std::vector<uint8_t> msg = {0x10};
    // Use 64-byte placeholder key so simulation path (if reached) doesn't throw
    const std::vector<uint8_t> dummy_sk(64, 0x00);
    auto sig = sp.sign(msg, dummy_sk);
    EXPECT_TRUE(sign_called);
    EXPECT_EQ(sig, sentinel_sig);

    EXPECT_TRUE(sp.verify(msg, sig, kp.public_key));
    EXPECT_TRUE(verify_called);
}

// ── SPHINCS-BRIDGE-03 ─────────────────────────────────────────────────────────
// When injected sign/verify fns throw, the methods return empty/{} / false
// (fail-closed behaviour).
TEST_F(SphincsPlusBridgeTest, ThrowingFnIsFailClosed) {
    SphincsPlus::setSignFn(
        [](const std::vector<uint8_t>&,
           const std::vector<uint8_t>&) -> std::vector<uint8_t> {
            throw std::runtime_error("simulated sign error");
        });
    SphincsPlus::setVerifyFn(
        [](const std::vector<uint8_t>&,
           const std::vector<uint8_t>&,
           const std::vector<uint8_t>&) -> bool {
            throw std::runtime_error("simulated verify error");
        });

    SphincsPlus sp;
    const std::vector<uint8_t> msg    = {0xFF};
    const std::vector<uint8_t> sk(64, 0x42);
    const std::vector<uint8_t> pub(32, 0x42);
    const std::vector<uint8_t> sig(64, 0x00);

    EXPECT_NO_THROW({
        auto result = sp.sign(msg, sk);
        EXPECT_TRUE(result.empty());
    });

    EXPECT_NO_THROW({
        bool ok = sp.verify(msg, sig, pub);
        EXPECT_FALSE(ok);
    });
}
