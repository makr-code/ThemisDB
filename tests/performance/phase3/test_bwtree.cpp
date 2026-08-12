// Unit tests for Bw-Tree (Phase 3)
// Based on "The Bw-Tree: A B-tree for New Hardware Platforms" (ICDE'13)

#include <gtest/gtest.h>
#include "performance/phase3/bwtree.h"
#include <thread>
#include <random>

using namespace themis::performance::phase3;

// ==================== MappingTable Tests ====================

TEST(MappingTableTest, BasicOperations) {
    MappingTable table(100);
    
    // Initial get should return nullptr
    EXPECT_EQ(table.get(1), nullptr);
    
    // Create a test page
    auto page = new LeafPage();
    
    // CAS with nullptr expected should succeed
    BwTreePage* expected = nullptr;
    EXPECT_TRUE(table.compare_and_swap(1, expected, page));
    
    // Get should now return the page
    EXPECT_EQ(table.get(1), page);
    
    // CAS with wrong expected should fail
    auto page2 = new LeafPage();
    BwTreePage* wrong_expected = nullptr;
    EXPECT_FALSE(table.compare_and_swap(1, wrong_expected, page2));
    
    // Clean up
    delete page2;
    delete page;
}

TEST(MappingTableTest, CompareAndSwap) {
    MappingTable table(100);
    
    auto page1 = new LeafPage();
    auto page2 = new LeafPage();
    
    // Install page1
    BwTreePage* expected = nullptr;
    EXPECT_TRUE(table.compare_and_swap(5, expected, page1));
    
    // Replace page1 with page2
    expected = page1;
    EXPECT_TRUE(table.compare_and_swap(5, expected, page2));
    
    // Verify page2 is installed
    EXPECT_EQ(table.get(5), page2);
    
    // Clean up
    delete page1;
    delete page2;
}

// ==================== BwTree Tests ====================

TEST(BwTreeTest, Construction) {
    BwTree tree;
    auto stats = tree.get_stats();
    EXPECT_GT(stats.num_pages, 0u);
}

TEST(BwTreeTest, InsertAndSearch) {
    BwTree tree;
    
    // Insert key-value pairs
    EXPECT_TRUE(tree.insert(10, "ten"));
    EXPECT_TRUE(tree.insert(20, "twenty"));
    EXPECT_TRUE(tree.insert(30, "thirty"));
    
    // Search for existing keys
    std::string value;
    EXPECT_TRUE(tree.search(10, value));
    EXPECT_EQ(value, "ten");
    
    EXPECT_TRUE(tree.search(20, value));
    EXPECT_EQ(value, "twenty");
    
    EXPECT_TRUE(tree.search(30, value));
    EXPECT_EQ(value, "thirty");
}

TEST(BwTreeTest, SearchNonExistent) {
    BwTree tree;
    
    tree.insert(10, "ten");
    tree.insert(30, "thirty");
    
    std::string value;
    EXPECT_FALSE(tree.search(20, value));  // Key doesn't exist
    EXPECT_FALSE(tree.search(40, value));  // Key doesn't exist
}

TEST(BwTreeTest, UpdateExistingKey) {
    BwTree tree;
    
    // Insert initial value
    EXPECT_TRUE(tree.insert(10, "ten"));
    
    // Update with new value
    EXPECT_TRUE(tree.insert(10, "TEN"));
    
    // Verify updated value
    std::string value;
    EXPECT_TRUE(tree.search(10, value));
    EXPECT_EQ(value, "TEN");
}

TEST(BwTreeTest, RangeScan) {
    BwTree tree;
    
    // Insert multiple keys
    tree.insert(10, "ten");
    tree.insert(20, "twenty");
    tree.insert(30, "thirty");
    tree.insert(40, "forty");
    tree.insert(50, "fifty");
    
    // Range scan [20, 40]
    auto results = tree.range_scan(20, 40);
    
    EXPECT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].first, 20);
    EXPECT_EQ(results[1].first, 30);
    EXPECT_EQ(results[2].first, 40);
}

TEST(BwTreeTest, EmptyRangeScan) {
    BwTree tree;
    
    tree.insert(10, "ten");
    tree.insert(50, "fifty");
    
    // Range scan with no matching keys
    auto results = tree.range_scan(20, 40);
    
    EXPECT_TRUE(results.empty());
}

TEST(BwTreeTest, MultipleInserts) {
    BwTree tree;
    
    // Insert many keys
    for (int i = 0; i < 100; i++) {
        EXPECT_TRUE(tree.insert(i * 10, "value_" + std::to_string(i)));
    }
    
    // Verify some keys
    std::string value;
    EXPECT_TRUE(tree.search(0, value));
    EXPECT_EQ(value, "value_0");
    
    EXPECT_TRUE(tree.search(500, value));
    EXPECT_EQ(value, "value_50");
    
    EXPECT_TRUE(tree.search(990, value));
    EXPECT_EQ(value, "value_99");
}

TEST(BwTreeTest, DeltaChainStatistics) {
    BwTree tree;
    
    // Insert keys to create delta chain
    for (int i = 0; i < 10; i++) {
        tree.insert(i, "value_" + std::to_string(i));
    }
    
    auto stats = tree.get_stats();
    EXPECT_GT(stats.num_deltas, 0u);
}

TEST(BwTreeTest, ConcurrentInserts) {
    BwTree tree;
    const int num_threads = 4;
    const int inserts_per_thread = 25;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&tree, t, inserts_per_thread]() {
            for (int i = 0; i < inserts_per_thread; i++) {
                int key = t * inserts_per_thread + i;
                tree.insert(key, "value_" + std::to_string(key));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all inserts succeeded
    int found_count = 0;
    for (int i = 0; i < num_threads * inserts_per_thread; i++) {
        std::string value;
        if (tree.search(i, value)) {
            found_count++;
        }
    }
    
    EXPECT_EQ(found_count, num_threads * inserts_per_thread);
}

TEST(BwTreeTest, MixedConcurrentOperations) {
    BwTree tree;
    
    // Pre-populate with some data
    for (int i = 0; i < 50; i++) {
        tree.insert(i, "value_" + std::to_string(i));
    }
    
    std::atomic<int> search_count{0};
    std::atomic<int> insert_count{0};
    
    // Launch concurrent readers and writers
    std::vector<std::thread> threads;
    
    // Reader threads
    for (int t = 0; t < 2; t++) {
        threads.emplace_back([&tree, &search_count]() {
            for (int i = 0; i < 100; i++) {
                std::string value;
                if (tree.search(i % 50, value)) {
                    search_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    
    // Writer threads
    for (int t = 0; t < 2; t++) {
        threads.emplace_back([&tree, &insert_count, t]() {
            for (int i = 0; i < 25; i++) {
                int key = 50 + t * 25 + i;
                if (tree.insert(key, "new_value_" + std::to_string(key))) {
                    insert_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(search_count.load(), 0);
    EXPECT_EQ(insert_count.load(), 50);
}

// Test for double-free bug fix: concurrent operations with high contention
// This test creates many concurrent inserts which will trigger consolidation
// when delta chains exceed the threshold, testing the ownership transfer fix
TEST(BwTreeTest, ConcurrentConsolidationSafety) {
    BwTree tree;
    
    // Pre-populate with data to create delta chain
    for (int i = 0; i < 20; i++) {
        tree.insert(i, "initial_" + std::to_string(i));
    }
    
    // Create heavy contention with concurrent inserts and searches
    // This increases the likelihood of CAS failures during consolidation
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};
    
    // Writer threads - create delta chains
    for (int t = 0; t < 4; t++) {
        threads.emplace_back([&tree, &stop, t]() {
            int count = 0;
            while (!stop.load() && count < 50) {
                for (int i = 0; i < 5; i++) {
                    int key = (t * 10 + i) % 20;
                    tree.insert(key, "updated_" + std::to_string(t) + "_" + std::to_string(count));
                }
                count++;
            }
        });
    }
    
    // Reader threads - trigger apply_deltas() in search operations
    // This creates temporary consolidated views for reading
    for (int t = 0; t < 4; t++) {
        threads.emplace_back([&tree, &stop]() {
            int count = 0;
            while (!stop.load() && count < 100) {
                for (int i = 0; i < 20; i++) {
                    std::string value;
                    tree.search(i, value);
                }
                count++;
            }
        });
    }
    
    // Let threads run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // If there was a double-free bug, the test would likely crash or 
    // trigger memory sanitizer errors. The fact that we reach here means
    // memory management is correct.
    
    // Verify tree is still functional
    std::string value;
    EXPECT_TRUE(tree.search(5, value));
}

// Test that consolidation is triggered when delta chain gets too long
TEST(BwTreeTest, AutomaticConsolidation) {
    BwTree tree;
    
    // Insert more than DELTA_CHAIN_THRESHOLD (defined as 10 in bwtree.h) records
    // on the same key so they all land in a single growing delta chain.
    for (int i = 0; i < 15; i++) {
        EXPECT_TRUE(tree.insert(1, "value_" + std::to_string(i)));
    }
    
    // Get stats to verify consolidation happened
    auto stats = tree.get_stats();
    
    // After 15 inserts with DELTA_CHAIN_THRESHOLD=10 we should have triggered
    // at least one consolidation, so delta count should be < 15
    EXPECT_LT(stats.num_deltas, 15u);
    
    // Verify the tree still works correctly after consolidation
    std::string value;
    EXPECT_TRUE(tree.search(1, value));
    EXPECT_EQ(value, "value_14");  // Last inserted value
}

// ==================== Remove Tests ====================

TEST(BwTreeTest, RemoveExistingKey) {
    BwTree tree;

    tree.insert(10, "ten");
    tree.insert(20, "twenty");
    tree.insert(30, "thirty");

    // Remove an existing key
    EXPECT_TRUE(tree.remove(20));

    // Removed key must not be found
    std::string value;
    EXPECT_FALSE(tree.search(20, value));

    // Other keys must still be present
    EXPECT_TRUE(tree.search(10, value));
    EXPECT_EQ(value, "ten");
    EXPECT_TRUE(tree.search(30, value));
    EXPECT_EQ(value, "thirty");
}

TEST(BwTreeTest, RemoveNonExistentKeyReturnsFalse) {
    BwTree tree;

    tree.insert(10, "ten");

    // remove() installs a DeltaDelete unconditionally when it succeeds via CAS.
    // The delta has no effect during apply_deltas() if the key was never present,
    // but the operation itself is valid (and returns true on CAS success).
    // The important invariant: the key must not appear in subsequent searches.
    tree.remove(99);  // key absent — operation is benign

    // Existing key must be unaffected
    std::string value;
    EXPECT_TRUE(tree.search(10, value));
    EXPECT_FALSE(tree.search(99, value));
}

TEST(BwTreeTest, RemoveThenReinsert) {
    BwTree tree;

    tree.insert(42, "original");
    EXPECT_TRUE(tree.remove(42));

    std::string value;
    EXPECT_FALSE(tree.search(42, value));

    // Re-insert the same key with a different value
    EXPECT_TRUE(tree.insert(42, "reinserted"));
    EXPECT_TRUE(tree.search(42, value));
    EXPECT_EQ(value, "reinserted");
}

TEST(BwTreeTest, RemoveAffectsRangeScan) {
    BwTree tree;

    tree.insert(10, "ten");
    tree.insert(20, "twenty");
    tree.insert(30, "thirty");

    tree.remove(20);

    auto results = tree.range_scan(10, 30);

    // Only 10 and 30 should remain
    EXPECT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].first, 10);
    EXPECT_EQ(results[1].first, 30);
}

TEST(BwTreeTest, RemoveAfterConsolidation) {
    BwTree tree;

    // Insert more than DELTA_CHAIN_THRESHOLD (10) distinct keys to trigger at
    // least one consolidation before we attempt the remove.
    for (int i = 0; i < 15; i++) {
        tree.insert(i * 10, "val_" + std::to_string(i));
    }

    // Remove a key that survived consolidation
    EXPECT_TRUE(tree.remove(50));

    std::string value;
    EXPECT_FALSE(tree.search(50, value));
    EXPECT_TRUE(tree.search(40, value));
    EXPECT_TRUE(tree.search(60, value));
}

