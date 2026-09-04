/**
 * @file integrity_verification.h
 * @brief Integrity verification for distributed tensor artifacts.
 *
 * Provides cryptographic hash verification for individual shards and
 * full artifact manifests to detect corruption during transfer or storage.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "distributed_tensor/artifact_manifest.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace themis {
namespace distributed_tensor {

/// @defgroup integrity_verification Integrity Verification
/// @brief Integrity verification, Merkle trees, and receipt generation.
/// @{

/// Hash algorithm enumeration.
///
/// Specifies the cryptographic hash algorithm used for integrity verification.
enum class HashAlgorithm {
  /// SHA-256 cryptographic hash.
  SHA256,

  /// BLAKE3 cryptographic hash (faster, better parallelization).
  BLAKE3,

  /// BLAKE2b cryptographic hash.
  BLAKE2B,

  /// Secure Hash Algorithm 3 (Keccak).
  SHA3_256,
};

/// Merkle tree node representing a level in the tree structure.
///
/// The Merkle tree allows efficient incremental verification and
/// pinpoints corrupted blocks.
struct MerkleTreeNode {
  /// Hash value of this node.
  std::string hash = {};

  /// Level in the tree (0 = leaf, higher = internal nodes).
  uint32_t level = 0;

  /// Index at this level.
  uint32_t index = 0;

  /// Child node hashes (for internal nodes).
  std::vector<std::string> children_hashes;

  /// Custom metadata.
  std::unordered_map<std::string, std::string> custom_metadata;
};

/// Integrity verification receipt.
///
/// Provides cryptographic proof that an artifact and its shards have
/// not been corrupted or modified.
class IntegrityVerificationReceipt {
 public:
  /// Construct a verification receipt for an artifact.
  ///
  /// @param artifact_id Artifact identifier.
  /// @param algorithm Hash algorithm used.
  IntegrityVerificationReceipt(const std::string& artifact_id,
                               HashAlgorithm algorithm);

  /// Copy constructor deleted.
  IntegrityVerificationReceipt(const IntegrityVerificationReceipt&) = delete;

  /// Move constructor.
  IntegrityVerificationReceipt(IntegrityVerificationReceipt&&) noexcept =
      default;

  /// Assignment operator deleted.
  IntegrityVerificationReceipt& operator=(const IntegrityVerificationReceipt&) =
      delete;

  /// Move assignment operator.
  IntegrityVerificationReceipt& operator=(
      IntegrityVerificationReceipt&&) noexcept = default;

  /// Destructor.
  ~IntegrityVerificationReceipt() = default;

  /// Return the artifact ID.
  const std::string& artifact_id() const noexcept { return artifact_id_; }

  /// Return the hash algorithm.
  HashAlgorithm algorithm() const noexcept { return algorithm_; }

  /// Return the Merkle root hash.
  const std::string& merkle_root_hash() const noexcept {
    return merkle_root_hash_;
  }

  /// Set the Merkle root hash.
  void set_merkle_root_hash(const std::string& hash) noexcept {
    merkle_root_hash_ = hash;
  }

  /// Add a shard hash.
  ///
  /// @param shard_id Shard identifier.
  /// @param hash Hash value for the shard.
  void add_shard_hash(const std::string& shard_id,
                      const std::string& hash) noexcept {
    shard_hashes_[shard_id] = hash;
  }

  /// Retrieve shard hash.
  ///
  /// @param shard_id Shard identifier.
  /// @return Optional shard hash if found.
  std::optional<std::string> get_shard_hash(
      const std::string& shard_id) const noexcept {
    auto it = shard_hashes_.find(shard_id);
    if (it != shard_hashes_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  /// Return all shard hashes.
  const std::unordered_map<std::string, std::string>& shard_hashes()
      const noexcept {
    return shard_hashes_;
  }

  /// Add a Merkle tree node.
  ///
  /// @param node Merkle tree node.
  void add_merkle_tree_node(MerkleTreeNode node) noexcept {
    merkle_tree_nodes_.push_back(std::move(node));
  }

  /// Return all Merkle tree nodes.
  const std::vector<MerkleTreeNode>& merkle_tree_nodes() const noexcept {
    return merkle_tree_nodes_;
  }

  /// Mark the receipt as verified (all hashes match).
  void mark_verified() noexcept { is_verified_ = true; }

  /// Mark the receipt as failed.
  ///
  /// @param error Human-readable explanation of the blocking verification issue.
  void mark_failed(std::string error) noexcept {
    is_verified_ = false;
    verification_error_ = std::move(error);
  }

  /// Query if receipt is verified.
  bool is_verified() const noexcept { return is_verified_; }

  /// Return the timestamp of verification (ISO 8601).
  const std::string& verified_at() const noexcept { return verified_at_; }

  /// Set the verification timestamp.
  void set_verified_at(const std::string& timestamp) noexcept {
    verified_at_ = timestamp;
  }

  /// Return the blocking verification error, if any.
  const std::string& verification_error() const noexcept {
    return verification_error_;
  }

 private:
  /// Artifact identifier.
  std::string artifact_id_;

  /// Hash algorithm used.
  HashAlgorithm algorithm_;

  /// Merkle root hash of entire artifact.
  std::string merkle_root_hash_;

  /// Shard-level hashes (shard_id -> hash).
  std::unordered_map<std::string, std::string> shard_hashes_;

  /// Merkle tree nodes for incremental verification.
  std::vector<MerkleTreeNode> merkle_tree_nodes_;

  /// Verification flag: true if all hashes have been verified.
  bool is_verified_ = false;

  /// Timestamp of verification.
  std::string verified_at_;

  /// Blocking verification error for incomplete or invalid receipts.
  std::string verification_error_;
};

/// Integrity verification engine.
///
/// Computes and verifies cryptographic hashes for artifacts and shards.
class IntegrityVerificationEngine {
 public:
  /// Construct the engine with default SHA-256 algorithm.
  IntegrityVerificationEngine();

  /// Construct with explicit hash algorithm.
  ///
  /// @param algorithm Hash algorithm to use.
  explicit IntegrityVerificationEngine(HashAlgorithm algorithm);

  /// Copy constructor deleted.
  IntegrityVerificationEngine(const IntegrityVerificationEngine&) = delete;

  /// Move constructor.
  IntegrityVerificationEngine(IntegrityVerificationEngine&&) noexcept = default;

  /// Assignment operator deleted.
  IntegrityVerificationEngine& operator=(const IntegrityVerificationEngine&) =
      delete;

  /// Move assignment operator.
  IntegrityVerificationEngine& operator=(
      IntegrityVerificationEngine&&) noexcept = default;

  /// Virtual destructor.
  virtual ~IntegrityVerificationEngine() = default;

  /// Compute integrity verification receipt for an artifact manifest.
  ///
  /// This computes the Merkle root hash based on shard placements
  /// and their content hashes.
  ///
  /// @param manifest Artifact manifest to verify.
  /// @return Integrity verification receipt.
  virtual IntegrityVerificationReceipt compute_verification(
      const ArtifactManifest& manifest) const noexcept = 0;

  /// Verify integrity of an artifact against its receipt.
  ///
  /// @param manifest Artifact manifest.
  /// @param receipt Integrity verification receipt.
  /// @return true if all hashes match, false otherwise.
  virtual bool verify_integrity(const ArtifactManifest& manifest,
                                const IntegrityVerificationReceipt& receipt)
      const noexcept = 0;

  /// Verify integrity of a single shard.
  ///
  /// @param shard_data Shard content (raw bytes).
  /// @param expected_hash Expected hash value.
  /// @return true if shard hash matches expected value, false otherwise.
  virtual bool verify_shard(const std::string& shard_data,
                            const std::string& expected_hash) const noexcept = 0;

 protected:
  /// Hash algorithm used by this engine.
  HashAlgorithm algorithm_;

  /// Compute hash of arbitrary data.
  ///
  /// @param data Input data.
  /// @return Hash value as hexadecimal string.
  virtual std::string compute_hash(const std::string& data) const noexcept = 0;
};

/// Default integrity verification engine.
///
/// Uses SHA-256 for hashing and builds Merkle trees for incremental verification.
class DefaultIntegrityVerificationEngine : public IntegrityVerificationEngine {
 public:
  /// Construct with default SHA-256 algorithm.
  DefaultIntegrityVerificationEngine();

  /// Construct with explicit hash algorithm.
  ///
  /// @param algorithm Hash algorithm to use.
  explicit DefaultIntegrityVerificationEngine(HashAlgorithm algorithm);

  /// Move constructor.
  DefaultIntegrityVerificationEngine(
      DefaultIntegrityVerificationEngine&&) noexcept = default;

  /// Move assignment operator.
  DefaultIntegrityVerificationEngine& operator=(
      DefaultIntegrityVerificationEngine&&) noexcept = default;

  /// Destructor.
  ~DefaultIntegrityVerificationEngine() override = default;

  /// Compute integrity verification receipt.
  IntegrityVerificationReceipt compute_verification(
      const ArtifactManifest& manifest) const noexcept override;

  /// Verify integrity of artifact.
  bool verify_integrity(const ArtifactManifest& manifest,
                        const IntegrityVerificationReceipt& receipt) const
      noexcept override;

  /// Verify integrity of a single shard.
  bool verify_shard(const std::string& shard_data,
                    const std::string& expected_hash) const noexcept override;

 private:
  /// Compute hash of arbitrary data.
  std::string compute_hash(const std::string& data) const noexcept override;

  /// Build Merkle tree for shard hashes.
  std::vector<MerkleTreeNode> build_merkle_tree(
      const std::vector<std::pair<std::string, std::string>>& shard_hashes)
      const noexcept;
};

/// @}

}  // namespace distributed_tensor
}  // namespace themis
