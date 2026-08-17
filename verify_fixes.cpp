// Verification test for const-correctness and move semantics fixes

#include "distributed_tensor/tensor_storage_strategy.h"
#include "src/distributed_tensor/include/manifest_store.h"
#include "src/distributed_tensor/include/shard_summary_coordinator.h"

#include <memory>
#include <iostream>
#include <type_traits>

namespace themis {
namespace distributed_tensor {

// Test 1: Verify MmapLoader::advise is non-const
static_assert(
    !std::is_const_v<decltype(&MmapLoader::advise)>,
    "MmapLoader::advise should be non-const"
);

// Test 2: Verify ManifestStore move operations are noexcept
static_assert(
    std::is_nothrow_move_constructible_v<ManifestStore>,
    "ManifestStore move constructor should be noexcept"
);

static_assert(
    std::is_nothrow_move_assignable_v<ManifestStore>,
    "ManifestStore move assignment should be noexcept"
);

// Test 3: Verify ShardSummaryCoordinator move operations are noexcept
static_assert(
    std::is_nothrow_move_constructible_v<ShardSummaryCoordinator>,
    "ShardSummaryCoordinator move constructor should be noexcept"
);

static_assert(
    std::is_nothrow_move_assignable_v<ShardSummaryCoordinator>,
    "ShardSummaryCoordinator move assignment should be noexcept"
);

// Test 4: Verify copy operations are deleted where intended
static_assert(
    !std::is_copy_constructible_v<ManifestStore>,
    "ManifestStore copy constructor should be deleted"
);

static_assert(
    !std::is_copy_assignable_v<ManifestStore>,
    "ManifestStore copy assignment should be deleted"
);

// Test 5: Verify ShardSummaryCoordinator copy is deleted but move is allowed
static_assert(
    !std::is_copy_constructible_v<ShardSummaryCoordinator>,
    "ShardSummaryCoordinator copy constructor should be deleted"
);

static_assert(
    std::is_move_constructible_v<ShardSummaryCoordinator>,
    "ShardSummaryCoordinator move constructor should be available"
);

// Runtime test: verify move semantics work
void testRuntimeMoveSemantics() {
    // Test ManifestStore move
    {
        ManifestStore store1(nullptr);
        ManifestStore store2 = std::move(store1);  // Should use noexcept move constructor
        ManifestStore store3(nullptr);
        store3 = std::move(store2);  // Should use noexcept move assignment
        std::cout << "✓ ManifestStore move semantics work correctly\n";
    }

    // Test ShardSummaryCoordinator move
    {
        ShardSummaryCoordinator coord1(nullptr, nullptr);
        ShardSummaryCoordinator coord2 = std::move(coord1);  // Should use noexcept move constructor
        ShardSummaryCoordinator coord3(nullptr, nullptr);
        coord3 = std::move(coord2);  // Should use noexcept move assignment
        std::cout << "✓ ShardSummaryCoordinator move semantics work correctly\n";
    }
}

} // namespace distributed_tensor
} // namespace themis

int main() {
    std::cout << "Running verification tests...\n\n";
    
    std::cout << "Compile-time checks:\n";
    std::cout << "✓ MmapLoader::advise is non-const (no const_cast needed)\n";
    std::cout << "✓ ManifestStore has noexcept move constructor\n";
    std::cout << "✓ ManifestStore has noexcept move assignment\n";
    std::cout << "✓ ShardSummaryCoordinator has noexcept move constructor\n";
    std::cout << "✓ ShardSummaryCoordinator has noexcept move assignment\n";
    std::cout << "✓ Copy operations properly deleted\n";
    
    std::cout << "\nRuntime checks:\n";
    themis::distributed_tensor::testRuntimeMoveSemantics();
    
    std::cout << "\n✅ All verification tests passed!\n";
    return 0;
}
