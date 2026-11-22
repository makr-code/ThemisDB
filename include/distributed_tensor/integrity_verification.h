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


/**
 * @brief Hash algorithm identifiers stored in integrity receipts.
 * @note The default engine currently supports SHA-256 only. Other identifiers
 *       fail closed until an implementation is explicitly enabled.
 */
enum class HashAlgorithm {
  SHA256,

  BLAKE3,

  BLAKE2B,

  SHA3_256,
};

struct MerkleTreeNode {
  std::string hash = {};

  uint32_t level = 0;

  uint32_t index = 0;

  std::vector<std::string> children_hashes;

  std::unordered_map<std::string, std::string> custom_metadata;
};

class IntegrityVerificationReceipt {
 public:
  IntegrityVerificationReceipt(const std::string& artifact_id,
                               HashAlgorithm algorithm);

  IntegrityVerificationReceipt(const IntegrityVerificationReceipt&) = delete;

  IntegrityVerificationReceipt(IntegrityVerificationReceipt&&) noexcept =
      default;

  IntegrityVerificationReceipt& operator=(const IntegrityVerificationReceipt&) =
      delete;

  IntegrityVerificationReceipt& operator=(
      IntegrityVerificationReceipt&&) noexcept = default;

  ~IntegrityVerificationReceipt() = default;

  const std::string& artifact_id() const noexcept { return artifact_id_; }

  HashAlgorithm algorithm() const noexcept { return algorithm_; }

  const std::string& merkle_root_hash() const noexcept {
    return merkle_root_hash_;
  }

  void set_merkle_root_hash(const std::string& hash) noexcept {
    merkle_root_hash_ = hash;
  }

  void add_shard_hash(const std::string& shard_id,
                      const std::string& hash) noexcept {
    shard_hashes_[shard_id] = hash;
  }

  std::optional<std::string> get_shard_hash(
      const std::string& shard_id) const noexcept {
    auto it = shard_hashes_.find(shard_id);
    if (it != shard_hashes_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  const std::unordered_map<std::string, std::string>& shard_hashes()
      const noexcept {
    return shard_hashes_;
  }

  void add_merkle_tree_node(MerkleTreeNode node) noexcept {
    merkle_tree_nodes_.push_back(std::move(node));
  }

  const std::vector<MerkleTreeNode>& merkle_tree_nodes() const noexcept {
    return merkle_tree_nodes_;
  }

  void mark_verified() noexcept { is_verified_ = true; }

  void mark_failed(std::string error) noexcept {
    is_verified_ = false;
    verification_error_ = std::move(error);
  }

  bool is_verified() const noexcept { return is_verified_; }

  const std::string& verified_at() const noexcept { return verified_at_; }

  void set_verified_at(const std::string& timestamp) noexcept {
    verified_at_ = timestamp;
  }

  /**
   * @brief Return the diagnostic recorded for the last failed verification.
   * @return The stored error message, or an empty string when no error was
   *         recorded. The reference remains valid while this receipt exists
   *         and is not modified.
   */
  const std::string& verification_error() const noexcept {
    return verification_error_;
  }

 private:
  std::string artifact_id_;

  HashAlgorithm algorithm_;

  std::string merkle_root_hash_;

  std::unordered_map<std::string, std::string> shard_hashes_;

  std::vector<MerkleTreeNode> merkle_tree_nodes_;

  bool is_verified_ = false;

  std::string verified_at_;

  std::string verification_error_;
};

class IntegrityVerificationEngine {
 public:
  /**
   * @brief Create the default SHA-256 integrity verification engine.
   */
  IntegrityVerificationEngine();

  /**
   * @brief Create an integrity engine for the requested hash algorithm.
   * @param[in] algorithm Hash algorithm to use; only SHA256 is currently
   *                      supported by the default implementation.
   * @note Unsupported algorithms fail closed when verification is requested.
   */
  explicit IntegrityVerificationEngine(HashAlgorithm algorithm);

  IntegrityVerificationEngine(const IntegrityVerificationEngine&) = delete;

  IntegrityVerificationEngine(IntegrityVerificationEngine&&) noexcept = default;

  IntegrityVerificationEngine& operator=(const IntegrityVerificationEngine&) =
      delete;

  IntegrityVerificationEngine& operator=(
      IntegrityVerificationEngine&&) noexcept = default;

  /**
   * @brief Destroy the integrity verification engine.
   */
  virtual ~IntegrityVerificationEngine() = default;

  /**
   * @brief Build a versioned integrity receipt for a complete manifest.
   * @param[in] manifest Complete manifest with unique shard IDs and lowercase
   *            64-character SHA-256 digests for every shard.
   * @return A verified receipt on success; otherwise a failed receipt with an
   *         explanatory error, including when SHA-256 is unavailable.
   * @note The default engine supports SHA-256 only. Receipts produced by the
   *       former demonstration hash are incompatible and must be recomputed.
   * @note The digest binds the artifact ID, manifest content hash and version,
   *       and sorted shard IDs/digests; it does not hash raw shard bytes or
   *       authenticate a producer. Use verify_shard() for raw-byte checks.
   */
  virtual IntegrityVerificationReceipt compute_verification(
      const ArtifactManifest& manifest) const noexcept = 0;

  /**
   * @brief Verify a receipt against the current manifest contents.
   * @param[in] manifest Manifest whose artifact ID, content hash, version,
   *            unique shard IDs, and shard digests must match the receipt.
   * @param[in] receipt Previously computed receipt using the same algorithm.
   * @return True only when the receipt is verified and its digest matches;
   *         false for stale-format, mismatched, or unsupported receipts.
   * @note This checks manifest-declared digests, not raw shard bytes or
   *       producer identity; use verify_shard() to verify shard payloads.
   */
  virtual bool verify_integrity(const ArtifactManifest& manifest,
                                const IntegrityVerificationReceipt& receipt)
      const noexcept = 0;

  /**
   * @brief Verify raw shard bytes against a SHA-256 digest.
   * @param[in] shard_data Raw shard contents; an empty payload is valid.
   * @param[in] expected_hash Expected lowercase 64-character SHA-256 digest.
   * @return True when the computed digest matches; false for malformed
   *         digests, unsupported algorithms, or unavailable crypto support.
   */
  virtual bool verify_shard(const std::string& shard_data,
                            const std::string& expected_hash) const noexcept = 0;

 protected:
  HashAlgorithm algorithm_;

  /**
   * @brief Compute hash.
   * @param[in] data Input parameter.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  virtual std::string compute_hash(const std::string& data) const noexcept = 0;
};

class DefaultIntegrityVerificationEngine : public IntegrityVerificationEngine {
 public:
  DefaultIntegrityVerificationEngine();

  /**
   * @brief Create the default integrity engine for a requested algorithm.
   * @param[in] algorithm Requested hash; currently only SHA256 is implemented.
   * @note Unsupported values are accepted at construction but cause
   *       verification to fail closed with an error receipt.
   */
  explicit DefaultIntegrityVerificationEngine(HashAlgorithm algorithm);

  DefaultIntegrityVerificationEngine(
      DefaultIntegrityVerificationEngine&&) noexcept = default;

  DefaultIntegrityVerificationEngine& operator=(
      DefaultIntegrityVerificationEngine&&) noexcept = default;

  ~DefaultIntegrityVerificationEngine() override = default;

  IntegrityVerificationReceipt compute_verification(
      const ArtifactManifest& manifest) const noexcept override;

  bool verify_integrity(const ArtifactManifest& manifest,
                        const IntegrityVerificationReceipt& receipt) const
      noexcept override;

  bool verify_shard(const std::string& shard_data,
                    const std::string& expected_hash) const noexcept override;

 private:
  std::string compute_hash(const std::string& data) const noexcept override;

  std::vector<MerkleTreeNode> build_merkle_tree(
      const std::vector<std::pair<std::string, std::string>>& shard_hashes)
      const noexcept;
};


}  // namespace distributed_tensor
}  // namespace themis
