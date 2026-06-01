/**
 * @file integrity_verification_test.cc
 * @brief Contract tests for IIntegrityVerifier (sub-issue #5432).
 *
 * Validates factory construction, checksum computation, stripe verification,
 * Merkle root building, receipt chain operations, and chain verification.
 * Production cryptographic implementations are tracked in sub-issue #5432.
 */

#include "distributed_tensor/include/integrity_verification.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace themis::distributed_tensor;

namespace {

std::vector<std::uint8_t> makeBytes(std::size_t n, std::uint8_t fill = 0xAB) {
    return std::vector<std::uint8_t>(n, fill);
}

} // namespace

class IntegrityVerifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        verifier_ = makeIntegrityVerifier();
        ASSERT_NE(verifier_, nullptr);
    }

    std::unique_ptr<IIntegrityVerifier> verifier_;
};

TEST_F(IntegrityVerifierTest, FactoryReturnsNonNull) {
    EXPECT_NE(verifier_, nullptr);
}

TEST_F(IntegrityVerifierTest, ChecksumDoesNotThrow) {
    EXPECT_NO_THROW(verifier_->checksum(makeBytes(256)));
}

TEST_F(IntegrityVerifierTest, ChecksumReturnsNonEmptyString) {
    std::string cs = verifier_->checksum(makeBytes(256));
    EXPECT_FALSE(cs.empty());
}

TEST_F(IntegrityVerifierTest, ChecksumIsDeterministic) {
    auto data = makeBytes(128);
    std::string cs1 = verifier_->checksum(data);
    std::string cs2 = verifier_->checksum(data);
    EXPECT_EQ(cs1, cs2);
}

TEST_F(IntegrityVerifierTest, ChecksumDiffersForDifferentData) {
    std::string cs1 = verifier_->checksum(makeBytes(64, 0x00));
    std::string cs2 = verifier_->checksum(makeBytes(64, 0xFF));
    EXPECT_NE(cs1, cs2);
}

TEST_F(IntegrityVerifierTest, VerifyStripePassWhenChecksumsMatch) {
    auto data = makeBytes(512);
    std::string cs = verifier_->checksum(data);
    StripeVerification result = verifier_->verifyStripe("art-1", 0, data, cs);
    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.artifact_id, "art-1");
    EXPECT_EQ(result.stripe_index, 0u);
}

TEST_F(IntegrityVerifierTest, VerifyStripeFailsWhenChecksumsDoNotMatch) {
    auto data = makeBytes(512);
    StripeVerification result =
        verifier_->verifyStripe("art-2", 1, data, "wrong-checksum");
    EXPECT_FALSE(result.passed);
}

TEST_F(IntegrityVerifierTest, BuildMerkleRootDoesNotThrow) {
    std::vector<std::string> checksums = {
        verifier_->checksum(makeBytes(128, 0x01)),
        verifier_->checksum(makeBytes(128, 0x02)),
        verifier_->checksum(makeBytes(128, 0x03)),
        verifier_->checksum(makeBytes(128, 0x04)),
    };
    EXPECT_NO_THROW(verifier_->buildMerkleRoot("art-3", checksums, 1));
}

TEST_F(IntegrityVerifierTest, BuildMerkleRootReturnsNonEmptyHash) {
    std::vector<std::string> checksums = {"cs1", "cs2"};
    MerkleRoot root = verifier_->buildMerkleRoot("art-4", checksums, 1);
    EXPECT_EQ(root.artifact_id, "art-4");
    EXPECT_FALSE(root.root_hash.empty());
}

TEST_F(IntegrityVerifierTest, AppendReceiptReturnsNonEmptyId) {
    std::string receipt_id = verifier_->appendReceipt("art-5", "merkle-hash-123");
    EXPECT_FALSE(receipt_id.empty());
}

TEST_F(IntegrityVerifierTest, LatestReceiptAfterAppend) {
    verifier_->appendReceipt("art-6", "hash-001");
    auto receipt = verifier_->latestReceipt("art-6");
    EXPECT_TRUE(receipt.has_value());
    EXPECT_EQ(receipt->artifact_id, "art-6");
}

TEST_F(IntegrityVerifierTest, LatestReceiptNulloptForUnknownArtifact) {
    auto receipt = verifier_->latestReceipt("unknown-artifact");
    EXPECT_FALSE(receipt.has_value());
}

TEST_F(IntegrityVerifierTest, VerifyChainReturnsTrueAfterAppend) {
    verifier_->appendReceipt("art-7", "hash-a");
    verifier_->appendReceipt("art-7", "hash-b");
    EXPECT_TRUE(verifier_->verifyChain("art-7"));
}

TEST_F(IntegrityVerifierTest, VerifyChainReturnsFalseForUnknownArtifact) {
    EXPECT_FALSE(verifier_->verifyChain("unknown-artifact"));
}
