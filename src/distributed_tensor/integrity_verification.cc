// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/integrity_verification.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace distributed_tensor {

// Helper: Format current timestamp as ISO 8601 string.
static std::string get_iso8601_timestamp() noexcept {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss = {};
  oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

// Helper: Simple SHA256 simulation (basic hash for demo purposes).
// In production, use actual cryptographic library (OpenSSL, libsodium, etc.).
static std::string simple_sha256_hash(const std::string& input) noexcept {
  // Simple hash: XOR-based hash for demonstration.
  uint64_t hash = 0xCBF29CE484222325ULL; // FNV-1a offset basis
  for (unsigned char c : input) {
    hash ^= c;
    hash *= 0x100000001B3ULL; // FNV-1a prime
  }

  std::ostringstream oss = {};
  oss << std::hex << std::setfill('0') << std::setw(64) << hash;
  std::string result = oss.str();

  // Pad to 64 characters (256 bits / 4 bits per hex digit).
  while (result.length() < 64) {
    result = "0" + result;
  }

  return result.substr(result.length() - 64);
}

// IntegrityVerificationReceipt implementation.

IntegrityVerificationReceipt::IntegrityVerificationReceipt(
    const std::string& artifact_id,
    HashAlgorithm algorithm)
    : artifact_id_(artifact_id),
      algorithm_(algorithm),
      merkle_root_hash_(""),
      is_verified_(false),
      verified_at_("") {}

// IntegrityVerificationEngine implementation.

IntegrityVerificationEngine::IntegrityVerificationEngine()
    : algorithm_(HashAlgorithm::SHA256) {}

IntegrityVerificationEngine::IntegrityVerificationEngine(
    HashAlgorithm algorithm)
    : algorithm_(algorithm) {}

// DefaultIntegrityVerificationEngine implementation.

DefaultIntegrityVerificationEngine::DefaultIntegrityVerificationEngine()
    : IntegrityVerificationEngine(HashAlgorithm::SHA256) {}

DefaultIntegrityVerificationEngine::DefaultIntegrityVerificationEngine(
    HashAlgorithm algorithm)
    : IntegrityVerificationEngine(algorithm) {}

std::string DefaultIntegrityVerificationEngine::compute_hash(
    const std::string& data) const noexcept {
  return simple_sha256_hash(data);
}

IntegrityVerificationReceipt
DefaultIntegrityVerificationEngine::compute_verification(
    const ArtifactManifest& manifest) const noexcept {
  IntegrityVerificationReceipt receipt(manifest.artifact_id(), algorithm_);
  receipt.set_verified_at(get_iso8601_timestamp());

  if (!manifest.is_complete()) {
    receipt.mark_failed(
        "Manifest is incomplete; refusing to generate an integrity receipt.");
    return receipt;
  }

  // Collect shard hashes.
  std::vector<std::pair<std::string, std::string>> shard_hashes;
  for (const auto& shard : manifest.shard_placements()) {
    if (!shard.shard_content_hash.empty()) {
      shard_hashes.emplace_back(shard.shard_id, shard.shard_content_hash);
      receipt.add_shard_hash(shard.shard_id, shard.shard_content_hash);
    }
  }

  if (shard_hashes.size() != manifest.shard_placements().size()) {
    receipt.mark_failed(
        "At least one shard is missing a content hash; receipt coverage is partial.");
    return receipt;
  }

  if (shard_hashes.empty()) {
    receipt.mark_failed("No shard hashes are available for verification.");
    return receipt;
  }

  // Build Merkle tree.
  auto merkle_nodes = build_merkle_tree(shard_hashes);
  for (auto& node : merkle_nodes) {
    receipt.add_merkle_tree_node(std::move(node));
  }

  // Compute Merkle root hash.
  std::string root_input = manifest.artifact_id() + ":" +
                           manifest.content_hash() + ":" +
                           manifest.version();
  for (const auto& [shard_id, hash] : shard_hashes) {
    root_input += "|" + shard_id + ":" + hash;
  }

  receipt.set_merkle_root_hash(compute_hash(root_input));
  receipt.mark_verified();

  return receipt;
}

bool DefaultIntegrityVerificationEngine::verify_integrity(
    const ArtifactManifest& manifest,
    const IntegrityVerificationReceipt& receipt) const noexcept {
  if (manifest.artifact_id() != receipt.artifact_id() || !receipt.is_verified() ||
      !receipt.verification_error().empty()) {
    return false;
  }

  auto expected_receipt = compute_verification(manifest);
  if (!expected_receipt.is_verified()) {
    return false;
  }

  if (expected_receipt.merkle_root_hash() != receipt.merkle_root_hash()) {
    return false;
  }

  if (expected_receipt.shard_hashes().size() != receipt.shard_hashes().size()) {
    return false;
  }

  for (const auto& [shard_id, shard_hash] : expected_receipt.shard_hashes()) {
    auto provided_hash = receipt.get_shard_hash(shard_id);
    if (!provided_hash || *provided_hash != shard_hash) {
      return false;
    }
  }

  return true;
}

bool DefaultIntegrityVerificationEngine::verify_shard(
    const std::string& shard_data,
    const std::string& expected_hash) const noexcept {
  if (expected_hash.empty()) {
    return false;
  }

  std::string computed_hash = compute_hash(shard_data);
  return computed_hash == expected_hash;
}

std::vector<MerkleTreeNode>
DefaultIntegrityVerificationEngine::build_merkle_tree(
    const std::vector<std::pair<std::string, std::string>>& shard_hashes)
    const noexcept {
  std::vector<MerkleTreeNode> nodes;

  // Level 0: Leaf nodes (one per shard).
  std::vector<std::string> current_level_hashes;
  uint32_t index = 0;
  for (const auto& [shard_id, hash] : shard_hashes) {
    MerkleTreeNode leaf;
    leaf.hash = hash;
    leaf.level = 0;
    leaf.index = index++;
    leaf.custom_metadata["shard_id"] = shard_id;
    nodes.push_back(leaf);
    current_level_hashes.push_back(hash);
  }

  // Build tree levels (bottom-up).
  uint32_t level = 1;
  index = 0;
  while (current_level_hashes.size() > 1) {
    std::vector<std::string> next_level_hashes;

    for (size_t i = 0; i < current_level_hashes.size(); i += 2) {
      std::string combined = current_level_hashes[i];
      if (i + 1 < current_level_hashes.size()) {
        combined += current_level_hashes[i + 1];
      }

      std::string parent_hash = compute_hash(combined);
      next_level_hashes.push_back(parent_hash);

      MerkleTreeNode internal;
      internal.hash = parent_hash;
      internal.level = level;
      internal.index = index++;
      internal.children_hashes.push_back(current_level_hashes[i]);
      if (i + 1 < current_level_hashes.size()) {
        internal.children_hashes.push_back(current_level_hashes[i + 1]);
      }
      nodes.push_back(internal);
    }

    current_level_hashes = next_level_hashes;
    level++;
    index = 0;
  }

  return nodes;
}

}  // namespace distributed_tensor
}  // namespace themis
