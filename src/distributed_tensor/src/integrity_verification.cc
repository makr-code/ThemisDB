/**
 * @file integrity_verification.cc
 * @brief Integrity verifier and receipt-chain implementation stub.
 *
 * Skeleton: SHA-256 checksum computation and in-memory receipt chain.
 * Replace with hardware-accelerated hashing and persistent chain in #5432.
 */

#include "distributed_tensor/include/integrity_verification.h"

#include <iomanip>
#include <numeric>
#include <sstream>

namespace themis::distributed_tensor {

namespace {

/// Simple DJB2-based fingerprint used as a stand-in until SHA-256 is wired.
std::string djb2hex(const std::vector<std::uint8_t>& data) {
    std::uint64_t h = 5381;
    for (auto b : data) h = ((h << 5) + h) ^ b;
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << h;
    return ss.str();
    // TODO(#5432): Replace with real SHA-256 (e.g. via OpenSSL EVP).
}

class IntegrityVerifierImpl final : public IIntegrityVerifier {
public:
    std::string checksum(const std::vector<std::uint8_t>& data) const override {
        return djb2hex(data);
    }

    StripeVerification verifyStripe(
        const std::string& artifact_id,
        std::uint32_t stripe_index,
        const std::vector<std::uint8_t>& data,
        const std::string& expected_checksum) const override {
        StripeVerification v{
            .artifact_id       = artifact_id,
            .stripe_index      = stripe_index,
            .expected_checksum = expected_checksum,
            .actual_checksum   = checksum(data),
        };
        v.passed = (v.actual_checksum == v.expected_checksum);
        return v;
    }

    MerkleRoot buildMerkleRoot(
        const std::string& artifact_id,
        const std::vector<std::string>& stripe_checksums,
        std::uint64_t epoch) const override {
        // Concatenate all checksums and hash them.
        std::string combined;
        for (const auto& cs : stripe_checksums) combined += cs;
        std::vector<std::uint8_t> buf(combined.begin(), combined.end());
        return MerkleRoot{
            .artifact_id = artifact_id,
            .root_hash   = djb2hex(buf),
            .num_leaves  = static_cast<std::uint32_t>(stripe_checksums.size()),
            .epoch       = epoch,
        };
    }

    std::string appendReceipt(const std::string& artifact_id,
                               const std::string& merkle_root_hash) override {
        std::string prev_id = receipts_.empty() ? "" : receipts_.back().receipt_id;
        std::string new_id  = "receipt-" + std::to_string(receipts_.size() + 1);
        receipts_.push_back({
            .receipt_id           = new_id,
            .artifact_id          = artifact_id,
            .merkle_root_hash     = merkle_root_hash,
            .previous_receipt_id  = prev_id,
        });
        latest_by_artifact_[artifact_id] = receipts_.size() - 1;
        return new_id;
    }

    std::optional<IntegrityReceipt> latestReceipt(
        const std::string& artifact_id) const override {
        auto it = latest_by_artifact_.find(artifact_id);
        if (it == latest_by_artifact_.end()) return std::nullopt;
        return receipts_[it->second];
    }

    bool verifyChain(const std::string& /*artifact_id*/) const override {
        // TODO(#5432): Walk back the receipt chain and verify each link.
        return true;
    }

private:
    std::vector<IntegrityReceipt> receipts_;
    std::unordered_map<std::string, std::size_t> latest_by_artifact_;
};

} // namespace

std::unique_ptr<IIntegrityVerifier> makeIntegrityVerifier() {
    return std::make_unique<IntegrityVerifierImpl>();
}

} // namespace themis::distributed_tensor
