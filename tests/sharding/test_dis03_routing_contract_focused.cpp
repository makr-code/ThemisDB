#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <vector>

#include "sharding/consistent_hash.h"

#ifdef __has_include
#if __has_include(<xxhash.h>)
#define THEMIS_TEST_HAS_XXHASH
#endif
#endif

using namespace themis::sharding;

TEST(DIS03RoutingContract, PreservesCurrentPartitionHashVectors) {
    ConsistentHashRing ring;
    const std::vector<std::pair<std::string, uint64_t>> vectors{
        {"a", 0x82a2a958a9bece5bULL},
        {"abc", 0x33ebaf9927cbc5bdULL},
        {std::string("a\0b", 3), 0xab78f5eca36d0e2bULL},
        {std::string("\xff", 1), 0x1bbd5c813c69a8d7ULL}
    };

    for (const auto& [bytes, expected] : vectors) {
        RoutingKeyDescriptor descriptor{
            RoutingKeyType::PartitionKey,
            RoutingHashAlgorithm::Fnv1aMix64V1,
            bytes,
            7
        };
        auto token = ring.resolveRoutingToken(descriptor);
        EXPECT_EQ(ring.hashKey(bytes), expected);
        ASSERT_TRUE(token);
        EXPECT_EQ(*token, expected);
    }

    EXPECT_FALSE(ring.resolveRoutingToken({
        RoutingKeyType::PartitionKey,
        RoutingHashAlgorithm::XxHash64Seed0V1,
        "key",
        7
    }));
    EXPECT_FALSE(ring.resolveRoutingToken({
        RoutingKeyType::PartitionKey,
        RoutingHashAlgorithm::Fnv1aMix64V1,
        "",
        7
    }));
    EXPECT_FALSE(ring.resolveRoutingToken({
        RoutingKeyType::PartitionKey,
        RoutingHashAlgorithm::Fnv1aMix64V1,
        "key",
        0
    }));
    EXPECT_FALSE(ring.resolveRoutingToken({
        static_cast<RoutingKeyType>(255),
        RoutingHashAlgorithm::Fnv1aMix64V1,
        "key",
        7
    }));

    constexpr std::string_view uuid = "550e8400-e29b-41d4-a716-446655440000";
    const auto urn_token = ring.resolveRoutingToken({
        RoutingKeyType::UrnUuid,
        RoutingHashAlgorithm::XxHash64Seed0V1,
        uuid,
        7
    });
    EXPECT_FALSE(ring.resolveRoutingToken({
        RoutingKeyType::UrnUuid,
        RoutingHashAlgorithm::XxHash64Seed0V1,
        "not-a-uuid",
        7
    }));
#ifdef THEMIS_TEST_HAS_XXHASH
    URN parsed_urn;
    parsed_urn.uuid = std::string(uuid);
    ASSERT_TRUE(urn_token);
    EXPECT_EQ(*urn_token, parsed_urn.hash());
#else
    EXPECT_FALSE(urn_token);
#endif
}

TEST(DIS03RoutingContract, SnapshotMatchesActualRingRouting) {
    ConsistentHashRing ring;
    ring.addShard("alpha", 8);
    ring.addShard("beta", 8);

    auto snapshot = ring.snapshotOwnership(11);
    ASSERT_TRUE(snapshot);
    EXPECT_EQ(snapshot->topologyEpoch(), 11u);
    EXPECT_EQ(snapshot->entries().size(), ring.getVirtualNodeCount());
    EXPECT_EQ(snapshot->fingerprint().size(), 64u);

    for (const auto& entry : snapshot->entries()) {
        EXPECT_EQ(ring.getShardForHash(entry.token), entry.owner);
        EXPECT_TRUE(snapshot->ownsToken(entry.token, entry.owner));
        EXPECT_FALSE(snapshot->ownsToken(entry.token, "not-an-owner"));
    }
    constexpr std::string_view key = "partition-a";
    const auto token = ring.resolveRoutingToken({
        RoutingKeyType::PartitionKey,
        RoutingHashAlgorithm::Fnv1aMix64V1,
        key,
        11
    });
    ASSERT_TRUE(token);
    const auto owner = ring.getShardForHash(*token);
    EXPECT_TRUE(ring.ownsKeyInSnapshot(*snapshot, {
        RoutingKeyType::PartitionKey,
        RoutingHashAlgorithm::Fnv1aMix64V1,
        key,
        11
    }, owner));
    EXPECT_FALSE(ring.ownsKeyInSnapshot(*snapshot, {
        RoutingKeyType::PartitionKey,
        RoutingHashAlgorithm::Fnv1aMix64V1,
        key,
        12
    }, owner));
    EXPECT_FALSE(ring.snapshotOwnership(0));
}

TEST(DIS03RoutingContract, ExactSegmentsHandleZeroMaximumAndFullRing) {
    constexpr auto kMax = std::numeric_limits<uint64_t>::max();
    auto snapshot = RingOwnershipSnapshot::fromEntries(
        3,
        {{0, "zero-owner"}, {kMax, "max-owner"}}
    );
    ASSERT_TRUE(snapshot);
    EXPECT_TRUE(snapshot->ownsToken(0, "zero-owner"));
    EXPECT_TRUE(snapshot->ownsToken(1, "max-owner"));
    EXPECT_TRUE(snapshot->ownsToken(kMax, "max-owner"));
    EXPECT_FALSE(snapshot->ownsToken(kMax, "zero-owner"));

    auto singleton = RingOwnershipSnapshot::fromEntries(4, {{kMax, "only"}});
    ASSERT_TRUE(singleton);
    ASSERT_EQ(singleton->segments().size(), 1u);
    EXPECT_TRUE(singleton->segments().front().full_ring);
    EXPECT_TRUE(singleton->ownsToken(0, "only"));
    EXPECT_TRUE(singleton->ownsToken(kMax, "only"));

    auto empty = RingOwnershipSnapshot::fromEntries(5, {});
    ASSERT_TRUE(empty);
    EXPECT_TRUE(empty->entries().empty());
    EXPECT_TRUE(empty->segments().empty());
    EXPECT_FALSE(empty->ownsToken(0, "any"));
    EXPECT_FALSE(RingOwnershipSnapshot::fromEntries(6, {{1, ""}}));
    EXPECT_FALSE(RingOwnershipSnapshot::fromEntries(6, {{2, "a"}, {2, "b"}}));
}

TEST(DIS03RoutingContract, HalfOpenSegmentsPreserveClockwiseEndpointOwnership) {
    auto snapshot = RingOwnershipSnapshot::fromEntries(
        9,
        {{10, "first"}, {20, "second"}, {30, "first"}}
    );
    ASSERT_TRUE(snapshot);

    EXPECT_TRUE(snapshot->ownsToken(10, "first"));
    EXPECT_TRUE(snapshot->ownsToken(11, "second"));
    EXPECT_TRUE(snapshot->ownsToken(20, "second"));
    EXPECT_TRUE(snapshot->ownsToken(21, "first"));
    EXPECT_TRUE(snapshot->ownsToken(30, "first"));
    EXPECT_TRUE(snapshot->ownsToken(0, "first"));
    EXPECT_TRUE(snapshot->ownsToken(31, "first"));
    EXPECT_FALSE(snapshot->ownsToken(20, "first"));
}
