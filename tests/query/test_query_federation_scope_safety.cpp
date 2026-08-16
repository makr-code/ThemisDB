/**
 * @file test_query_federation_scope_safety.cpp
 * @brief Phase 2 Agent 3: Federated query scope safety tests
 * @version 0.1.0
 * @note Status: Phase 2 Executor Scope Enforcement
 * 
 * Tests for scope isolation and enforcement in federated query execution:
 * - Per-shard scope validation
 * - Cross-shard result merging with scope isolation
 * - Accumulated size limits per scope
 * - Federated RAG result scope safety
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "query/scope_enforcer.h"
#include "query/query_federation.h"
#include "sharding/shard_router.h"
#include "utils/expected.h"

using namespace themis;
using namespace themis::query;

// ============================================================================
// Test Fixtures & Utilities
// ============================================================================

class ScopeEnforcerTest : public ::testing::Test {
protected:
    std::unique_ptr<ScopeEnforcer> enforcer;
    
    void SetUp() override {
        enforcer = std::make_unique<ScopeEnforcerImpl>();
    }
};

class FederatedScopeIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<ScopeEnforcer> enforcer;
    
    void SetUp() override {
        enforcer = std::make_unique<ScopeEnforcerImpl>();
    }
    
    // Helper to create test JSON result with scope metadata
    std::string createScopedResult(
        const std::string& collection,
        const std::string& shard_id,
        uint64_t generation = 1) {
        nlohmann::json result;
        result["_scope"] = {
            {"collection", collection},
            {"shard", shard_id},
            {"generation", generation},
            {"is_federated", true}
        };
        result["data"] = "test_result";
        return result.dump();
    }
};

// ============================================================================
// ScopeEnforcer Unit Tests
// ============================================================================

TEST_F(ScopeEnforcerTest, ExtractResultScopeFromValidJson) {
    nlohmann::json json_result;
    json_result["_scope"] = {
        {"collection", "test_collection"},
        {"shard", "shard_1"},
        {"generation", 42},
        {"is_federated", true}
    };
    
    const auto scope = enforcer->extractResultScope(json_result.dump());
    
    EXPECT_EQ(scope.collection_name, "test_collection");
    EXPECT_EQ(scope.shard_id, "shard_1");
    EXPECT_EQ(scope.scope_generation, 42);
    EXPECT_TRUE(scope.is_federated);
}

TEST_F(ScopeEnforcerTest, ExtractResultScopeFromEmptyJson) {
    nlohmann::json json_result = nlohmann::json::object();
    
    const auto scope = enforcer->extractResultScope(json_result.dump());
    
    EXPECT_EQ(scope.collection_name, "");
    EXPECT_EQ(scope.shard_id, "");
    EXPECT_EQ(scope.scope_generation, 0);
    EXPECT_FALSE(scope.is_federated);
}

TEST_F(ScopeEnforcerTest, ExtractResultScopeFromInvalidJson) {
    const auto scope = enforcer->extractResultScope("not valid json");
    
    // Should return default scope without throwing
    EXPECT_EQ(scope.collection_name, "");
    EXPECT_EQ(scope.shard_id, "");
}

TEST_F(ScopeEnforcerTest, ValidateResultScopeMatch) {
    const auto scoped_result = createScopedResult("users", "shard_1", 1);
    QueryScope expected_scope;
    expected_scope.collection_name = "users";
    expected_scope.shard_id = "shard_1";
    expected_scope.is_federated = true;
    
    const auto result = enforcer->validateResultScope(scoped_result, expected_scope);
    
    EXPECT_TRUE(result);
}

TEST_F(ScopeEnforcerTest, ValidateResultScopeCollectionMismatch) {
    const auto scoped_result = createScopedResult("users", "shard_1", 1);
    QueryScope expected_scope;
    expected_scope.collection_name = "products";  // Different collection
    expected_scope.shard_id = "shard_1";
    expected_scope.is_federated = true;
    
    const auto result = enforcer->validateResultScope(scoped_result, expected_scope);
    
    EXPECT_FALSE(result);
    EXPECT_TRUE(result.error().context().find("Scope mismatch") != std::string::npos);
}

TEST_F(ScopeEnforcerTest, ValidateResultScopeShardMismatch) {
    const auto scoped_result = createScopedResult("users", "shard_1", 1);
    QueryScope expected_scope;
    expected_scope.collection_name = "users";
    expected_scope.shard_id = "shard_2";  // Different shard
    expected_scope.is_federated = true;
    
    const auto result = enforcer->validateResultScope(scoped_result, expected_scope);
    
    EXPECT_FALSE(result);
    EXPECT_TRUE(result.error().context().find("Federated scope mismatch") != std::string::npos);
}

TEST_F(ScopeEnforcerTest, EnforceAccumulatedScopeBoundsFirstResult) {
    const std::string scope_key = "users:shard_1";
    const uint64_t max_bytes = 1000;
    const uint64_t result_bytes = 500;
    
    const auto result = enforcer->enforceAccumulatedScopeBounds(
        scope_key, result_bytes, max_bytes);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes(scope_key), result_bytes);
}

TEST_F(ScopeEnforcerTest, EnforceAccumulatedScopeBoundsAccumulation) {
    const std::string scope_key = "users:shard_1";
    const uint64_t max_bytes = 1000;
    
    // First result: 300 bytes
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(scope_key, 300, max_bytes));
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes(scope_key), 300);
    
    // Second result: 400 bytes (total 700)
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(scope_key, 400, max_bytes));
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes(scope_key), 700);
    
    // Third result: 300 bytes (would exceed)
    const auto result = enforcer->enforceAccumulatedScopeBounds(scope_key, 300, max_bytes);
    EXPECT_FALSE(result);
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes(scope_key), 700);  // Unchanged
}

TEST_F(ScopeEnforcerTest, EnforceAccumulatedScopeBoundsExceeded) {
    const std::string scope_key = "users:shard_1";
    const uint64_t max_bytes = 100;
    const uint64_t result_bytes = 150;  // Exceeds limit
    
    const auto result = enforcer->enforceAccumulatedScopeBounds(
        scope_key, result_bytes, max_bytes);
    
    EXPECT_FALSE(result);
    EXPECT_TRUE(result.error().context().find("exceeds per-scope limit") != std::string::npos);
}

TEST_F(ScopeEnforcerTest, ResetScopeAccumulation) {
    const std::string scope_key = "users:shard_1";
    
    // Accumulate some bytes
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(scope_key, 500, 1000));
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes(scope_key), 500);
    
    // Reset
    enforcer->resetScopeAccumulation(scope_key);
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes(scope_key), 0);
}

TEST_F(ScopeEnforcerTest, ValidatePageScopeValidRange) {
    QueryScope scope;
    scope.collection_name = "users";
    
    // Valid range: [0, 100) within total_size=1000
    const auto result = enforcer->validatePageScope(0, 100, 1000, scope);
    EXPECT_TRUE(result);
}

TEST_F(ScopeEnforcerTest, ValidatePageScopeBeginOffsetOutOfRange) {
    QueryScope scope;
    scope.collection_name = "users";
    
    // Invalid: begin_offset > total_size
    const auto result = enforcer->validatePageScope(1500, 1600, 1000, scope);
    EXPECT_FALSE(result);
}

TEST_F(ScopeEnforcerTest, ValidatePageScopeEndOffsetOutOfRange) {
    QueryScope scope;
    scope.collection_name = "users";
    
    // Invalid: end_offset > total_size
    const auto result = enforcer->validatePageScope(900, 1500, 1000, scope);
    EXPECT_FALSE(result);
}

TEST_F(ScopeEnforcerTest, ValidatePageScopeInvertedRange) {
    QueryScope scope;
    scope.collection_name = "users";
    
    // Invalid: begin_offset > end_offset
    const auto result = enforcer->validatePageScope(600, 400, 1000, scope);
    EXPECT_FALSE(result);
}

// ============================================================================
// Federated Result Merging Scope Tests
// ============================================================================

TEST_F(FederatedScopeIntegrationTest, MultiShardScopeIsolation) {
    // Simulate results from multiple shards
    std::vector<std::string> shard_results;
    shard_results.push_back(createScopedResult("users", "shard_1", 1));
    shard_results.push_back(createScopedResult("users", "shard_2", 1));
    shard_results.push_back(createScopedResult("users", "shard_3", 1));
    
    // Validate each result against its expected scope
    QueryScope expected_scope;
    expected_scope.collection_name = "users";
    expected_scope.is_federated = true;
    
    for (size_t i = 0; i < shard_results.size(); ++i) {
        expected_scope.shard_id = "shard_" + std::to_string(i + 1);
        const auto result = enforcer->validateResultScope(
            shard_results[i], expected_scope);
        EXPECT_TRUE(result);
    }
}

TEST_F(FederatedScopeIntegrationTest, CrossShardScopeContamination) {
    // Create a result with wrong shard ID
    const auto contaminated_result = createScopedResult("users", "shard_1", 1);
    
    // Try to validate against different shard
    QueryScope expected_scope;
    expected_scope.collection_name = "users";
    expected_scope.shard_id = "shard_2";  // Different!
    expected_scope.is_federated = true;
    
    const auto result = enforcer->validateResultScope(
        contaminated_result, expected_scope);
    
    EXPECT_FALSE(result);
}

TEST_F(FederatedScopeIntegrationTest, AccumulatedSizePerShardScope) {
    // Simulate merging results from two shards with per-shard limits
    const uint64_t max_bytes_per_scope = 2000;
    
    // Shard 1: accumulate 800 bytes
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(
        "users:shard_1", 500, max_bytes_per_scope));
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(
        "users:shard_1", 300, max_bytes_per_scope));
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes("users:shard_1"), 800);
    
    // Shard 2: accumulate 900 bytes
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(
        "users:shard_2", 600, max_bytes_per_scope));
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(
        "users:shard_2", 300, max_bytes_per_scope));
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes("users:shard_2"), 900);
    
    // Each shard stays within limits independently
    EXPECT_LT(enforcer->getScopeAccumulatedBytes("users:shard_1"), max_bytes_per_scope);
    EXPECT_LT(enforcer->getScopeAccumulatedBytes("users:shard_2"), max_bytes_per_scope);
}

TEST_F(FederatedScopeIntegrationTest, ShardScopeExceedance) {
    const uint64_t max_bytes_per_scope = 1000;
    
    // Accumulate within limit
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(
        "users:shard_1", 700, max_bytes_per_scope));
    
    // Next result would exceed limit
    const auto result = enforcer->enforceAccumulatedScopeBounds(
        "users:shard_1", 400, max_bytes_per_scope);
    
    EXPECT_FALSE(result);
}

TEST_F(FederatedScopeIntegrationTest, MultipleCollectionsMultipleShards) {
    const uint64_t max_bytes = 2000;
    
    // Different collections can accumulate independently
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(
        "users:shard_1", 1000, max_bytes));
    EXPECT_TRUE(enforcer->enforceAccumulatedScopeBounds(
        "products:shard_1", 1500, max_bytes));
    
    // Each should be within limits
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes("users:shard_1"), 1000);
    EXPECT_EQ(enforcer->getScopeAccumulatedBytes("products:shard_1"), 1500);
}

// ============================================================================
// Pagination Scope Validation Tests
// ============================================================================

TEST_F(FederatedScopeIntegrationTest, PaginationWithinScopeRange) {
    QueryScope scope;
    scope.collection_name = "users";
    scope.shard_id = "shard_1";
    
    // Page 1: items 0-99 out of 1000 total
    const auto page1 = enforcer->validatePageScope(0, 100, 1000, scope);
    EXPECT_TRUE(page1);
    
    // Page 2: items 100-199 out of 1000 total
    const auto page2 = enforcer->validatePageScope(100, 200, 1000, scope);
    EXPECT_TRUE(page2);
    
    // Last page: items 900-999 out of 1000 total
    const auto last_page = enforcer->validatePageScope(900, 1000, 1000, scope);
    EXPECT_TRUE(last_page);
}

TEST_F(FederatedScopeIntegrationTest, PaginationCrossesScopeRangeDetected) {
    QueryScope scope;
    scope.collection_name = "users";
    scope.shard_id = "shard_1";
    
    // Attempt to read beyond total_size
    const auto result = enforcer->validatePageScope(900, 1100, 1000, scope);
    EXPECT_FALSE(result);
}

// ============================================================================
// Complex Scenario Tests
// ============================================================================

TEST_F(FederatedScopeIntegrationTest, ComplexFederatedQueryExecution) {
    // Simulate executing a federated query across 3 shards with scope enforcement
    
    struct ShardQueryResult {
        std::string shard_id;
        std::vector<std::string> result_data;
        uint64_t total_bytes = 0;
    };
    
    std::vector<ShardQueryResult> shard_results = {
        {"shard_1", {createScopedResult("users", "shard_1", 1)}, 150},
        {"shard_2", {createScopedResult("users", "shard_2", 1)}, 200},
        {"shard_3", {createScopedResult("users", "shard_3", 1)}, 175}
    };
    
    QueryScope collection_scope;
    collection_scope.collection_name = "users";
    collection_scope.is_federated = true;
    
    uint64_t total_merged_bytes = 0;
    const uint64_t max_per_scope = 5000;
    
    for (const auto& shard_result : shard_results) {
        collection_scope.shard_id = shard_result.shard_id;
        
        for (const auto& result_data : shard_result.result_data) {
            // Validate scope
            EXPECT_TRUE(enforcer->validateResultScope(result_data, collection_scope));
            
            // Enforce per-shard accumulated limit
            const auto enforce_result = enforcer->enforceAccumulatedScopeBounds(
                "users:" + shard_result.shard_id,
                shard_result.total_bytes,
                max_per_scope);
            EXPECT_TRUE(enforce_result);
            
            total_merged_bytes += shard_result.total_bytes;
        }
    }
    
    // Verify total bytes
    EXPECT_EQ(total_merged_bytes, 150 + 200 + 175);
}

TEST_F(FederatedScopeIntegrationTest, SequentialPaginationWithScopeTracking) {
    const uint64_t total_results = 1000;
    const uint64_t page_size = 100;
    
    QueryScope scope;
    scope.collection_name = "users";
    scope.shard_id = "shard_1";
    
    // Paginate through entire result set
    for (size_t offset = 0; offset < total_results; offset += page_size) {
        const auto begin = offset;
        const auto end = std::min(offset + page_size, total_results);
        
        const auto page_result = enforcer->validatePageScope(begin, end, total_results, scope);
        EXPECT_TRUE(page_result) << "Failed at page offset " << offset;
    }
}

} // namespace
