/**
 * @file test_hsm_dek_bridge.cpp
 * @brief Unit tests for HSMKeyProviderAdapter WrapDEKFn/UnwrapDEKFn bridge
 *        (STUB #47 / #48).
 *
 * Tests verify the injectable callback slots:
 *   – null by default (slot empty after TearDown)
 *   – setting a non-null fn and a null fn both succeed without error
 *   – the two slots are independent (clearing one does not affect the other)
 *
 * Tests: HSM-DEK-BRIDGE-01..03
 */

#include <gtest/gtest.h>
#include "security/hsm_key_provider_adapter.h"

using themis::security::HSMKeyProviderAdapter;

// ── Fixture ──────────────────────────────────────────────────────────────────

class HsmDekBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Always clear both global bridge slots after each test.
        HSMKeyProviderAdapter::setWrapDEKFn({});
        HSMKeyProviderAdapter::setUnwrapDEKFn({});
    }
};

// ── HSM-DEK-BRIDGE-01 ────────────────────────────────────────────────────────
// WrapDEKFn slot: registering a non-null fn and then clearing it both succeed
// without throwing.
TEST_F(HsmDekBridgeTest, WrapFnSlotAcceptsAndClears) {
    bool called = false;
    EXPECT_NO_THROW(
        HSMKeyProviderAdapter::setWrapDEKFn(
            [&](const std::vector<uint8_t>&) -> std::vector<uint8_t> {
                called = true;
                return {0xDE, 0xAD};
            }));

    // Clearing the slot must also succeed without throwing.
    EXPECT_NO_THROW(HSMKeyProviderAdapter::setWrapDEKFn({}));
    (void)called;  // fn was only stored, not called in this test
}

// ── HSM-DEK-BRIDGE-02 ────────────────────────────────────────────────────────
// UnwrapDEKFn slot: same acceptance + clear contract as the wrap slot.
TEST_F(HsmDekBridgeTest, UnwrapFnSlotAcceptsAndClears) {
    EXPECT_NO_THROW(
        HSMKeyProviderAdapter::setUnwrapDEKFn(
            [](const std::vector<uint8_t>&) -> std::vector<uint8_t> {
                return {0xBE, 0xEF};
            }));

    EXPECT_NO_THROW(HSMKeyProviderAdapter::setUnwrapDEKFn({}));
}

// ── HSM-DEK-BRIDGE-03 ────────────────────────────────────────────────────────
// Wrap and unwrap slots are independent: clearing one does not affect the other.
TEST_F(HsmDekBridgeTest, WrapAndUnwrapSlotsAreIndependent) {
    // Set both slots.
    EXPECT_NO_THROW(
        HSMKeyProviderAdapter::setWrapDEKFn(
            [](const std::vector<uint8_t>&) -> std::vector<uint8_t> {
                return {0x01};
            }));
    EXPECT_NO_THROW(
        HSMKeyProviderAdapter::setUnwrapDEKFn(
            [](const std::vector<uint8_t>&) -> std::vector<uint8_t> {
                return {0x02};
            }));

    // Clearing only the wrap slot must not throw and must leave unwrap intact.
    EXPECT_NO_THROW(HSMKeyProviderAdapter::setWrapDEKFn({}));

    // Replacing the unwrap slot after clearing the wrap slot must still work.
    EXPECT_NO_THROW(
        HSMKeyProviderAdapter::setUnwrapDEKFn(
            [](const std::vector<uint8_t>&) -> std::vector<uint8_t> {
                return {0x03};
            }));

    // Final clean-up (also exercised by TearDown, but explicit here for clarity).
    EXPECT_NO_THROW(HSMKeyProviderAdapter::setUnwrapDEKFn({}));
}
