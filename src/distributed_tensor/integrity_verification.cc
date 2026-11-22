// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/integrity_verification.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string_view>

#if defined(THEMIS_DISTRIBUTED_TENSOR_WITH_OPENSSL)
#include <openssl/evp.h>
#endif

namespace themis {
namespace distributed_tensor {

namespace {

constexpr std::size_t kSha256DigestBytes = 32;
constexpr std::size_t kSha256HexLength = kSha256DigestBytes * 2;

[[nodiscard]] bool is_sha256_hex(std::string_view value) noexcept {
  return value.size() == kSha256HexLength &&
         std::all_of(value.begin(), value.end(), [](const char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

void append_length_prefixed(std::string& target, std::string_view value) {
  target.append(std::to_string(value.size()));
  target.push_back(':');
  target.append(value);
}

// Helper: Format current timestamp as ISO 8601 string.
static std::string get_iso8601_timestamp() noexcept {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss = {};
  oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

}  // namespace

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
  if (algorithm_ != HashAlgorithm::SHA256) {
    return {};
  }

#if defined(THEMIS_DISTRIBUTED_TENSOR_WITH_OPENSSL)
  using EvpContext =
      std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  EvpContext context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!context) {
    return {};
  }

  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  unsigned int digest_length = 0;
  if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
      (!data.empty() &&
       EVP_DigestUpdate(context.get(), data.data(), data.size()) != 1) ||
      EVP_DigestFinal_ex(context.get(), digest.data(), &digest_length) != 1 ||
      digest_length != kSha256DigestBytes) {
    return {};
  }

  constexpr char kHexDigits[] = "0123456789abcdef";
  std::string result(kSha256HexLength, '0');
  for (std::size_t index = 0; index < kSha256DigestBytes; ++index) {
    result[index * 2] = kHexDigits[digest[index] >> 4];
    result[index * 2 + 1] = kHexDigits[digest[index] & 0x0f];
  }
  return result;
#else
  (void)data;
  return {};
#endif
}

IntegrityVerificationReceipt
DefaultIntegrityVerificationEngine::compute_verification(
    const ArtifactManifest& manifest) const noexcept {
  IntegrityVerificationReceipt receipt(manifest.artifact_id(), algorithm_);
  receipt.set_verified_at(get_iso8601_timestamp());

  if (algorithm_ != HashAlgorithm::SHA256) {
    receipt.mark_failed(
        "Unsupported hash algorithm; only SHA-256 is implemented.");
    return receipt;
  }

#if !defined(THEMIS_DISTRIBUTED_TENSOR_WITH_OPENSSL)
  receipt.mark_failed(
      "SHA-256 verification is unavailable because OpenSSL Crypto was not "
      "found at build time.");
  return receipt;
#endif

  if (!manifest.is_complete()) {
    receipt.mark_failed(
        "Manifest is incomplete; refusing to generate an integrity receipt.");
    return receipt;
  }

  // Collect shard hashes.
  std::vector<std::pair<std::string, std::string>> shard_hashes;
  shard_hashes.reserve(manifest.shard_placements().size());
  for (const auto& shard : manifest.shard_placements()) {
    if (shard.shard_id.empty() || !is_sha256_hex(shard.shard_content_hash)) {
      receipt.mark_failed(
          "A shard has a missing or malformed SHA-256 content hash.");
      return receipt;
    }
    shard_hashes.emplace_back(shard.shard_id, shard.shard_content_hash);
  }

  if (shard_hashes.empty()) {
    receipt.mark_failed(
        "No shard hashes are available for verification.");
    return receipt;
  }

  std::sort(shard_hashes.begin(), shard_hashes.end(),
            [](const auto& left, const auto& right) {
              return left.first < right.first;
            });
  if (std::adjacent_find(
          shard_hashes.begin(), shard_hashes.end(),
          [](const auto& left, const auto& right) {
            return left.first == right.first;
          }) != shard_hashes.end()) {
    receipt.mark_failed("Duplicate shard IDs are not valid in an integrity receipt.");
    return receipt;
  }

  for (const auto& [shard_id, hash] : shard_hashes) {
    receipt.add_shard_hash(shard_id, hash);
  }

  // Build Merkle tree.
  auto merkle_nodes = build_merkle_tree(shard_hashes);
  if (std::any_of(merkle_nodes.begin(), merkle_nodes.end(),
                  [](const MerkleTreeNode& node) {
                    return !is_sha256_hex(node.hash);
                  })) {
    receipt.mark_failed("SHA-256 failed while constructing the Merkle tree.");
    return receipt;
  }
  for (auto& node : merkle_nodes) {
    receipt.add_merkle_tree_node(std::move(node));
  }

  // The domain tag versions this digest format; length prefixes prevent
  // ambiguous concatenations of caller-controlled manifest fields.
  std::string root_input = "themisdb.distributed-tensor.integrity.v1";
  append_length_prefixed(root_input, manifest.artifact_id());
  append_length_prefixed(root_input, manifest.content_hash());
  append_length_prefixed(root_input, manifest.version());
  append_length_prefixed(root_input, std::to_string(shard_hashes.size()));
  for (const auto& [shard_id, hash] : shard_hashes) {
    append_length_prefixed(root_input, shard_id);
    append_length_prefixed(root_input, hash);
  }

  const auto root_hash = compute_hash(root_input);
  if (!is_sha256_hex(root_hash)) {
    receipt.mark_failed("SHA-256 failed while hashing the integrity receipt.");
    return receipt;
  }
  receipt.set_merkle_root_hash(root_hash);
  receipt.mark_verified();

  return receipt;
}

bool DefaultIntegrityVerificationEngine::verify_integrity(
    const ArtifactManifest& manifest,
    const IntegrityVerificationReceipt& receipt) const noexcept {
  if (manifest.artifact_id() != receipt.artifact_id() || !receipt.is_verified() ||
      !receipt.verification_error().empty() ||
      receipt.algorithm() != algorithm_) {
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
  if (!is_sha256_hex(expected_hash) || algorithm_ != HashAlgorithm::SHA256) {
    return false;
  }

  const auto computed_hash = compute_hash(shard_data);
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
      const auto child_count =
          i + 1 < current_level_hashes.size() ? std::string_view("2")
                                              : std::string_view("1");
      std::string combined = "themisdb.distributed-tensor.merkle-node.v1";
      append_length_prefixed(combined, child_count);
      append_length_prefixed(combined, current_level_hashes[i]);
      if (i + 1 < current_level_hashes.size()) {
        append_length_prefixed(combined, current_level_hashes[i + 1]);
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
