/**
 * Unit tests for ConfidentialComputing (Intel TDX / AMD SEV support).
 *
 * All tests run on ordinary hardware via the software-fallback path.
 * TEE-specific paths (kernel ioctls) are exercised separately in the
 * integration test suite when hardware is available.
 */

#include <gtest/gtest.h>
#include "security/confidential_computing.h"
#include <cstring>

using namespace themis::security;

// ── Fixture ───────────────────────────────────────────────────────────────────

class ConfidentialComputingTest : public ::testing::Test {
protected:
    void SetUp() override {
        cc_ = ConfidentialComputing::create();
        ASSERT_NE(cc_, nullptr);
    }

    std::unique_ptr<ConfidentialComputing> cc_;
};

// ── Factory / detection tests ─────────────────────────────────────────────────

TEST_F(ConfidentialComputingTest, CreateReturnsNonNull) {
    EXPECT_NE(cc_, nullptr);
}

TEST_F(ConfidentialComputingTest, NameIsNonEmpty) {
    EXPECT_FALSE(cc_->name().empty());
}

TEST_F(ConfidentialComputingTest, DetectReturnsValidResult) {
    auto result = cc_->detect();
    // type must be one of the defined enum values
    EXPECT_TRUE(result.type == TeeType::NONE     ||
                result.type == TeeType::INTEL_TDX ||
                result.type == TeeType::AMD_SEV   ||
                result.type == TeeType::AMD_SEV_SNP);
    EXPECT_FALSE(result.description.empty());
}

TEST_F(ConfidentialComputingTest, DetectConsistentWithName) {
    auto result = cc_->detect();
    // On standard CI hardware we expect software fallback
    if (result.type == TeeType::NONE) {
        EXPECT_EQ(cc_->name(), "Software fallback");
    } else if (result.type == TeeType::INTEL_TDX) {
        EXPECT_EQ(cc_->name(), "Intel TDX");
    } else if (result.type == TeeType::AMD_SEV_SNP) {
        EXPECT_EQ(cc_->name(), "AMD SEV-SNP");
    } else {
        EXPECT_EQ(cc_->name(), "AMD SEV");
    }
}

// ── Attestation report tests ──────────────────────────────────────────────────

TEST_F(ConfidentialComputingTest, AttestationReportHasCorrectTeeType) {
    std::vector<uint8_t> nonce(32, 0xAB);
    auto report = cc_->getAttestationReport(nonce);
    EXPECT_EQ(report.tee_type, cc_->detect().type);
}

TEST_F(ConfidentialComputingTest, AttestationReportEmbeds64ByteReportData) {
    std::vector<uint8_t> nonce(32, 0x42);
    auto report = cc_->getAttestationReport(nonce);
    ASSERT_EQ(report.report_data.size(), 64u);
    // First 32 bytes should equal the supplied nonce
    EXPECT_EQ(std::vector<uint8_t>(report.report_data.begin(),
                                   report.report_data.begin() + 32), nonce);
    // Remaining bytes should be zero-padded
    for (size_t i = 32; i < 64; ++i) {
        EXPECT_EQ(report.report_data[i], 0x00u);
    }
}

TEST_F(ConfidentialComputingTest, AttestationReportHandles64ByteNonce) {
    std::vector<uint8_t> nonce(64, 0xCD);
    auto report = cc_->getAttestationReport(nonce);
    ASSERT_EQ(report.report_data.size(), 64u);
    EXPECT_EQ(report.report_data, nonce);
}

TEST_F(ConfidentialComputingTest, AttestationReportTruncatesOversizeNonce) {
    std::vector<uint8_t> nonce(128, 0x77); // 128 bytes > 64 limit
    auto report = cc_->getAttestationReport(nonce);
    ASSERT_EQ(report.report_data.size(), 64u);
    for (size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(report.report_data[i], 0x77u);
    }
}

TEST_F(ConfidentialComputingTest, AttestationReportEmptyNonceYieldsZeros) {
    std::vector<uint8_t> empty_nonce;
    auto report = cc_->getAttestationReport(empty_nonce);
    ASSERT_EQ(report.report_data.size(), 64u);
    for (auto b : report.report_data) {
        EXPECT_EQ(b, 0x00u);
    }
}

TEST_F(ConfidentialComputingTest, AttestationReportVersionNonEmpty) {
    std::vector<uint8_t> nonce(16, 0x01);
    auto report = cc_->getAttestationReport(nonce);
    EXPECT_FALSE(report.tee_version.empty());
}

TEST_F(ConfidentialComputingTest, SoftwareModeReportIsNotGenuine) {
    auto result = cc_->detect();
    if (result.type == TeeType::NONE) {
        std::vector<uint8_t> nonce(8, 0xFF);
        auto report = cc_->getAttestationReport(nonce);
        EXPECT_FALSE(report.is_genuine);
        EXPECT_TRUE(report.raw_report.empty());
    }
}

// ── Seal / Unseal tests ───────────────────────────────────────────────────────

TEST_F(ConfidentialComputingTest, SealProducesNonEmptyCiphertext) {
    std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03, 0x04};
    auto blob = cc_->seal(plaintext);
    EXPECT_FALSE(blob.ciphertext.empty());
    EXPECT_EQ(blob.iv.size(), 12u);
    EXPECT_EQ(blob.tag.size(), 16u);
}

TEST_F(ConfidentialComputingTest, SealUnsealRoundTrip) {
    std::vector<uint8_t> plaintext = {'H', 'e', 'l', 'l', 'o', ' ', 'T', 'E', 'E'};
    auto blob      = cc_->seal(plaintext);
    auto recovered = cc_->unseal(blob);
    EXPECT_EQ(recovered, plaintext);
}

TEST_F(ConfidentialComputingTest, SealUnsealEmptyData) {
    std::vector<uint8_t> empty;
    auto blob      = cc_->seal(empty);
    auto recovered = cc_->unseal(blob);
    EXPECT_EQ(recovered, empty);
}

TEST_F(ConfidentialComputingTest, SealUnsealLargeData) {
    std::vector<uint8_t> plaintext(4096);
    for (size_t i = 0; i < plaintext.size(); ++i)
        plaintext[i] = static_cast<uint8_t>(i & 0xFF);
    auto blob      = cc_->seal(plaintext);
    auto recovered = cc_->unseal(blob);
    EXPECT_EQ(recovered, plaintext);
}

TEST_F(ConfidentialComputingTest, SealTeeTypeMatchesDetected) {
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
    auto blob = cc_->seal(data);
    EXPECT_EQ(blob.tee_type, cc_->detect().type);
}

TEST_F(ConfidentialComputingTest, SealIvIsRandomised) {
    // Two seal calls should produce different IVs
    std::vector<uint8_t> data(32, 0x55);
    auto blob1 = cc_->seal(data);
    auto blob2 = cc_->seal(data);
    EXPECT_NE(blob1.iv, blob2.iv);
}

TEST_F(ConfidentialComputingTest, TamperedCiphertextFailsUnseal) {
    std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03};
    auto blob = cc_->seal(plaintext);
    // Flip a bit in the ciphertext
    blob.ciphertext[0] ^= 0xFF;
    EXPECT_THROW(cc_->unseal(blob), std::runtime_error);
}

TEST_F(ConfidentialComputingTest, TamperedTagFailsUnseal) {
    std::vector<uint8_t> plaintext = {0xAA, 0xBB};
    auto blob = cc_->seal(plaintext);
    blob.tag[0] ^= 0x01;
    EXPECT_THROW(cc_->unseal(blob), std::runtime_error);
}

TEST_F(ConfidentialComputingTest, TamperedIvFailsUnseal) {
    std::vector<uint8_t> plaintext = {0x11, 0x22, 0x33};
    auto blob = cc_->seal(plaintext);
    blob.iv[0] ^= 0x80;
    EXPECT_THROW(cc_->unseal(blob), std::runtime_error);
}

TEST_F(ConfidentialComputingTest, UnsealWithWrongTeeTypeThrows) {
    std::vector<uint8_t> plaintext = {0x01, 0x02};
    auto blob = cc_->seal(plaintext);

    // Change the stored tee_type to something different from what cc_ uses
    auto detected = cc_->detect();
    if (detected.type == TeeType::NONE) {
        blob.tee_type = TeeType::INTEL_TDX; // force mismatch
    } else {
        blob.tee_type = TeeType::NONE; // force mismatch
    }
    EXPECT_THROW(cc_->unseal(blob), std::runtime_error);
}

// ── teeTypeToString helper tests ──────────────────────────────────────────────

TEST(TeeTypeToStringTest, NoneReturnsNone) {
    EXPECT_EQ(teeTypeToString(TeeType::NONE), "None");
}

TEST(TeeTypeToStringTest, IntelTdxReturnsCorrectString) {
    EXPECT_EQ(teeTypeToString(TeeType::INTEL_TDX), "Intel TDX");
}

TEST(TeeTypeToStringTest, AmdSevReturnsCorrectString) {
    EXPECT_EQ(teeTypeToString(TeeType::AMD_SEV), "AMD SEV");
}

TEST(TeeTypeToStringTest, AmdSevSnpReturnsCorrectString) {
    EXPECT_EQ(teeTypeToString(TeeType::AMD_SEV_SNP), "AMD SEV-SNP");
}

// ── Multiple independent instances ───────────────────────────────────────────

TEST(ConfidentialComputingMultiInstanceTest, TwoInstancesHaveIndependentKeys) {
    // Each create() call generates a fresh sealing key; blobs from one
    // instance must not be unsealed by another.
    auto cc1 = ConfidentialComputing::create();
    auto cc2 = ConfidentialComputing::create();
    ASSERT_NE(cc1, nullptr);
    ASSERT_NE(cc2, nullptr);

    std::vector<uint8_t> plaintext = {0x42, 0x43, 0x44};
    auto blob = cc1->seal(plaintext);

    // Unseal with cc2 should throw (different sealing key)
    EXPECT_THROW(cc2->unseal(blob), std::runtime_error);
}
