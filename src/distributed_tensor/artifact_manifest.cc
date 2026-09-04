// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/artifact_manifest.h"

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
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

// Helper: Simple hash computation (basic XOR-based hash for manifest structure).
// In production, this would use a cryptographic hash like SHA-256.
static std::string compute_simple_hash(const std::string& input) noexcept {
  if (input.empty()) {
    return "0000000000000000";
  }

  // Simple hash: take first 16 bytes as hex string.
  uint64_t hash = 0;
  for (size_t i = 0; i < input.length() && i < 8; ++i) {
    hash = hash * 31 + static_cast<uint8_t>(input[i]);
  }

  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return oss.str();
}

// ArtifactManifest implementation.

ArtifactManifest::ArtifactManifest(const std::string& artifact_id,
                                   ArtifactClass artifact_class)
    : artifact_id_(artifact_id),
      artifact_class_(artifact_class),
      version_(""),
      content_hash_(""),
      manifest_hash_(""),
      total_size_bytes_(0),
      lifecycle_stage_(ArtifactLifecycleStage::STAGING),
      recovery_strategy_("replication"),
      freshness_timestamp_(get_iso8601_timestamp()),
      provenance_origin_(""),
      package_lineage_id_(""),
      parent_artifact_id_("") {}

void ArtifactManifest::compute_manifest_hash() noexcept {
  // Construct a normalized string representation of manifest metadata.
  std::ostringstream oss;
  oss << artifact_id_ << "|" << static_cast<int>(artifact_class_) << "|"
      << version_ << "|" << content_hash_ << "|" << total_size_bytes_ << "|"
      << static_cast<int>(lifecycle_stage_) << "|" << recovery_strategy_ << "|"
      << provenance_origin_ << "|" << package_lineage_id_ << "|"
      << parent_artifact_id_;

  // For each shard placement, include shard_id and node_id.
  for (const auto& shard : shard_placements_) {
    oss << "|shard:" << shard.shard_id << ":" << shard.node_id;
  }

  manifest_hash_ = compute_simple_hash(oss.str());
}

void ArtifactManifest::add_shard_placement(ShardPlacement placement) noexcept {
  // Remove any existing shard with the same ID.
  auto it = std::find_if(
      shard_placements_.begin(), shard_placements_.end(),
      [&](const ShardPlacement& sp) { return sp.shard_id == placement.shard_id; });
  if (it != shard_placements_.end()) {
    shard_placements_.erase(it);
  }

  shard_placements_.push_back(std::move(placement));
}

std::optional<ShardPlacement> ArtifactManifest::get_shard_placement(
    const std::string& shard_id) const noexcept {
  auto it = std::find_if(
      shard_placements_.begin(), shard_placements_.end(),
      [&](const ShardPlacement& sp) { return sp.shard_id == shard_id; });

  if (it != shard_placements_.end()) {
    return *it;
  }
  return std::nullopt;
}

void ArtifactManifest::update_freshness() noexcept {
  freshness_timestamp_ = get_iso8601_timestamp();
}

bool ArtifactManifest::is_complete() const noexcept {
  // A manifest is considered complete if it has:
  // 1. artifact_id and version set
  // 2. At least one shard placement
  // 3. recovery_strategy set
  // 4. For primary artifacts, content_hash is set

  if (artifact_id_.empty() || version_.empty()) {
    return false;
  }

  if (shard_placements_.empty()) {
    return false;
  }

  for (const auto& shard : shard_placements_) {
    if (shard.shard_id.empty() || shard.node_id.empty()) {
      return false;
    }
  }

  // Primary artifacts require content hash.
  if (artifact_class_ == ArtifactClass::PRIMARY && content_hash_.empty()) {
    return false;
  }

  if (artifact_class_ == ArtifactClass::DERIVED &&
      parent_artifact_id_.empty() && !reconstruction_instruction_) {
    return false;
  }

  return true;
}

bool ArtifactManifest::validate() const noexcept {
  // SG-DT-01 (Fail-Closed Invariant): Must fail closed on any validation error.
  // Checks all manifest invariants to ensure consistency and policy compliance.
  
  // Check 1: artifact_id is non-empty
  if (artifact_id_.empty()) {
    return false;
  }

  // Check 2: At least one shard placement exists with replication_factor >= 1
  if (shard_placements_.empty()) {
    return false;
  }

  for (const auto& shard : shard_placements_) {
    // Shard ID and node ID must be non-empty
    if (shard.shard_id.empty() || shard.node_id.empty()) {
      return false;
    }
    // Replication factor must be at least 1
    if (shard.replication_factor < 1) {
      return false;
    }
  }

  // Check 3: version must be set
  if (version_.empty()) {
    return false;
  }

  // Check 4: artifact_class constraints
  // Primary artifacts require content hash
  if (artifact_class_ == ArtifactClass::PRIMARY && content_hash_.empty()) {
    return false;
  }

  // Derived artifacts must have parent or reconstruction instruction
  if (artifact_class_ == ArtifactClass::DERIVED &&
      parent_artifact_id_.empty() && !reconstruction_instruction_) {
    return false;
  }

  // All invariants hold
  return true;
}

}  // namespace distributed_tensor
}  // namespace themis
