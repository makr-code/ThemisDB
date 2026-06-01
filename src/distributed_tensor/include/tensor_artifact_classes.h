/**
 * @file tensor_artifact_classes.h
 * @brief Tensor artifact classes and lifecycle for distributed Themis sharding.
 *
 * Defines the primary artifact taxonomy: RawTensorArtifact, ShardedArtifact,
 * FederatedSummaryArtifact, and their shared lifecycle interface.
 *
 * Planned in: docs/EPIC3_ARTIFACT_CLASSES.md
 * Sub-issue:   #5429
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::distributed_tensor {

/// Top-level artifact classification.
enum class ArtifactClass {
    Raw,       ///< Unsharded, single-node tensor
    Sharded,   ///< RAID-style distributed across N shards
    Federated, ///< Cross-shard summary artifact
    Derived,   ///< Computed from one or more source artifacts
};

/// Lifecycle state of a tensor artifact.
enum class ArtifactState {
    Pending,   ///< Awaiting initial write
    Active,    ///< Fully written and readable
    Stale,     ///< Source changed; rebuild needed
    Archived,  ///< Moved to cold storage
    Deleted,   ///< Logically removed
};

/// Shared metadata present on every tensor artifact.
struct ArtifactMetadata {
    std::string  id;          ///< Content-addressable UUID
    ArtifactClass artifact_class;
    ArtifactState state = ArtifactState::Pending;
    std::string  schema_version;
    std::uint64_t size_bytes = 0;
    std::uint32_t rank       = 0;    ///< Tensor rank (number of dims)
    std::vector<std::uint64_t> shape;///< Shape per dimension
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    std::string  owner_shard_key;
};

/// A raw, non-sharded tensor artifact stored locally.
struct RawTensorArtifact {
    ArtifactMetadata meta;
    std::vector<std::uint8_t> data; ///< Serialised tensor payload
};

/// A reference to one stripe of a sharded artifact.
struct ShardStripe {
    std::string  shard_key;
    std::uint32_t stripe_index = 0;
    std::string  placement_uri;  ///< Where this stripe is stored
    std::string  checksum;       ///< SHA-256 of stripe bytes
};

/// A distributed, RAID-style sharded tensor artifact.
struct ShardedArtifact {
    ArtifactMetadata meta;
    std::uint32_t    num_data_stripes   = 0;
    std::uint32_t    num_parity_stripes = 0;
    std::vector<ShardStripe> stripes;
};

/**
 * @brief Artifact class registry and factory.
 */
class IArtifactClassRegistry {
public:
    virtual ~IArtifactClassRegistry() = default;

    /// Register a new raw artifact.
    virtual std::string registerRaw(RawTensorArtifact artifact) = 0;

    /// Register a new sharded artifact.
    virtual std::string registerSharded(ShardedArtifact artifact) = 0;

    /// Look up artifact metadata by ID.
    virtual std::optional<ArtifactMetadata> lookupMetadata(
        const std::string& id) const = 0;

    /// Transition the lifecycle state of an artifact.
    virtual bool transitionState(const std::string& id,
                                  ArtifactState target) = 0;

    /// List all artifact IDs in a given state.
    virtual std::vector<std::string> listByState(
        ArtifactState state) const = 0;
};

/// Factory: create an in-memory artifact class registry.
std::unique_ptr<IArtifactClassRegistry> makeArtifactClassRegistry();

} // namespace themis::distributed_tensor
