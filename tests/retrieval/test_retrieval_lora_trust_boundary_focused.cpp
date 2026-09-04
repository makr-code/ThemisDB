/**
 * @file test_retrieval_lora_trust_boundary_focused.cpp
 * @brief Block 3 — LoRA Adapter Trust Boundary (GA-Blocking).
 *
 * Acceptance criteria:
 *   LTB-01  Adapter with correct checksum → AdapterConsistencyChecker accepts
 *           the adapter (is_valid == true).
 *   LTB-02  Adapter with tampered data (wrong checksum) → checker rejects it
 *           (is_valid == false).
 *   LTB-03  Adapter from an unknown signer_id without a registered verifier →
 *           the loading behaviour is clearly defined: the adapter is rejected
 *           when the trust policy is strict, or accepted-with-warning when
 *           lenient.  Both outcomes are explicitly documented; no silent pass.
 *
 * All infrastructure is fully in-process; no real model files are required.
 * The AdapterConsistencyChecker is used directly; signer_id enforcement is
 * layered via a thin in-test trust registry.
 *
 * @version 1.0.0
 * @note CTest labels: retrieval;security;lora;trust;GA-blocking
 */

#include <gtest/gtest.h>

#include "llm/lora_framework/adapter_consistency_checker.h"
#include "llm/lora_framework/lora_config.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace lora = themis::llm::lora;

// ─────────────────────────────────────────────────────────────────────────────
// In-test trust registry
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/**
 * @brief Minimal in-test trust registry for signer IDs.
 *
 * Production code would look up signers against a PKI/key-management service.
 * Here we model the policy contract with a simple allowlist so the tests can
 * exercise the "unknown signer" branch deterministically.
 */
class AdapterTrustRegistry {
public:
    enum class Policy { STRICT, LENIENT };

    explicit AdapterTrustRegistry(Policy policy = Policy::STRICT)
        : policy_(policy) {}

    void registerSigner(const std::string& signer_id) {
        known_signers_.insert(signer_id);
    }

    /**
     * @brief Check whether a signer is trusted.
     *
     * STRICT  → unknown signer → reject (return false).
     * LENIENT → unknown signer → accept with logged warning (return true).
     */
    bool isTrusted(const std::string& signer_id, std::string* warning_out = nullptr) const {
        if (known_signers_.count(signer_id)) {
            return true;
        }
        if (policy_ == Policy::LENIENT) {
            if (warning_out) {
                *warning_out = "WARNING: signer_id '" + signer_id +
                               "' is not in the trust registry; "
                               "adapter loaded under lenient policy.";
            }
            return true;
        }
        return false;  // STRICT: unknown signer → reject
    }

private:
    std::unordered_set<std::string> known_signers_;
    Policy policy_;
};

/**
 * @brief Build a deterministic byte payload for testing.
 *
 * Produces exactly @p size bytes with a reproducible pattern.
 */
std::vector<uint8_t> makePayload(size_t size, uint8_t seed = 0xAB) {
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>((seed + i) & 0xFF);
    }
    return data;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// LTB-01 — Correct checksum: adapter accepted
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LTB-01a: Adapter with matching checksum → verifyChecksum returns true.
 */
TEST(LoRATrustBoundaryTest, LTB01a_CorrectChecksumAccepted) {
    lora::AdapterConsistencyChecker checker;

    const auto data = makePayload(1024);
    const std::string expected_checksum = checker.calculateChecksum(data);

    ASSERT_FALSE(expected_checksum.empty())
        << "calculateChecksum must produce a non-empty hex digest.";

    EXPECT_TRUE(checker.verifyChecksum(data, expected_checksum))
        << "Adapter data with correct checksum must be accepted.";
}

/**
 * @test LTB-01b: Full checkAdapter pipeline with correct checksum → is_valid.
 */
TEST(LoRATrustBoundaryTest, LTB01b_FullCheckAdapterPipelineAccepted) {
    lora::AdapterConsistencyChecker::Config cfg;
    cfg.enable_checksums  = true;
    cfg.enable_signatures = false;  // signature not exercised in this sub-test
    cfg.strict_mode       = true;
    lora::AdapterConsistencyChecker checker(cfg);

    const auto data = makePayload(2048, 0x7F);

    lora::AdapterMetadata meta;
    meta.adapter_id = "test-adapter-v1";
    meta.version    = "1.0.0";
    meta.checksum   = checker.calculateChecksum(data);
    meta.base_model = "qwen2.5-coder:14b";

    const auto result = checker.checkAdapter(meta.adapter_id, data, meta);

    EXPECT_TRUE(result.is_valid)
        << "checkAdapter must succeed when the checksum matches.  "
           "error_message: " << result.error_message;
    EXPECT_EQ(result.checksum, meta.checksum)
        << "Result checksum must match the expected checksum.";
}

// ─────────────────────────────────────────────────────────────────────────────
// LTB-02 — Tampered data: adapter rejected
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LTB-02a: verifyChecksum with tampered data → returns false.
 */
TEST(LoRATrustBoundaryTest, LTB02a_TamperedDataRejectedByVerifyChecksum) {
    lora::AdapterConsistencyChecker checker;

    const auto original  = makePayload(512, 0x11);
    const std::string cs = checker.calculateChecksum(original);

    // Tamper: flip one byte in the payload.
    auto tampered = original;
    tampered[42] ^= 0xFF;

    EXPECT_FALSE(checker.verifyChecksum(tampered, cs))
        << "Tampered adapter data must NOT pass checksum verification.";
}

/**
 * @test LTB-02b: checkAdapter with mismatched checksum in metadata → is_valid=false.
 */
TEST(LoRATrustBoundaryTest, LTB02b_FullCheckAdapterRejectedOnMismatch) {
    lora::AdapterConsistencyChecker::Config cfg;
    cfg.enable_checksums  = true;
    cfg.enable_signatures = false;
    cfg.strict_mode       = true;
    lora::AdapterConsistencyChecker checker(cfg);

    const auto original_data = makePayload(1024, 0x55);
    const auto tampered_data = makePayload(1024, 0x56);  // different seed → different content

    lora::AdapterMetadata meta;
    meta.adapter_id = "tampered-adapter";
    meta.version    = "1.0.0";
    // Checksum computed from ORIGINAL data, but we submit TAMPERED data.
    meta.checksum   = checker.calculateChecksum(original_data);
    meta.base_model = "mistral-7b";

    const auto result = checker.checkAdapter(meta.adapter_id, tampered_data, meta);

    EXPECT_FALSE(result.is_valid)
        << "checkAdapter must reject an adapter whose data does not match "
           "the stored checksum.  error_message: " << result.error_message;
}

/**
 * @test LTB-02c: Empty adapter data with non-empty checksum → rejected.
 */
TEST(LoRATrustBoundaryTest, LTB02c_EmptyDataWithChecksumRejected) {
    lora::AdapterConsistencyChecker checker;

    const auto real_data = makePayload(256);
    const std::string real_checksum = checker.calculateChecksum(real_data);

    const std::vector<uint8_t> empty_data;

    EXPECT_FALSE(checker.verifyChecksum(empty_data, real_checksum))
        << "Empty data submitted with a non-empty checksum must be rejected.";
}

// ─────────────────────────────────────────────────────────────────────────────
// LTB-03 — Unknown signer_id: behaviour clearly defined
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test LTB-03a: STRICT trust registry + unknown signer → adapter rejected.
 *
 * Governance requirement: an adapter from an unrecognised signer must NOT
 * be loaded under a strict trust policy.
 */
TEST(LoRATrustBoundaryTest, LTB03a_UnknownSignerRejectedUnderStrictPolicy) {
    AdapterTrustRegistry registry(AdapterTrustRegistry::Policy::STRICT);
    registry.registerSigner("known-signer-org");

    const std::string unknown_signer = "rogue-signer-unknown";

    EXPECT_FALSE(registry.isTrusted(unknown_signer))
        << "LTB-03a: Unknown signer must be REJECTED under STRICT trust policy.";
}

/**
 * @test LTB-03b: LENIENT trust registry + unknown signer → loaded with warning.
 *
 * Documented behaviour: lenient policy accepts unknown signers but MUST
 * emit a warning so operators can detect and remediate.
 *
 * Governance note: lenient mode must only be used in development environments.
 * Production deployments must use STRICT mode (see ROADMAP.md §LoRA Trust).
 */
TEST(LoRATrustBoundaryTest, LTB03b_UnknownSignerAcceptedWithWarningUnderLenientPolicy) {
    AdapterTrustRegistry registry(AdapterTrustRegistry::Policy::LENIENT);
    registry.registerSigner("known-signer-org");

    const std::string unknown_signer = "external-contributor-unverified";
    std::string warning = {};

    const bool trusted = registry.isTrusted(unknown_signer, &warning);

    EXPECT_TRUE(trusted)
        << "LTB-03b: Unknown signer is accepted under LENIENT policy (dev-only mode).";
    EXPECT_FALSE(warning.empty())
        << "LTB-03b: A non-empty warning MUST be emitted when an unknown signer "
           "is accepted under lenient policy so operators can detect the gap.\n"
           "warning=" << warning;
}

/**
 * @test LTB-03c: Known signer under STRICT policy → no warning, accepted.
 *
 * Confirms the positive path: registered signers load without warnings
 * under the strict policy.
 */
TEST(LoRATrustBoundaryTest, LTB03c_KnownSignerAcceptedWithoutWarningUnderStrictPolicy) {
    AdapterTrustRegistry registry(AdapterTrustRegistry::Policy::STRICT);
    registry.registerSigner("trusted-org-prod");

    std::string warning = {};
    const bool trusted = registry.isTrusted("trusted-org-prod", &warning);

    EXPECT_TRUE(trusted)
        << "LTB-03c: Registered signer must be trusted under STRICT policy.";
    EXPECT_TRUE(warning.empty())
        << "LTB-03c: No warning expected for a registered signer.";
}

/**
 * @test LTB-03d: End-to-end load simulation — unknown signer under strict
 *       policy prevents the adapter from reaching checkAdapter.
 *
 * This test models the complete trust chain:
 *   1. Check signer trust (registry gate).
 *   2. Only proceed to checksum/checkAdapter when signer is trusted.
 */
TEST(LoRATrustBoundaryTest, LTB03d_UnknownSignerStopsBeforeChecksumValidation) {
    AdapterTrustRegistry registry(AdapterTrustRegistry::Policy::STRICT);
    // No signers registered.

    lora::AdapterConsistencyChecker checker;
    const auto data = makePayload(512);
    const std::string checksum = checker.calculateChecksum(data);

    lora::AdapterMetadata meta;
    meta.adapter_id = "rogue-adapter";
    meta.version    = "0.0.1";
    meta.checksum   = checksum;
    meta.base_model = "any-model";

    const std::string signer = "rogue-org";

    // Gate 1: trust check
    const bool signer_trusted = registry.isTrusted(signer);
    EXPECT_FALSE(signer_trusted)
        << "LTB-03d: Adapter from unknown signer must be stopped at the trust gate.";

    // Gate 2: only call checkAdapter when signer is trusted.
    if (signer_trusted) {
        const auto result = checker.checkAdapter(meta.adapter_id, data, meta);
        // This branch must NOT be taken.
        FAIL() << "LTB-03d: checkAdapter must NOT be called when signer is untrusted.";
        (void)result;
    }

    // If we reach here without calling checkAdapter, the test passes: the
    // trust boundary correctly stops the adapter before checksum validation.
    SUCCEED();
}
