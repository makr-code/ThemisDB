/**
 * @file object_block_manifest.h
 * @brief Versioned metadata and local integrity helpers for object-version blocks.
 */

#pragma once

#include "utils/expected.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace themis::storage {

inline constexpr std::size_t OBJECT_BLOCK_MAX_BYTES = 256 * 1024;
inline constexpr std::uint32_t OBJECT_BLOCK_MANIFEST_FORMAT_VERSION = 1;

/**
 * @brief Metadata for one block in an immutable object version.
 *
 * The ordinal is the order in the object version, and shard_id is the caller's
 * placement decision for the pinned placement-map epoch. The SHA-256 digest
 * identifies and verifies the bytes; it does not prove that the bytes are durable.
 */
struct ObjectBlockDescriptor {
    std::uint64_t ordinal{0};
    std::string shard_id;
    std::uint32_t length_bytes{0};
    std::string sha256;
};

/**
 * @brief Versioned manifest describing one immutable object version.
 *
 * An empty object is represented by zero blocks and total_size_bytes equal to zero.
 * A non-empty block must be no larger than OBJECT_BLOCK_MAX_BYTES.
 */
struct ObjectVersionBlockManifest {
    std::uint32_t format_version{OBJECT_BLOCK_MANIFEST_FORMAT_VERSION};
    std::string object_id;
    std::string version_id;
    std::uint64_t placement_map_epoch{0};
    std::uint64_t total_size_bytes{0};
    std::vector<ObjectBlockDescriptor> blocks;
};

/**
 * @brief Bytes for one block, identified by its manifest ordinal.
 */
struct ObjectBlockPayload {
    std::uint64_t ordinal{0};
    std::vector<std::uint8_t> bytes;
};

/**
 * @brief A manifest and its block payloads in ordinal order.
 */
struct ObjectVersionBlockSet {
    ObjectVersionBlockManifest manifest;
    std::vector<ObjectBlockPayload> payloads;
};

/**
 * @brief Split object bytes into integrity-checked, manifest-ordered blocks.
 * @param object_id Stable, non-empty object identifier.
 * @param version_id Stable, non-empty immutable version identifier.
 * @param placement_map_epoch Epoch pinned by the caller for this object version.
 * @param bytes Complete object bytes; an empty span creates a valid empty version.
 * @param block_shard_ids One non-empty shard ID per generated block, in ordinal order.
 * @return Manifest and byte payloads, or an error if identifiers or placements are invalid
 *         or SHA-256 computation fails.
 * @note This local helper does not persist data, select placements, authorize callers,
 *       replicate blocks, or publish a version.
 */
[[nodiscard]] Result<ObjectVersionBlockSet> buildObjectVersionBlockSet(
    std::string_view object_id,
    std::string_view version_id,
    std::uint64_t placement_map_epoch,
    std::span<const std::uint8_t> bytes,
    std::span<const std::string> block_shard_ids);

/**
 * @brief Verify and assemble payloads in manifest order.
 * @param manifest Manifest to validate; ordinals must be contiguous from zero.
 * @param payloads Payloads identified by unique block ordinals; input order is unrestricted.
 * @return Complete object bytes, or an error for an invalid manifest, missing/duplicate
 *         payload, length mismatch, or SHA-256 mismatch.
 * @throws std::bad_alloc if memory for the assembled object cannot be allocated.
 */
[[nodiscard]] Result<std::vector<std::uint8_t>> assembleObjectVersion(
    const ObjectVersionBlockManifest& manifest,
    std::span<const ObjectBlockPayload> payloads);

} // namespace themis::storage
