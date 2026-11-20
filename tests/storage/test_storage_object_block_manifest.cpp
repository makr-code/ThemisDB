// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "storage/object_block_manifest.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

using namespace themis::storage;

TEST(ObjectVersionBlockManifest, EmptyObjectHasNoBlocks) {
    const std::vector<std::string> shard_ids;
    auto block_set = buildObjectVersionBlockSet("object-1", "version-1", 0, {}, shard_ids);

    ASSERT_TRUE(block_set.has_value());
    EXPECT_EQ(block_set->manifest.total_size_bytes, 0u);
    EXPECT_TRUE(block_set->manifest.blocks.empty());
    EXPECT_TRUE(block_set->payloads.empty());

    auto assembled = assembleObjectVersion(block_set->manifest, block_set->payloads);
    ASSERT_TRUE(assembled.has_value());
    EXPECT_TRUE(assembled->empty());
}

TEST(ObjectVersionBlockManifest, RecordsSha256AndPlacementForOneBlock) {
    const std::vector<std::uint8_t> bytes{'a', 'b', 'c'};
    const std::vector<std::string> shard_ids{"shard-a"};

    auto block_set = buildObjectVersionBlockSet("object-1", "version-2", 7, bytes, shard_ids);
    ASSERT_TRUE(block_set.has_value());

    const auto& manifest = block_set->manifest;
    ASSERT_EQ(manifest.blocks.size(), 1u);
    EXPECT_EQ(manifest.format_version, OBJECT_BLOCK_MANIFEST_FORMAT_VERSION);
    EXPECT_EQ(manifest.object_id, "object-1");
    EXPECT_EQ(manifest.version_id, "version-2");
    EXPECT_EQ(manifest.placement_map_epoch, 7u);
    EXPECT_EQ(manifest.blocks[0].ordinal, 0u);
    EXPECT_EQ(manifest.blocks[0].shard_id, "shard-a");
    EXPECT_EQ(manifest.blocks[0].length_bytes, 3u);
    EXPECT_EQ(manifest.blocks[0].sha256,
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(ObjectVersionBlockManifest, SplitsAtMaximumBlockSize) {
    const std::vector<std::uint8_t> bytes(OBJECT_BLOCK_MAX_BYTES + 1, 0x5a);
    const std::vector<std::string> shard_ids{"shard-a", "shard-b"};

    auto block_set = buildObjectVersionBlockSet("object-1", "version-3", 9, bytes, shard_ids);

    ASSERT_TRUE(block_set.has_value());
    ASSERT_EQ(block_set->manifest.blocks.size(), 2u);
    EXPECT_EQ(block_set->manifest.blocks[0].length_bytes, OBJECT_BLOCK_MAX_BYTES);
    EXPECT_EQ(block_set->manifest.blocks[1].length_bytes, 1u);
    EXPECT_EQ(block_set->manifest.blocks[1].ordinal, 1u);
    EXPECT_EQ(block_set->manifest.total_size_bytes, bytes.size());
}

TEST(ObjectVersionBlockManifest, RejectsInvalidIdentifiersAndShardAssignments) {
    const std::vector<std::uint8_t> bytes{1};
    const std::vector<std::string> missing_shard_ids;
    const std::vector<std::string> empty_shard_id{""};

    EXPECT_FALSE(buildObjectVersionBlockSet("", "version-1", 0, bytes, empty_shard_id).has_value());
    EXPECT_FALSE(buildObjectVersionBlockSet("object-1", "", 0, bytes, empty_shard_id).has_value());
    EXPECT_FALSE(
        buildObjectVersionBlockSet("object-1", "version-1", 0, bytes, missing_shard_ids).has_value());
    EXPECT_FALSE(
        buildObjectVersionBlockSet("object-1", "version-1", 0, bytes, empty_shard_id).has_value());
}

TEST(ObjectVersionBlockManifest, ReassemblesBlocksInManifestOrder) {
    std::vector<std::uint8_t> bytes(OBJECT_BLOCK_MAX_BYTES + 3);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::uint8_t>(index % 251);
    }
    const std::vector<std::string> shard_ids{"shard-a", "shard-b"};

    auto block_set = buildObjectVersionBlockSet("object-1", "version-4", 11, bytes, shard_ids);
    ASSERT_TRUE(block_set.has_value());
    std::reverse(block_set->payloads.begin(), block_set->payloads.end());

    auto assembled = assembleObjectVersion(block_set->manifest, block_set->payloads);
    ASSERT_TRUE(assembled.has_value());
    EXPECT_EQ(assembled.value(), bytes);
}

TEST(ObjectVersionBlockManifest, RejectsMissingDuplicateAndCorruptPayloads) {
    std::vector<std::uint8_t> bytes(OBJECT_BLOCK_MAX_BYTES + 1, 0x2a);
    const std::vector<std::string> shard_ids{"shard-a", "shard-b"};
    auto block_set = buildObjectVersionBlockSet("object-1", "version-5", 12, bytes, shard_ids);
    ASSERT_TRUE(block_set.has_value());

    auto missing = block_set->payloads;
    missing.pop_back();
    EXPECT_FALSE(assembleObjectVersion(block_set->manifest, missing).has_value());

    auto duplicate = block_set->payloads;
    duplicate[1].ordinal = duplicate[0].ordinal;
    EXPECT_FALSE(assembleObjectVersion(block_set->manifest, duplicate).has_value());

    auto corrupted = block_set->payloads;
    corrupted[0].bytes[0] ^= 0xff;
    EXPECT_FALSE(assembleObjectVersion(block_set->manifest, corrupted).has_value());

    auto wrong_length = block_set->payloads;
    wrong_length[0].bytes.pop_back();
    EXPECT_FALSE(assembleObjectVersion(block_set->manifest, wrong_length).has_value());
}

TEST(ObjectVersionBlockManifest, RejectsMalformedManifestMetadata) {
    const std::vector<std::uint8_t> bytes{1, 2, 3};
    const std::vector<std::string> shard_ids{"shard-a"};
    auto block_set = buildObjectVersionBlockSet("object-1", "version-6", 13, bytes, shard_ids);
    ASSERT_TRUE(block_set.has_value());

    auto wrong_size = block_set->manifest;
    ++wrong_size.total_size_bytes;
    EXPECT_FALSE(assembleObjectVersion(wrong_size, block_set->payloads).has_value());

    auto unsupported_version = block_set->manifest;
    ++unsupported_version.format_version;
    EXPECT_FALSE(assembleObjectVersion(unsupported_version, block_set->payloads).has_value());

    auto malformed_digest = block_set->manifest;
    malformed_digest.blocks[0].sha256 = "not-a-digest";
    EXPECT_FALSE(assembleObjectVersion(malformed_digest, block_set->payloads).has_value());
}
