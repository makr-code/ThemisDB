/**
 * @file integrity_verification.cc
 * @brief Implementation of integrity verification, Merkle structures, and receipt semantics.
 */

#include "../include/integrity_verification.h"

#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace themis {
namespace distributed_tensor {

namespace {

[[nodiscard]] bool isLowercaseHexCharacter(const unsigned char c) {
    return std::isdigit(c) != 0 || (c >= 'a' && c <= 'f');
}

[[nodiscard]] bool isLikelyArtifactIdentifier(std::string_view artifact_id) {
    if (artifact_id.empty()) {
        return false;
    }

    return std::none_of(artifact_id.begin(), artifact_id.end(), [](const char c) {
        return std::iscntrl(static_cast<unsigned char>(c)) != 0;
    });
}

[[nodiscard]] json canonicalizeJson(const json& value) {
    if (value.is_object()) {
        json canonical = json::object();
        for (auto it = value.begin(); it != value.end(); ++it) {
            canonical[it.key()] = canonicalizeJson(it.value());
        }
        return canonical;
    }

    if (value.is_array()) {
        json canonical = json::array();
        for (const auto& element : value) {
            canonical.push_back(canonicalizeJson(element));
        }
        return canonical;
    }

    return value;
}

[[nodiscard]] std::optional<std::string> getRequiredString(
    const json& j,
    std::string_view key) {
    const auto iter = j.find(key);
    if (iter == j.end() || !iter->is_string()) {
        return std::nullopt;
    }

    return iter->get<std::string>();
}

[[nodiscard]] json serializeProofPath(
    const std::vector<MerkleProofComponent>& proof_path) {
    auto serialized = json::array();
    for (const auto& component : proof_path) {
        serialized.push_back(component.toJSON());
    }
    return serialized;
}

}  // namespace

// ============================================================================
// ContentHash Implementation
// ============================================================================

bool ContentHash::isValid() const {
    // SHA-256 produces 32 bytes = 64 hex characters (lowercase)
    if (value.size() != 64) {
      return false;
    }
    for (char c : value) {
        if (!isLowercaseHexCharacter(static_cast<unsigned char>(c))) {
          return false;
        }
    }
    return true;
}

// ============================================================================
// Merkle Proof Implementation
// ============================================================================

json MerkleProofComponent::toJSON() const {
    return json{
        {"sibling_hash", sibling_hash},
        {"is_left", is_left},
    };
}

std::optional<MerkleProofComponent> MerkleProofComponent::fromJSON(const json& j) {
    if (!j.is_object()) {
        return std::nullopt;
    }

    const auto sibling_hash = getRequiredString(j, "sibling_hash");
    const auto is_left_iter = j.find("is_left");
    if (!sibling_hash.has_value() || is_left_iter == j.end() ||
        !is_left_iter->is_boolean()) {
        return std::nullopt;
    }

    return MerkleProofComponent{
        .sibling_hash = *sibling_hash,
        .is_left = is_left_iter->get<bool>(),
    };
}

bool MerkleProof::verify(const std::string& expected_root) const {
    if (!isLikelyArtifactIdentifier(artifact_id)) {
        spdlog::warn("MerkleProof::verify: invalid artifact_id");
        return false;
    }
    if (!isValidSHA256Hex(fragment_hash) || fragment_hash.empty()) {
        spdlog::warn("MerkleProof::verify: invalid fragment_hash");
        return false;
    }
    // Support compatibility with older tests that set `root_hash` directly.
    const std::string& actual_artifact_root = !artifact_root_hash.empty() ? artifact_root_hash : root_hash;
    if (!isValidSHA256Hex(actual_artifact_root) || actual_artifact_root.empty()) {
        spdlog::warn("MerkleProof::verify: invalid artifact_root_hash/root_hash");
        return false;
    }
    if (!isValidSHA256Hex(expected_root) || expected_root.empty()) {
        spdlog::warn("MerkleProof::verify: invalid expected_root");
        return false;
    }

    // Start with fragment hash and work up the tree
    std::string current_hash = fragment_hash;

    for (const auto& component : proof_path) {
        if (!isValidSHA256Hex(component.sibling_hash)) {
            spdlog::warn("MerkleProof::verify: invalid sibling_hash in proof path");
            return false;
        }

        // Concatenate hashes in the correct order (left sibling first, then right)
        std::string concat = {};
        if (component.is_left) {
            concat = component.sibling_hash + current_hash;
        } else {
            concat = current_hash + component.sibling_hash;
        }

        // Compute hash of concatenation
        current_hash = computeSHA256(concat);
    }

    // Verify final hash matches expected root
    bool verified = (current_hash == actual_artifact_root) &&
                    (actual_artifact_root == expected_root);

    if (!verified) {
        spdlog::debug("MerkleProof::verify: computed root {} does not match expected {}",
                      current_hash, expected_root);
    }

    return verified;
}

json MerkleProof::toJSON() const {
    return json{
        {"artifact_id", artifact_id},
        {"fragment_index", fragment_index},
        {"fragment_hash", fragment_hash},
        {"proof_path", serializeProofPath(proof_path)},
        {"artifact_root_hash", artifact_root_hash},
    };
}

std::optional<MerkleProof> MerkleProof::fromJSON(const json& j) {
    if (!j.is_object()) {
        return std::nullopt;
    }

    const auto artifact_id = getRequiredString(j, "artifact_id");
    const auto fragment_hash = getRequiredString(j, "fragment_hash");
    const auto artifact_root_hash = getRequiredString(j, "artifact_root_hash");
    const auto fragment_index_iter = j.find("fragment_index");
    const auto proof_path_iter = j.find("proof_path");
    if (!artifact_id.has_value() || !fragment_hash.has_value() ||
        !artifact_root_hash.has_value() || fragment_index_iter == j.end() ||
        !fragment_index_iter->is_number_unsigned() ||
        proof_path_iter == j.end() || !proof_path_iter->is_array()) {
        return std::nullopt;
    }
    if (!isLikelyArtifactIdentifier(*artifact_id) ||
        !isValidSHA256Hex(*fragment_hash) ||
        !isValidSHA256Hex(*artifact_root_hash)) {
        return std::nullopt;
    }

    std::vector<MerkleProofComponent> proof_path;
    proof_path.reserve(proof_path_iter->size());
    for (const auto& component_json : *proof_path_iter) {
        auto component = MerkleProofComponent::fromJSON(component_json);
        if (!component.has_value()) {
            return std::nullopt;
        }
        proof_path.push_back(std::move(*component));
    }

    MerkleProof proof{
        .artifact_id = *artifact_id,
        .fragment_index = fragment_index_iter->get<uint64_t>(),
        .fragment_hash = *fragment_hash,
        .proof_path = std::move(proof_path),
        .artifact_root_hash = *artifact_root_hash,
    };
    // Keep compatibility alias in sync.
    proof.root_hash = proof.artifact_root_hash;
    return proof;
}

// ============================================================================
// VerificationReceipt Implementation
// ============================================================================

std::string VerificationReceipt::computeContentHash() const {
    json j;
    j["receipt_id"] = receipt_id;
    j["artifact_id"] = artifact_id;
    j["content_hash"] = content_hash;
    j["timestamp"] = timestamp;
    j["parent_receipt_hash"] = parent_receipt_hash;
    j["package_lineage_hash"] = package_lineage_hash;
    j["shard_placement_id"] = shard_placement_id;
    j["metadata"] = metadata;

    return computeJSONHash(j);
}

bool VerificationReceipt::verifyIntegrity() const {
    return isValidSHA256Hex(receipt_hash) && receipt_hash == computeContentHash();
}

json VerificationReceipt::toJSON() const {
    return json{
        {"receipt_id", receipt_id},
        {"artifact_id", artifact_id},
        {"content_hash", content_hash},
        {"timestamp", timestamp},
        {"parent_receipt_hash", parent_receipt_hash},
        {"receipt_hash", receipt_hash},
        {"package_lineage_hash", package_lineage_hash},
        {"shard_placement_id", shard_placement_id},
        {"metadata", metadata},
    };
}

std::optional<VerificationReceipt> VerificationReceipt::fromJSON(const json& j) {
    if (!j.is_object()) {
        return std::nullopt;
    }

    const auto receipt_id = getRequiredString(j, "receipt_id");
    const auto artifact_id = getRequiredString(j, "artifact_id");
    const auto content_hash = getRequiredString(j, "content_hash");
    const auto timestamp = getRequiredString(j, "timestamp");
    const auto parent_receipt_hash = getRequiredString(j, "parent_receipt_hash");
    const auto receipt_hash = getRequiredString(j, "receipt_hash");
    const auto package_lineage_hash = getRequiredString(j, "package_lineage_hash");
    const auto shard_placement_id = getRequiredString(j, "shard_placement_id");
    const auto metadata_iter = j.find("metadata");
    if (!receipt_id.has_value() || !artifact_id.has_value() ||
        !content_hash.has_value() || !timestamp.has_value() ||
        !parent_receipt_hash.has_value() || !receipt_hash.has_value() ||
        !package_lineage_hash.has_value() || !shard_placement_id.has_value() ||
        metadata_iter == j.end()) {
        return std::nullopt;
    }
    if (receipt_id->empty() || !isLikelyArtifactIdentifier(*artifact_id) ||
        !isValidSHA256Hex(*content_hash) || timestamp->empty() ||
        (!parent_receipt_hash->empty() && !isValidSHA256Hex(*parent_receipt_hash)) ||
        !isValidSHA256Hex(*receipt_hash)) {
        return std::nullopt;
    }

    return VerificationReceipt{
        .receipt_id = *receipt_id,
        .artifact_id = *artifact_id,
        .content_hash = *content_hash,
        .timestamp = *timestamp,
        .parent_receipt_hash = *parent_receipt_hash,
        .receipt_hash = *receipt_hash,
        .package_lineage_hash = *package_lineage_hash,
        .shard_placement_id = *shard_placement_id,
        .metadata = *metadata_iter,
    };
}

// ============================================================================
// ReceiptChain Implementation
// ============================================================================

VerificationReceipt ReceiptChain::appendReceipt(VerificationReceipt receipt) {
    // Determine parent receipt hash
    std::string parent_hash = {};
    if (!receipts_.empty()) {
        parent_hash = receipts_.back().receipt_hash;
    }

    // Update receipt with parent linkage
    receipt.parent_receipt_hash = parent_hash;

    // Compute receipt hash
    receipt.receipt_hash = receipt.computeContentHash();

    // Append to chain
    receipts_.push_back(receipt);

    spdlog::debug("ReceiptChain::appendReceipt: appended receipt {} for artifact {}",
                  receipt.receipt_id, receipt.artifact_id);

    return receipt;
}

std::vector<VerificationReceipt> ReceiptChain::getAllReceipts() const {
    return receipts_;
}

std::optional<VerificationReceipt> ReceiptChain::getHeadReceipt() const {
    if (receipts_.empty()) {
      return std::nullopt;
    }
    return receipts_.back();
}

std::optional<VerificationReceipt> ReceiptChain::getGenesisReceipt() const {
    if (receipts_.empty()) {
      return std::nullopt;
    }
    return receipts_.front();
}

bool ReceiptChain::verifyChainIntegrity() const {
    if (receipts_.empty()) {
        // Empty chain is trivially valid
        return true;
    }

    // Verify the first (genesis) receipt has empty parent
    if (!receipts_.front().parent_receipt_hash.empty()) {
        spdlog::error("ReceiptChain::verifyChainIntegrity: genesis receipt has non-empty parent");
        return false;
    }

    // Verify entire chain linkage
    std::string expected_previous = {};
    for (size_t i = 0; i < receipts_.size(); ++i) {
        const auto& receipt = receipts_[i];

        // Verify parent linkage
        if (receipt.parent_receipt_hash != expected_previous) {
            spdlog::error("ReceiptChain::verifyChainIntegrity: "
                         "parent_receipt_hash mismatch at entry {} (expected {}, got {})",
                         i, expected_previous, receipt.parent_receipt_hash);
            return false;
        }

        // Verify receipt integrity
        if (!receipt.verifyIntegrity()) {
            spdlog::error("ReceiptChain::verifyChainIntegrity: "
                         "receipt_hash mismatch at entry {}", i);
            return false;
        }

        // Update expected parent for next iteration
        expected_previous = receipt.receipt_hash;
    }

    return true;
}

size_t ReceiptChain::size() const {
    return receipts_.size();
}

json ReceiptChain::toJSON() const {
    return json{
        {"receipts", [&]() {
            auto serialized = json::array();
            for (const auto& receipt : receipts_) {
                serialized.push_back(receipt.toJSON());
            }
            return serialized;
        }()},
        {"metadata", toManifestMetadataJSON()},
    };
}

json ReceiptChain::toManifestMetadataJSON() const {
    const auto head = getHeadReceipt();
    const auto genesis = getGenesisReceipt();

    return json{
        {"chain_length", receipts_.size()},
        {"head_receipt_id", head.has_value() ? head->receipt_id : std::string{}},
        {"head_receipt_hash", head.has_value() ? head->receipt_hash : std::string{}},
        {"genesis_receipt_id", genesis.has_value() ? genesis->receipt_id : std::string{}},
        {"genesis_receipt_hash", genesis.has_value() ? genesis->receipt_hash : std::string{}},
    };
}

std::optional<ReceiptChain> ReceiptChain::fromJSON(const json& j) {
    const json* receipts_json = &j;
    if (j.is_object()) {
        const auto iter = j.find("receipts");
        if (iter == j.end()) {
            return std::nullopt;
        }
        receipts_json = &(*iter);
    }

    if (!receipts_json->is_array()) {
        return std::nullopt;
    }

    ReceiptChain chain;
    for (const auto& receipt_json : *receipts_json) {
        auto receipt = VerificationReceipt::fromJSON(receipt_json);
        if (!receipt.has_value()) {
            return std::nullopt;
        }
        chain.receipts_.push_back(std::move(*receipt));
    }

    if (!chain.verifyChainIntegrity()) {
        return std::nullopt;
    }

    return chain;
}

// ============================================================================
// VerificationState Utilities
// ============================================================================

std::string verificationStateToString(VerificationState state) {
    switch (state) {
        case VerificationState::UNVERIFIED:
            return "UNVERIFIED";
        case VerificationState::VERIFIED:
            return "VERIFIED";
        case VerificationState::VERIFIED_FRAGMENTS:
            return "VERIFIED_FRAGMENTS";
        case VerificationState::CORRUPT:
            return "CORRUPT";
        case VerificationState::STALE:
            return "STALE";
        default:
            return "UNKNOWN";
    }
}

std::optional<VerificationState> stringToVerificationState(
    const std::string& s) {
    if (s == "UNVERIFIED") {
      return VerificationState::UNVERIFIED;
    }
    if (s == "VERIFIED") {
      return VerificationState::VERIFIED;
    }
    if (s == "VERIFIED_FRAGMENTS") {
      return VerificationState::VERIFIED_FRAGMENTS;
    }
    if (s == "CORRUPT") {
      return VerificationState::CORRUPT;
    }
    if (s == "STALE") {
      return VerificationState::STALE;
    }
    return std::nullopt;
}

// ============================================================================
// PHASE 3: Error Handling & Edge Cases Implementation
// ============================================================================

namespace {
    IntegrityRecoveryHook* g_recovery_hook = nullptr;
}

VerificationResult verifyArtifactIntegrity(
    const std::string& artifact_id,
    std::string_view payload,
    const std::string& expected_content_hash,
    const std::optional<MerkleProof>& merkle_proof,
    const std::optional<ReceiptChain>& receipt_chain,
    ProvenanceVerificationHook* provenance_hook) {
    
    VerificationResult result;
    result.artifact_id = artifact_id;
    result.expected_hash = expected_content_hash;
    
    // Step 1: Validate expected hash format
    if (!isValidSHA256Hex(expected_content_hash)) {
        result.state = VerificationState::UNVERIFIED;
        result.error_messages.push_back(
            "Invalid expected content hash: not a valid SHA-256 hex");
        spdlog::warn("verifyArtifactIntegrity: invalid expected hash format for {}",
                    artifact_id);
        return result;
    }
    
    // Step 2: Compute actual content hash
    result.actual_hash = computeSHA256(payload);
    
    // Step 3: Check content hash match
    if (result.actual_hash != expected_content_hash) {
        result.state = VerificationState::CORRUPT;
        result.success = false;
        result.error_messages.push_back(
            fmt::format("Content hash mismatch: expected {}, got {}",
                       expected_content_hash, result.actual_hash));
        spdlog::error("verifyArtifactIntegrity: content hash mismatch for {}", artifact_id);
        
        // Attempt recovery coordination
        if (g_recovery_hook) {
            g_recovery_hook->requestArtifactRecovery(artifact_id, "content_hash_mismatch");
        }
        
        return result;
    }
    
    // Step 4: Verify fragment-level integrity if Merkle proof provided
    if (merkle_proof.has_value()) {
        if (!merkle_proof->verify(expected_content_hash)) {
            result.state = VerificationState::CORRUPT;
            result.success = false;
            result.error_messages.push_back("Merkle proof verification failed");
            spdlog::error("verifyArtifactIntegrity: Merkle proof invalid for {}", artifact_id);
            return result;
        }
        result.state = VerificationState::VERIFIED_FRAGMENTS;
        result.fragments_verified = merkle_proof->getProofDepth();
    } else {
        result.state = VerificationState::VERIFIED;
    }
    
    // Step 5: Verify receipt chain if provided
    if (receipt_chain.has_value()) {
        auto chain_result = detectReceiptChainTampering(*receipt_chain);
        if (!chain_result.success) {
            result.state = VerificationState::CORRUPT;
            result.success = false;
            result.error_messages.insert(
                result.error_messages.end(),
                chain_result.error_messages.begin(),
                chain_result.error_messages.end());
            spdlog::error("verifyArtifactIntegrity: receipt chain tampered for {}", artifact_id);
            return result;
        }
        
        // Check if receipt lineage is current
        auto head = receipt_chain->getHeadReceipt();
        if (head.has_value() && provenance_hook) {
            auto current_lineage = provenance_hook->getCurrentPackageLineage();
            if (!current_lineage.empty() && 
                head->package_lineage_hash != current_lineage) {
                result.state = VerificationState::STALE;
                result.error_messages.push_back(
                    "Receipt chain lineage is outdated (rebuild recommended)");
                spdlog::info("verifyArtifactIntegrity: stale lineage for {}", artifact_id);
            }
        }
    }
    
    // Step 6: Provenance verification if hook provided
    if (provenance_hook && result.state != VerificationState::CORRUPT) {
        std::string receipt_lineage_hash = {};
        if (receipt_chain.has_value()) {
            const auto head_receipt = receipt_chain->getHeadReceipt();
            if (head_receipt.has_value()) {
                receipt_lineage_hash = head_receipt->package_lineage_hash;
            }
        }

        auto lineage_result = provenance_hook->verifyProvenance(
            artifact_id,
            result.actual_hash,
            receipt_lineage_hash);
        
        if (!lineage_result.success && result.state == VerificationState::VERIFIED) {
            result.state = VerificationState::STALE;
            result.error_messages.push_back("Provenance lineage mismatch");
        }
    }
    
    result.success = (result.state == VerificationState::VERIFIED ||
                     result.state == VerificationState::VERIFIED_FRAGMENTS);
    spdlog::info("verifyArtifactIntegrity: artifact {} verification complete (state={})",
                artifact_id, verificationStateToString(result.state));
    
    return result;
}

VerificationResult detectReceiptChainTampering(const ReceiptChain& chain) {
    VerificationResult result;
    result.state = VerificationState::VERIFIED;
    result.success = true;
    
    if (chain.empty()) {
        return result;  // Empty chain is valid
    }
    
    const auto all_receipts = chain.getAllReceipts();
    
    // Verify genesis receipt has empty parent
    if (!all_receipts.front().parent_receipt_hash.empty()) {
        result.state = VerificationState::CORRUPT;
        result.success = false;
        result.error_messages.push_back(
            "Genesis receipt has non-empty parent_receipt_hash");
        spdlog::error("detectReceiptChainTampering: genesis receipt integrity failed");
        return result;
    }
    
    // Verify entire chain linkage
    std::string expected_previous = {};
    for (size_t i = 0; i < all_receipts.size(); ++i) {
        const auto& receipt = all_receipts[i];
        
        // Verify self-integrity
        if (!receipt.verifyIntegrity()) {
            result.state = VerificationState::CORRUPT;
            result.success = false;
            result.error_messages.push_back(
                fmt::format("Receipt {} content hash mismatch (tampered)", i));
            spdlog::error("detectReceiptChainTampering: receipt {} self-integrity failed", i);
            return result;
        }
        
        // Verify parent linkage
        if (receipt.parent_receipt_hash != expected_previous) {
            result.state = VerificationState::CORRUPT;
            result.success = false;
            result.error_messages.push_back(
                fmt::format("Receipt {} parent_receipt_hash mismatch (tampering detected)", i));
            spdlog::error("detectReceiptChainTampering: parent linkage broken at entry {}", i);
            return result;
        }
        
        // Update expected parent for next iteration
        expected_previous = receipt.receipt_hash;
    }
    
    result.fragments_verified = all_receipts.size();
    spdlog::info("detectReceiptChainTampering: chain integrity verified ({} receipts)",
                all_receipts.size());
    
    return result;
}

VerificationResult handlePartialReceiptChain(
    const ReceiptChain& partial_chain,
    const std::string& artifact_id,
    const std::string& current_hash) {
    
    VerificationResult result;
    result.artifact_id = artifact_id;
    result.expected_hash = current_hash;
    
    if (partial_chain.empty()) {
        result.state = VerificationState::UNVERIFIED;
        result.error_messages.push_back("Receipt chain is empty; cannot verify history");
        spdlog::warn("handlePartialReceiptChain: empty chain for {}", artifact_id);
        return result;
    }
    
    // Get head receipt
    auto head = partial_chain.getHeadReceipt();
    if (!head.has_value()) {
        result.state = VerificationState::UNVERIFIED;
        result.error_messages.push_back("Cannot retrieve head receipt");
        return result;
    }
    
    // Check if head receipt's content hash matches current
    if (head->content_hash != current_hash) {
        result.state = VerificationState::CORRUPT;
        result.success = false;
        result.error_messages.push_back(
            fmt::format("Head receipt content_hash ({}) does not match current ({})",
                       head->content_hash, current_hash));
        spdlog::error("handlePartialReceiptChain: head receipt mismatch for {}", artifact_id);
        
        if (g_recovery_hook) {
            g_recovery_hook->requestChainRebuild(artifact_id);
        }
        
        return result;
    }
    
    // Verify integrity of available chain
    if (!partial_chain.verifyChainIntegrity()) {
        result.state = VerificationState::CORRUPT;
        result.success = false;
        result.error_messages.push_back("Partial receipt chain integrity check failed");
        spdlog::error("handlePartialReceiptChain: chain verification failed for {}", artifact_id);
        
        if (g_recovery_hook) {
            g_recovery_hook->requestChainRebuild(artifact_id);
        }
        
        return result;
    }
    
    // Mark as STALE since history is incomplete
    result.state = VerificationState::STALE;
    result.success = true;
    result.fragments_verified = partial_chain.size();
    result.error_messages.push_back(
        fmt::format("Partial receipt chain: {} receipts available (history incomplete)",
                   partial_chain.size()));
    
    spdlog::info("handlePartialReceiptChain: partial history accepted for {} "
                "({} receipts available)", artifact_id, partial_chain.size());
    
    return result;
}

VerificationResult handleStaleReceipt(
    const VerificationReceipt& head_receipt,
    const std::string& current_lineage_hash,
    const std::string& content_hash) {
    
    VerificationResult result;
    result.artifact_id = head_receipt.artifact_id;
    result.expected_hash = content_hash;
    result.actual_hash = head_receipt.content_hash;
    
    // Verify receipt self-integrity
    if (!head_receipt.verifyIntegrity()) {
        result.state = VerificationState::CORRUPT;
        result.success = false;
        result.error_messages.push_back("Receipt self-integrity failed (tampering detected)");
        spdlog::error("handleStaleReceipt: receipt integrity failed for {}",
                     head_receipt.artifact_id);
        return result;
    }
    
    // Check content hash match
    if (head_receipt.content_hash != content_hash) {
        result.state = VerificationState::CORRUPT;
        result.success = false;
        result.error_messages.push_back(
            fmt::format("Receipt content_hash mismatch: receipt={}, current={}",
                       head_receipt.content_hash, content_hash));
        spdlog::error("handleStaleReceipt: content mismatch for {}", head_receipt.artifact_id);
        return result;
    }
    
    // Check lineage
    if (!current_lineage_hash.empty() &&
        head_receipt.package_lineage_hash != current_lineage_hash) {
        result.state = VerificationState::STALE;
        result.success = true;
        result.error_messages.push_back(
            fmt::format("Package lineage changed: receipt={}, current={}",
                       head_receipt.package_lineage_hash.substr(0, 8),
                       current_lineage_hash.substr(0, 8)));
        spdlog::info("handleStaleReceipt: stale lineage detected for {}",
                    head_receipt.artifact_id);
        
        if (g_recovery_hook) {
            g_recovery_hook->requestChainRebuild(head_receipt.artifact_id);
        }
        
        return result;
    }
    
    // Content and lineage both match
    result.state = VerificationState::VERIFIED;
    result.success = true;
    result.fragments_verified = 1;
    spdlog::info("handleStaleReceipt: receipt verified for {}", head_receipt.artifact_id);
    
    return result;
}

VerificationResult verifyFragmentIntegrity(
    const std::string& artifact_id,
    std::string_view fragment_data,
    size_t fragment_index,
    const MerkleProof& merkle_proof,
    const std::string& expected_root) {
    
    VerificationResult result;
    result.artifact_id = artifact_id;
    result.expected_hash = expected_root;
    result.state = VerificationState::UNVERIFIED;
    
    // Compute fragment hash
    result.actual_hash = computeSHA256(fragment_data);
    
    // Verify fragment against Merkle proof
    if (!merkle_proof.verify(expected_root)) {
        result.state = VerificationState::CORRUPT;
        result.success = false;
        result.error_messages.push_back(
            fmt::format("Fragment {} Merkle proof verification failed", fragment_index));
        spdlog::error("verifyFragmentIntegrity: fragment {} proof invalid for {}",
                     fragment_index, artifact_id);
        return result;
    }
    
    result.state = VerificationState::VERIFIED_FRAGMENTS;
    result.success = true;
    result.fragments_verified = 1;
    spdlog::info("verifyFragmentIntegrity: fragment {} verified for {}",
                fragment_index, artifact_id);
    
    return result;
}

void setIntegrityRecoveryHook(IntegrityRecoveryHook* hook) {
    g_recovery_hook = hook;
    if (hook) {
        spdlog::info("Integrity recovery hook installed");
    } else {
        spdlog::info("Integrity recovery hook removed");
    }
}

IntegrityRecoveryHook* getIntegrityRecoveryHook() {
    return g_recovery_hook;
}



// ============================================================================
// VerificationResult Implementation
// ============================================================================

json VerificationResult::toJSON() const {
    json j;
    j["success"] = success;
    j["state"] = verificationStateToString(state);
    j["artifact_id"] = artifact_id;
    j["expected_hash"] = expected_hash;
    j["actual_hash"] = actual_hash;
    j["error_messages"] = error_messages;
    j["fragments_verified"] = fragments_verified;
    j["metadata"] = metadata;
    return j;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string computeSHA256(const std::string& data) {
    return computeSHA256(std::string_view(data));
}

std::string computeSHA256(std::string_view data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(hash, &sha256);

    // Convert to hex string (lowercase)
    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(hash[i]);
    }

    return oss.str();
}

std::string computeSHA256(const char* data) {
    if (data == nullptr) {
      return std::string();
    }
    return computeSHA256(std::string_view(data));
}

std::string computeJSONHash(const json& j) {
    // Deterministic JSON serialization (sorted keys, no whitespace)
    std::string canonical = canonicalizeJson(j).dump(
        -1,  // compact output
        ' ', // indent (not used with -1)
        false,  // ensure ASCII output
        json::error_handler_t::strict
    );

    return computeSHA256(canonical);
}

bool isValidSHA256Hex(std::string_view hex_str) {
    // SHA-256 produces 32 bytes = 64 hex characters
    if (hex_str.size() != 64) {
      return false;
    }

    for (char c : hex_str) {
        if (!isLowercaseHexCharacter(static_cast<unsigned char>(c))) {
          return false;
        }
    }

    return true;
}

}  // namespace distributed_tensor
}  // namespace themis
