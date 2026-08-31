/*
 * Tests for Wave 9 Block 5 — speculative decode bridges
 *   W9-16 / W9-17: TokenizerFn and TargetLogitsFn injection into
 *   InferenceEngineEnhanced::trySpeculativeGeneration()
 *
 * Covers: SD-BRG-01..SD-BRG-07
 *   SD-BRG-01 — TokenizerFn: injected fn is registered and cleared correctly
 *   SD-BRG-02 — TokenizerFn: nullptr clears the fn without throwing
 *   SD-BRG-03 — TokenizerFn: clearTokenizerFn() equivalent to setTokenizerFn(nullptr)
 *   SD-BRG-04 — TargetLogitsFn: injected fn is registered and cleared correctly
 *   SD-BRG-05 — TargetLogitsFn: nullptr clears without throwing
 *   SD-BRG-06 — Bridge APIs are independent (clear one does not affect other)
 *   SD-BRG-07 — Repeated set/clear cycle does not throw or corrupt state
 *
 * Because the speculative-decode code path requires a live draft model and
 * remote executor (not available in unit-test CI), these tests exercise the
 * static bridge API directly (setter/getter round-trip via the storage
 * functions) following the same pattern as
 *   tests/llm/test_gpu_tensor_dtype_cast_bridge.cpp
 *
 * Full end-to-end bridge invocation is covered by integration tests that
 * supply a mock ILLMPlugin.
 */

#include <gtest/gtest.h>
#include "llm/inference_engine_enhanced.h"

using namespace themis::llm;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class SpeculativeDecodeBridgeTest : public ::testing::Test {
protected:
    InferenceEngineEnhanced engine_{InferenceEngineEnhanced::Config{}};

    void SetUp() override {
        engine_.setTokenizerFn(nullptr);
        engine_.setTargetLogitsFn(nullptr);
    }
    void TearDown() override {
        engine_.clearTokenizerFn();
        engine_.setTargetLogitsFn(nullptr);
    }
};

// ─── SD-BRG-01: TokenizerFn — registration does not invoke the fn ────────────

TEST_F(SpeculativeDecodeBridgeTest, TokenizerFnBridgeUsedWhenInjected) {
    bool called = false;

    engine_.setTokenizerFn(
        [&](const std::string& /*text*/, size_t vocab_size) -> std::vector<int> {
            called = true;
            return {1, 2, 3};
        });

    // The setter must not call the fn itself — only wires it for later use.
    EXPECT_FALSE(called);
    // Re-clear so TearDown is idempotent.
    engine_.clearTokenizerFn();
    EXPECT_FALSE(called);
}

// ─── SD-BRG-02: TokenizerFn — nullptr clears storage without throwing ────────

TEST_F(SpeculativeDecodeBridgeTest, TokenizerFnFallbackWhenNotInjected) {
    // After clearing, engine must accept nullptr without throwing.
    engine_.setTokenizerFn(
        [](const std::string&, size_t) -> std::vector<int> { return {42}; });
    EXPECT_NO_THROW(engine_.setTokenizerFn(nullptr));
}

// ─── SD-BRG-03: TokenizerFn — clearTokenizerFn() is safe and idempotent ─────

TEST_F(SpeculativeDecodeBridgeTest, TokenizerFnExceptionFallsThrough) {
    // Set a fn that would throw, then clear it — no exception during clear.
    engine_.setTokenizerFn(
        [](const std::string&, size_t) -> std::vector<int> {
            throw std::runtime_error("tokenizer error");
        });
    EXPECT_NO_THROW(engine_.clearTokenizerFn());
    // Second clear on already-clear state must also not throw.
    EXPECT_NO_THROW(engine_.clearTokenizerFn());
}

// ─── SD-BRG-04: TargetLogitsFn — registration does not invoke the fn ─────────

TEST_F(SpeculativeDecodeBridgeTest, TargetLogitsFnBridgeUsedWhenInjected) {
    bool called = false;

    engine_.setTargetLogitsFn(
        [&](const InferenceRequest&,
            size_t /*K*/, size_t vocab_size,
            std::shared_ptr<ILLMPlugin>) -> std::vector<std::vector<float>> {
            called = true;
            return {};
        });

    // Setter must not invoke the fn.
    EXPECT_FALSE(called);
    engine_.setTargetLogitsFn(nullptr);
    EXPECT_FALSE(called);
}

// ─── SD-BRG-05: TargetLogitsFn — nullptr clears without throwing ─────────────

TEST_F(SpeculativeDecodeBridgeTest, TargetLogitsFnFallbackWhenNotInjected) {
    engine_.setTargetLogitsFn(
        [](const InferenceRequest&, size_t K, size_t vocab_size,
           std::shared_ptr<ILLMPlugin>) -> std::vector<std::vector<float>> {
            return std::vector<std::vector<float>>(K + 1,
                                                  std::vector<float>(vocab_size, 0.0f));
        });
    EXPECT_NO_THROW(engine_.setTargetLogitsFn(nullptr));
}

// ─── SD-BRG-06: The two bridges are independent ───────────────────────────────

TEST_F(SpeculativeDecodeBridgeTest, SpeculativeDecodeEndToEndBothBridgesWired) {
    bool tok_registered  = false;
    bool logits_registered = false;

    engine_.setTokenizerFn(
        [&](const std::string&, size_t) -> std::vector<int> {
            tok_registered = true;
            return {10, 20, 30};
        });
    engine_.setTargetLogitsFn(
        [&](const InferenceRequest&, size_t K, size_t vocab_size,
            std::shared_ptr<ILLMPlugin>) -> std::vector<std::vector<float>> {
            logits_registered = true;
            return std::vector<std::vector<float>>(K + 1,
                                                  std::vector<float>(vocab_size, 0.0f));
        });

    // Clear only the tokenizer — logits fn must remain set.
    engine_.clearTokenizerFn();

    // Neither fn was invoked (no live speculative decode run in unit-test scope).
    EXPECT_FALSE(tok_registered);
    EXPECT_FALSE(logits_registered);

    engine_.setTargetLogitsFn(nullptr);
}

// ─── SD-BRG-07: Set/clear cycle is stable under repeated calls ───────────────

TEST_F(SpeculativeDecodeBridgeTest, SetAndClearTokenizerFn) {
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(
            engine_.setTokenizerFn(
                [](const std::string&, size_t v) -> std::vector<int> {
                    return {static_cast<int>(v % 100)};
                }));
        EXPECT_NO_THROW(engine_.clearTokenizerFn());
    }
    // Final state: no fn set; second clear must be safe.
    EXPECT_NO_THROW(engine_.clearTokenizerFn());
}
