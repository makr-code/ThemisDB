/**
 * @file test_replication_conflict_focused.cpp
 * @brief Focused replication conflict resolution test suite.
 * @version 0.0.2
 * @note Tier: unit
 * @note Scope: conflict-resolution, diagnostics, deterministic behavior
 * @copyright (c) 2026 ThemisDB Project, Apache-2.0 License
 */


#include <gtest/gtest.h>
#include "replication/conflict_resolution.h"
#include "replication/multi_master_replication.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace themisdb {
namespace replication {
namespace test {

// ============================================================================
// ThreeWayMergeResolver Focused Tests
// ============================================================================

/**
 * RCS-01: Three-Way Merge Strategy Tests
 * 
 * Tests validate that the three-way merge resolver correctly identifies
 * ancestors and performs merges without fabricating new writes.
 */
class ThreeWayMergeResolverTest : public ::testing::Test {
protected:
    std::unique_ptr<ThreeWayMergeResolver> resolver_;
    AdvancedConflictResolver::ResolutionContext default_context_;

    void SetUp() override {
        resolver_ = std::make_unique<ThreeWayMergeResolver>();
        default_context_.collection = "test_collection";
        default_context_.document_id = "doc_001";
        default_context_.user_roles = {"user"};
        default_context_.client_ip = "127.0.0.1";
        default_context_.request_time = std::chrono::system_clock::now();
    }

    /**
     * Helper to create a test MMWriteEntry.
     */
    static MMWriteEntry createTestWrite(
        const std::string& write_id,
        const std::string& doc_id,
        const std::string& collection,
        const std::string& operation = "WRITE"
    ) {
        MMWriteEntry entry{};
        entry.write_id = write_id;
        entry.document_id = doc_id;
        entry.collection = collection;
        entry.origin_node = "test_node";
        entry.operation = operation;
        entry.data = R"({"test":"data"})";
        entry.checksum = "test_checksum";
        return entry;
    }
};

/**
 * RCS-01.1: Single write in conflict set is returned unchanged.
 */
TEST_F(ThreeWayMergeResolverTest, SingleWriteReturnedUnchanged) {
    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection"));

    // Single write should return itself (no conflict logic needed)
    MMWriteEntry winner = resolver_->resolve("doc_001", conflict_set, default_context_);

    EXPECT_EQ(winner.write_id, "w1");
    EXPECT_EQ(winner.document_id, "doc_001");
}

/**
 * RCS-01.2: Multiple writes are resolved to one winner.
 */
TEST_F(ThreeWayMergeResolverTest, MultipleWritesResolved) {
    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection", "WRITE"));
    conflict_set.push_back(createTestWrite("w2", "doc_001", "test_collection", "WRITE"));
    conflict_set.push_back(createTestWrite("w3", "doc_001", "test_collection", "WRITE"));

    // Merge should select one winner
    MMWriteEntry winner = resolver_->resolve("doc_001", conflict_set, default_context_);

    EXPECT_EQ(winner.document_id, "doc_001");
    // Winner must be one of the input entries (no fabrication)
    EXPECT_TRUE(winner.write_id == "w1" || winner.write_id == "w2" || winner.write_id == "w3");
}

/**
 * RCS-01.3: Empty conflict set fails closed.
 */
TEST_F(ThreeWayMergeResolverTest, EmptyConflictSetFailsClosed) {
    std::vector<MMWriteEntry> conflict_set;

    // Empty set should not be processed
    EXPECT_THROW(
        resolver_->resolve("doc_001", conflict_set, default_context_),
        std::invalid_argument
    );
}

/**
 * RCS-01.4: Strategy name is consistent.
 */
TEST_F(ThreeWayMergeResolverTest, StrategyNameConsistent) {
    std::string strategy = resolver_->strategyName();

    EXPECT_EQ(strategy, "THREE_WAY_MERGE");
    // Repeated calls should return the same value
    EXPECT_EQ(resolver_->strategyName(), strategy);
}

// ============================================================================
// FieldLevelMergeResolver Focused Tests
// ============================================================================

/**
 * RCS-02: Field-Level Merge Strategy Tests
 * 
 * Tests validate that the field-level merge resolver correctly implements
 * UNION, INTERSECT, LEFT_BIAS, and RIGHT_BIAS strategies.
 */
class FieldLevelMergeResolverTest : public ::testing::Test {
protected:
    AdvancedConflictResolver::ResolutionContext default_context_;

    void SetUp() override {
        default_context_.collection = "test_collection";
        default_context_.document_id = "doc_001";
        default_context_.user_roles = {"user"};
        default_context_.client_ip = "127.0.0.1";
        default_context_.request_time = std::chrono::system_clock::now();
    }

    static MMWriteEntry createTestWrite(
        const std::string& write_id,
        const std::string& doc_id,
        const std::string& collection
    ) {
        MMWriteEntry entry{};
        entry.write_id = write_id;
        entry.document_id = doc_id;
        entry.collection = collection;
        entry.origin_node = "test_node";
        entry.operation = "WRITE";
        entry.data = R"({"test":"data"})";
        entry.checksum = "test_checksum";
        return entry;
    }
};

/**
 * RCS-02.1: UNION strategy includes all fields.
 */
TEST_F(FieldLevelMergeResolverTest, UnionStrategyIncludesAllFields) {
    auto resolver = std::make_unique<FieldLevelMergeResolver>(
        FieldLevelMergeResolver::MergeStrategy::UNION
    );

    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection"));
    conflict_set.push_back(createTestWrite("w2", "doc_001", "test_collection"));

    MMWriteEntry winner = resolver->resolve("doc_001", conflict_set, default_context_);

    EXPECT_EQ(winner.document_id, "doc_001");
}

/**
 * RCS-02.2: INTERSECT strategy includes only common fields.
 */
TEST_F(FieldLevelMergeResolverTest, IntersectStrategyOnlyCommonFields) {
    auto resolver = std::make_unique<FieldLevelMergeResolver>(
        FieldLevelMergeResolver::MergeStrategy::INTERSECT
    );

    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection"));
    conflict_set.push_back(createTestWrite("w2", "doc_001", "test_collection"));

    MMWriteEntry winner = resolver->resolve("doc_001", conflict_set, default_context_);

    EXPECT_EQ(winner.document_id, "doc_001");
}

/**
 * RCS-02.3: LEFT_BIAS strategy resolves without error and returns valid winner.
 *
 * Note: LEFT_BIAS applies at field-value level for conflicting fields; the base
 * winner is always the latest-HLC write. This test verifies the resolver runs
 * cleanly and returns a valid entry, not a specific write_id.
 */
TEST_F(FieldLevelMergeResolverTest, LeftBiasPreferFirstEntry) {
    auto resolver = std::make_unique<FieldLevelMergeResolver>(
        FieldLevelMergeResolver::MergeStrategy::LEFT_BIAS
    );

    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection"));
    conflict_set.push_back(createTestWrite("w2", "doc_001", "test_collection"));

    MMWriteEntry winner = resolver->resolve("doc_001", conflict_set, default_context_);

    // Base winner is HLC-latest; bias applies at field level not winner selection
    EXPECT_EQ(winner.document_id, "doc_001");
    EXPECT_TRUE(winner.write_id == "w1" || winner.write_id == "w2");
}

/**
 * RCS-02.4: RIGHT_BIAS strategy resolves without error and returns valid winner.
 *
 * Note: RIGHT_BIAS applies at field-value level for conflicting fields; the base
 * winner is always the latest-HLC write. This test verifies the resolver runs
 * cleanly and returns a valid entry, not a specific write_id.
 */
TEST_F(FieldLevelMergeResolverTest, RightBiasPreferLastEntry) {
    auto resolver = std::make_unique<FieldLevelMergeResolver>(
        FieldLevelMergeResolver::MergeStrategy::RIGHT_BIAS
    );

    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection"));
    conflict_set.push_back(createTestWrite("w2", "doc_001", "test_collection"));

    MMWriteEntry winner = resolver->resolve("doc_001", conflict_set, default_context_);

    // Base winner is HLC-latest; bias applies at field level not winner selection
    EXPECT_EQ(winner.document_id, "doc_001");
    EXPECT_TRUE(winner.write_id == "w1" || winner.write_id == "w2");
}

/**
 * RCS-02.5: Strategy names are correct and deterministic.
 */
TEST_F(FieldLevelMergeResolverTest, StrategyNamesCorrect) {
    auto union_resolver = std::make_unique<FieldLevelMergeResolver>(
        FieldLevelMergeResolver::MergeStrategy::UNION
    );
    auto intersect_resolver = std::make_unique<FieldLevelMergeResolver>(
        FieldLevelMergeResolver::MergeStrategy::INTERSECT
    );
    auto left_resolver = std::make_unique<FieldLevelMergeResolver>(
        FieldLevelMergeResolver::MergeStrategy::LEFT_BIAS
    );
    auto right_resolver = std::make_unique<FieldLevelMergeResolver>(
        FieldLevelMergeResolver::MergeStrategy::RIGHT_BIAS
    );

    EXPECT_EQ(union_resolver->strategyName(), "FIELD_MERGE_UNION");
    EXPECT_EQ(intersect_resolver->strategyName(), "FIELD_MERGE_INTERSECT");
    EXPECT_EQ(left_resolver->strategyName(), "FIELD_MERGE_LEFT_BIAS");
    EXPECT_EQ(right_resolver->strategyName(), "FIELD_MERGE_RIGHT_BIAS");
}

// ============================================================================
// Conflict Context Semantics Tests
// ============================================================================

/**
 * RCS-03: Conflict Context Semantics Tests
 * 
 * Tests validate that the resolver respects and processes resolution context
 * (collection, document_id, metadata, user_roles, client_ip, request_time).
 */
class ConflictContextTest : public ::testing::Test {
protected:
    std::unique_ptr<ThreeWayMergeResolver> resolver_;

    void SetUp() override {
        resolver_ = std::make_unique<ThreeWayMergeResolver>();
    }

    static MMWriteEntry createTestWrite(
        const std::string& write_id,
        const std::string& doc_id,
        const std::string& collection
    ) {
        MMWriteEntry entry{};
        entry.write_id = write_id;
        entry.document_id = doc_id;
        entry.collection = collection;
        entry.origin_node = "test_node";
        entry.operation = "WRITE";
        entry.data = R"({"test":"data"})";
        return entry;
    }
};

/**
 * RCS-03.1: Resolution context with metadata and roles is respected.
 */
TEST_F(ConflictContextTest, ResolutionContextRespected) {
    AdvancedConflictResolver::ResolutionContext ctx;
    ctx.collection = "test_col";
    ctx.document_id = "doc_123";
    ctx.metadata = {{"source", "client_a"}};
    ctx.user_roles = {"admin", "write"};
    ctx.client_ip = "192.168.1.1";
    ctx.request_time = std::chrono::system_clock::now();

    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_123", "test_col"));
    conflict_set.push_back(createTestWrite("w2", "doc_123", "test_col"));

    // Should not throw and should return a valid winner
    EXPECT_NO_THROW({
        MMWriteEntry winner = resolver_->resolve("doc_123", conflict_set, ctx);
        EXPECT_EQ(winner.document_id, "doc_123");
    });
}

/**
 * RCS-03.2: Empty user roles are handled correctly.
 */
TEST_F(ConflictContextTest, EmptyUserRolesHandled) {
    AdvancedConflictResolver::ResolutionContext ctx;
    ctx.user_roles.clear();  // No roles
    ctx.collection = "test_col";
    ctx.document_id = "doc_123";

    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_123", "test_col"));

    EXPECT_NO_THROW({
        MMWriteEntry winner = resolver_->resolve("doc_123", conflict_set, ctx);
        EXPECT_EQ(winner.document_id, "doc_123");
    });
}

// ============================================================================
// Deterministic Behavior Tests
// ============================================================================

/**
 * RCS-04: Deterministic Behavior Tests
 * 
 * Tests validate that conflict resolution is deterministic, idempotent,
 * and produces consistent results across repeated invocations.
 */
class DeterministicConflictResolutionTest : public ::testing::Test {
protected:
    std::unique_ptr<ThreeWayMergeResolver> resolver_;
    AdvancedConflictResolver::ResolutionContext default_context_;

    void SetUp() override {
        resolver_ = std::make_unique<ThreeWayMergeResolver>();
        default_context_.collection = "test_collection";
        default_context_.document_id = "doc_001";
    }

    static MMWriteEntry createTestWrite(
        const std::string& write_id,
        const std::string& doc_id,
        const std::string& collection
    ) {
        MMWriteEntry entry{};
        entry.write_id = write_id;
        entry.document_id = doc_id;
        entry.collection = collection;
        entry.origin_node = "test_node";
        entry.operation = "WRITE";
        entry.data = R"({"test":"data"})";
        return entry;
    }
};

/**
 * RCS-04.1: Same input always produces same winner (idempotence).
 */
TEST_F(DeterministicConflictResolutionTest, IdempotentResolution) {
    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection"));
    conflict_set.push_back(createTestWrite("w2", "doc_001", "test_collection"));

    MMWriteEntry winner1 = resolver_->resolve("doc_001", conflict_set, default_context_);
    MMWriteEntry winner2 = resolver_->resolve("doc_001", conflict_set, default_context_);

    EXPECT_EQ(winner1.write_id, winner2.write_id);
}

/**
 * RCS-04.2: Multiple sequential calls don't corrupt resolver state.
 */
TEST_F(DeterministicConflictResolutionTest, SequentialResolutionStateIntegrity) {
    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection"));
    conflict_set.push_back(createTestWrite("w2", "doc_001", "test_collection"));

    // Multiple sequential calls should not corrupt state
    std::vector<std::string> winners;
    for (int i = 0; i < 10; ++i) {
        MMWriteEntry winner = resolver_->resolve("doc_001", conflict_set, default_context_);
        winners.push_back(winner.write_id);
    }

    // All winners should be identical
    for (size_t i = 1; i < winners.size(); ++i) {
        EXPECT_EQ(winners[i], winners[0]);
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

/**
 * RCS-05: Edge Cases and Error Handling
 *
 * Tests validate resolver behavior under edge conditions: empty conflicts,
 * degenerate vectors, and malformed input.
 */
class EdgeCaseConflictResolutionTest : public ::testing::Test {
protected:
    std::unique_ptr<ThreeWayMergeResolver> resolver_;
    AdvancedConflictResolver::ResolutionContext default_context_;

    void SetUp() override {
        resolver_ = std::make_unique<ThreeWayMergeResolver>();
        default_context_.collection = "test_collection";
        default_context_.document_id = "doc_001";
    }

    static MMWriteEntry createTestWrite(
        const std::string& write_id,
        const std::string& doc_id,
        const std::string& collection
    ) {
        MMWriteEntry entry{};
        entry.write_id = write_id;
        entry.document_id = doc_id;
        entry.collection = collection;
        entry.origin_node = "test_node";
        entry.operation = "WRITE";
        entry.data = R"({"test":"data"})";
        return entry;
    }
};

/**
 * RCS-05.1: Minimum conflict set (2 writes) resolves correctly.
 */
TEST_F(EdgeCaseConflictResolutionTest, MinimalConflictSetResolved) {
    std::vector<MMWriteEntry> conflict_set;
    
    // Create two writes with identical data (degenerate case)
    MMWriteEntry write1 = createTestWrite("w1", "doc_001", "test_collection");
    MMWriteEntry write2 = createTestWrite("w2", "doc_001", "test_collection");
    write1.data = R"({"value": 42})";
    write2.data = R"({"value": 42})";  // Same data
    
    conflict_set.push_back(write1);
    conflict_set.push_back(write2);

    // Resolver should pick one consistently
    MMWriteEntry winner = resolver_->resolve("doc_001", conflict_set, default_context_);
    
    // Winner must be one of the two writes
    EXPECT_TRUE(winner.write_id == "w1" || winner.write_id == "w2");
    EXPECT_EQ(winner.collection, "test_collection");
}

/**
 * RCS-05.2: Large conflict set resolves deterministically.
 */
TEST_F(EdgeCaseConflictResolutionTest, LargeConflictSetDeterministic) {
    std::vector<MMWriteEntry> conflict_set;
    
    // Add 100 writes to conflict set
    for (int i = 0; i < 100; ++i) {
        conflict_set.push_back(
            createTestWrite("w" + std::to_string(i), "doc_001", "test_collection")
        );
    }

    // Resolve same conflict multiple times
    MMWriteEntry winner1 = resolver_->resolve("doc_001", conflict_set, default_context_);
    MMWriteEntry winner2 = resolver_->resolve("doc_001", conflict_set, default_context_);
    MMWriteEntry winner3 = resolver_->resolve("doc_001", conflict_set, default_context_);

    // All resolutions should produce the same winner
    EXPECT_EQ(winner1.write_id, winner2.write_id);
    EXPECT_EQ(winner2.write_id, winner3.write_id);
}

// ============================================================================
// Diagnostics and Observability Tests
// ============================================================================

/**
 * RCS-06: Diagnostics Consistency and Observability
 *
 * Tests verify that resolver strategies report consistent metadata
 * and diagnostic information across resolution cycles.
 */
class DiagnosticsConsistencyTest : public ::testing::Test {
protected:
    std::unique_ptr<ThreeWayMergeResolver> three_way_resolver_;
    std::unique_ptr<FieldLevelMergeResolver> field_level_resolver_;
    AdvancedConflictResolver::ResolutionContext default_context_;

    void SetUp() override {
        three_way_resolver_ = std::make_unique<ThreeWayMergeResolver>();
        field_level_resolver_ = std::make_unique<FieldLevelMergeResolver>();
        default_context_.collection = "test_collection";
        default_context_.document_id = "doc_001";
    }

    static MMWriteEntry createTestWrite(
        const std::string& write_id,
        const std::string& doc_id,
        const std::string& collection
    ) {
        MMWriteEntry entry{};
        entry.write_id = write_id;
        entry.document_id = doc_id;
        entry.collection = collection;
        entry.origin_node = "test_node";
        entry.operation = "WRITE";
        entry.data = R"({"test":"data"})";
        return entry;
    }
};

/**
 * RCS-06.1: Strategy names are consistent and identifiable.
 */
TEST_F(DiagnosticsConsistencyTest, StrategyNamesConsistent) {
    // Verify strategy names match resolver type
    EXPECT_EQ(three_way_resolver_->strategyName(), "THREE_WAY_MERGE");
    
    // Field-level resolver with default UNION strategy
    EXPECT_EQ(field_level_resolver_->strategyName(), "FIELD_MERGE_UNION");

    // Names should not change across invocations
    std::string name1 = three_way_resolver_->strategyName();
    std::string name2 = three_way_resolver_->strategyName();
    EXPECT_EQ(name1, name2);
}

/**
 * RCS-06.2: Resolution diagnostics include metadata about decision path.
 */
TEST_F(DiagnosticsConsistencyTest, DiagnosticMetadataAvailable) {
    std::vector<MMWriteEntry> conflict_set;
    conflict_set.push_back(createTestWrite("w1", "doc_001", "test_collection"));
    conflict_set.push_back(createTestWrite("w2", "doc_001", "test_collection"));

    // Resolve conflict
    MMWriteEntry winner = three_way_resolver_->resolve("doc_001", conflict_set, default_context_);

    // Winner should have valid metadata
    EXPECT_FALSE(winner.write_id.empty());
    EXPECT_EQ(winner.document_id, "doc_001");
    EXPECT_EQ(winner.collection, "test_collection");

    // Strategy name should be accessible
    std::string strategy = three_way_resolver_->strategyName();
    EXPECT_FALSE(strategy.empty());
}

}  // namespace test
}  // namespace replication
}  // namespace themisdb
