#include <gtest/gtest.h>
#include "api/persisted_queries.h"
#include "api/rate_limiter.h"
#include "api/audit_logger.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

using namespace themis::graphql;

// ============================================================================
// Persisted Queries Tests
// ============================================================================

TEST(PersistedQueries, RegisterAndRetrieve) {
    PersistedQueryRegistry& registry = PersistedQueryRegistry::instance();
    registry.clear();
    
    bool registered = registry.registerQuery(
        "getUser",
        "{ user(id: $id) { name email } }",
        "Get user by ID"
    );
    
    EXPECT_TRUE(registered);
    
    auto query = registry.getQuery("getUser");
    ASSERT_NE(query, nullptr);
    EXPECT_EQ(query->query_id, "getUser");
    EXPECT_EQ(query->query_text, "{ user(id: $id) { name email } }");
    EXPECT_EQ(query->description, "Get user by ID");
    EXPECT_FALSE(query->deprecated);
}

TEST(PersistedQueries, PreventDuplicateRegistration) {
    PersistedQueryRegistry& registry = PersistedQueryRegistry::instance();
    registry.clear();
    
    registry.registerQuery("test1", "{ test }");
    bool duplicate = registry.registerQuery("test1", "{ test2 }");
    
    EXPECT_FALSE(duplicate);
    
    auto query = registry.getQuery("test1");
    EXPECT_EQ(query->query_text, "{ test }");  // Original preserved
}

TEST(PersistedQueries, DeprecateQuery) {
    PersistedQueryRegistry& registry = PersistedQueryRegistry::instance();
    registry.clear();
    
    registry.registerQuery("oldQuery", "{ old }");
    bool deprecated = registry.deprecateQuery("oldQuery", "Use newQuery instead");
    
    EXPECT_TRUE(deprecated);
    
    auto query = registry.getQuery("oldQuery");
    ASSERT_NE(query, nullptr);
    EXPECT_TRUE(query->deprecated);
    EXPECT_EQ(query->deprecation_reason, "Use newQuery instead");
}

TEST(PersistedQueries, IsRegistered) {
    PersistedQueryRegistry& registry = PersistedQueryRegistry::instance();
    registry.clear();
    
    registry.registerQuery("exists", "{ test }");
    
    EXPECT_TRUE(registry.isRegistered("exists"));
    EXPECT_FALSE(registry.isRegistered("doesNotExist"));
}

TEST(PersistedQueries, GetAllQueryIds) {
    PersistedQueryRegistry& registry = PersistedQueryRegistry::instance();
    registry.clear();
    
    registry.registerQuery("query1", "{ test1 }");
    registry.registerQuery("query2", "{ test2 }");
    registry.registerQuery("query3", "{ test3 }");
    
    auto ids = registry.getAllQueryIds();
    EXPECT_EQ(ids.size(), 3);
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), "query1") != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), "query2") != ids.end());
    EXPECT_TRUE(std::find(ids.begin(), ids.end(), "query3") != ids.end());
}

// ============================================================================
// Query Allow-list Tests
// ============================================================================

TEST(QueryAllowList, AllowAndCheck) {
    QueryAllowList& allowList = QueryAllowList::instance();
    allowList.clear();
    allowList.setEnabled(true);
    
    std::string hash1 = "hash123";
    allowList.allow(hash1);
    
    EXPECT_TRUE(allowList.isAllowed(hash1));
    EXPECT_FALSE(allowList.isAllowed("unknown_hash"));
}

TEST(QueryAllowList, DisabledMode) {
    QueryAllowList& allowList = QueryAllowList::instance();
    allowList.clear();
    allowList.setEnabled(false);
    
    // When disabled, behavior depends on implementation
    // Could allow all or check list - document your choice
    EXPECT_FALSE(allowList.isEnabled());
}

TEST(QueryAllowList, RemoveFromAllowList) {
    QueryAllowList& allowList = QueryAllowList::instance();
    allowList.clear();
    
    std::string hash1 = "hash123";
    allowList.allow(hash1);
    EXPECT_TRUE(allowList.isAllowed(hash1));
    
    allowList.remove(hash1);
    EXPECT_FALSE(allowList.isAllowed(hash1));
}

TEST(QueryHasher, NormalizeQuery) {
    std::string query1 = "{ user { id   name } }";
    std::string query2 = "{user{id name}}";
    std::string query3 = "{\n  user {\n    id\n    name\n  }\n}";
    
    std::string norm1 = QueryHasher::normalize(query1);
    std::string norm2 = QueryHasher::normalize(query2);
    std::string norm3 = QueryHasher::normalize(query3);
    
    // All should normalize to same form
    EXPECT_EQ(norm1, norm2);
    EXPECT_EQ(norm2, norm3);
}

TEST(QueryHasher, NormalizeWithComments) {
    std::string query = "{ user { # comment\n id name } }";
    std::string normalized = QueryHasher::normalize(query);
    
    EXPECT_TRUE(normalized.find("#") == std::string::npos);
    EXPECT_TRUE(normalized.find("user") != std::string::npos);
}

TEST(QueryHasher, HashConsistency) {
    std::string query = "{ user { id name } }";
    std::string hash1 = QueryHasher::hash(query);
    std::string hash2 = QueryHasher::hash(query);
    
    EXPECT_EQ(hash1, hash2);
}

// ============================================================================
// Rate Limiter Tests
// ============================================================================

TEST(RateLimiter, BasicRateLimiting) {
    RateLimiter::Config config;
    config.capacity = 5;
    config.refill_rate = 1;
    
    RateLimiter limiter(config);
    
    // Should allow first 5 requests
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(limiter.allow("user1"));
    }
    
    // 6th request should be rejected
    EXPECT_FALSE(limiter.allow("user1"));
}

TEST(RateLimiter, TokenRefill) {
    RateLimiter::Config config;
    config.capacity = 5;
    config.refill_rate = 10;  // 10 tokens per second
    
    RateLimiter limiter(config);
    
    // Consume all tokens
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(limiter.allow("user1"));
    }
    
    EXPECT_FALSE(limiter.allow("user1"));
    
    // Wait for refill
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should have ~2 tokens refilled
    EXPECT_TRUE(limiter.allow("user1"));
}

TEST(RateLimiter, PerKeyLimiting) {
    RateLimiter::Config config;
    config.capacity = 2;
    
    RateLimiter limiter(config);
    
    // user1 uses tokens
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_TRUE(limiter.allow("user1"));
    EXPECT_FALSE(limiter.allow("user1"));  // Exhausted
    
    // user2 should have own tokens
    EXPECT_TRUE(limiter.allow("user2"));
    EXPECT_TRUE(limiter.allow("user2"));
}

TEST(RateLimiter, RemainingTokens) {
    RateLimiter::Config config;
    config.capacity = 10;
    
    RateLimiter limiter(config);
    
    EXPECT_EQ(limiter.remaining("user1"), 10);
    
    limiter.allow("user1");
    EXPECT_EQ(limiter.remaining("user1"), 9);
    
    limiter.allow("user1", 3);  // Consume 3 tokens
    EXPECT_EQ(limiter.remaining("user1"), 6);
}

TEST(RateLimiter, Reset) {
    RateLimiter::Config config;
    config.capacity = 2;
    
    RateLimiter limiter(config);
    
    limiter.allow("user1");
    limiter.allow("user1");
    EXPECT_FALSE(limiter.allow("user1"));
    
    limiter.reset("user1");
    EXPECT_TRUE(limiter.allow("user1"));  // Should work after reset
}

TEST(RateLimiter, Statistics) {
    RateLimiter::Config config;
    config.capacity = 2;
    
    RateLimiter limiter(config);
    
    limiter.allow("user1");
    limiter.allow("user1");
    limiter.allow("user1");  // Should be rejected
    
    auto stats = limiter.getStats();
    EXPECT_EQ(stats.allowed_requests.load(), 2);
    EXPECT_EQ(stats.rejected_requests.load(), 1);
    EXPECT_DOUBLE_EQ(stats.rejectRate(), 1.0/3.0);
}

// ============================================================================
// Operation Rate Limiter Tests
// ============================================================================

TEST(OperationRateLimiter, PerOperationLimits) {
    OperationRateLimiter& limiter = OperationRateLimiter::instance();
    limiter.clear();
    
    RateLimiter::Config queryConfig;
    queryConfig.capacity = 10;
    
    RateLimiter::Config mutationConfig;
    mutationConfig.capacity = 2;
    
    limiter.setLimit("Query", queryConfig);
    limiter.setLimit("Mutation", mutationConfig);
    
    // Queries should have higher limit
    for (int i = 0; i < 10; i++) {
        EXPECT_TRUE(limiter.allow("Query", "user1"));
    }
    EXPECT_FALSE(limiter.allow("Query", "user1"));
    
    // Mutations should have lower limit
    EXPECT_TRUE(limiter.allow("Mutation", "user1"));
    EXPECT_TRUE(limiter.allow("Mutation", "user1"));
    EXPECT_FALSE(limiter.allow("Mutation", "user1"));
}

TEST(OperationRateLimiter, RateLimitHeaders) {
    OperationRateLimiter& limiter = OperationRateLimiter::instance();
    limiter.clear();
    
    RateLimiter::Config config;
    config.capacity = 100;
    limiter.setLimit("Query", config);
    
    auto headers = limiter.getHeaders("Query", "user1");
    EXPECT_EQ(headers.limit, 100);
    EXPECT_GT(headers.remaining, 0);
}

TEST(OperationRateLimiter, ConcurrentReadsDontBlock) {
    // Verify shared_mutex: multiple allow() calls can proceed concurrently
    // (compile-time + runtime correctness check)
    OperationRateLimiter& limiter = OperationRateLimiter::instance();
    limiter.clear();

    RateLimiter::Config cfg;
    cfg.capacity = 10000;
    limiter.setLimit("Query", cfg);

    constexpr int kThreads = 4;
    constexpr int kCallsPerThread = 500;
    std::atomic<int> allowed_count{0};

    auto worker = [&]() {
        for (int i = 0; i < kCallsPerThread; ++i) {
            if (limiter.allow("Query", "user1")) {
                allowed_count.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    // All requests should be allowed (capacity 10000 > kThreads * kCallsPerThread)
    EXPECT_EQ(allowed_count.load(), kThreads * kCallsPerThread);
}

TEST(RateLimiter, StaleBucketEviction) {
    // Buckets that are fully recharged and idle for > 2×window should be evicted.
    // Eviction runs every 64 allow() calls to amortize the O(n) sweep.
    RateLimiter::Config cfg;
    cfg.capacity = 5;
    cfg.refill_rate = 1000;  // very fast refill
    cfg.window = std::chrono::seconds(1);

    RateLimiter limiter(cfg);

    // Create buckets for 10 "stale" keys
    for (int i = 0; i < 10; ++i) {
        limiter.allow("stale_" + std::to_string(i));
    }

    // Sleep > 2×window (2s) so buckets become idle and fully recharged
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));

    // Trigger the eviction sweep: eviction runs every 64 calls, so fire 64+
    for (int i = 0; i < 64; ++i) {
        limiter.allow("sweep_trigger_" + std::to_string(i));
    }

    // Evicted buckets: remaining() returns config_.capacity (no stored bucket)
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(limiter.remaining("stale_" + std::to_string(i)),
                  cfg.capacity)
            << "stale_" << i << " should have been evicted";
    }
}

// ============================================================================
// Audit Logger Tests
// ============================================================================

TEST(AuditLogger, LogEntry) {
    AuditLogger& logger = AuditLogger::instance();
    logger.clear();
    logger.clearHandlers();
    
    AuditLogEntry entry;
    entry.event_type = AuditLogEntry::EventType::QueryExecution;
    entry.operation_name = "getUser";
    entry.user_id = "user123";
    entry.success = true;
    entry.timestamp = std::chrono::system_clock::now();
    
    logger.log(entry);
    
    auto recent = logger.getRecent(1);
    ASSERT_EQ(recent.size(), 1);
    EXPECT_EQ(recent[0].operation_name, "getUser");
    EXPECT_EQ(recent[0].user_id, "user123");
}

TEST(AuditLogger, LogHandler) {
    AuditLogger& logger = AuditLogger::instance();
    logger.clear();
    logger.clearHandlers();
    
    bool handler_called = false;
    std::string logged_operation = {};
    
    logger.addHandler([&](const AuditLogEntry& entry) {
        handler_called = true;
        logged_operation = entry.operation_name;
    });
    
    AuditLogEntry entry;
    entry.event_type = AuditLogEntry::EventType::MutationExecution;
    entry.operation_name = "createUser";
    entry.timestamp = std::chrono::system_clock::now();
    
    logger.log(entry);
    
    EXPECT_TRUE(handler_called);
    EXPECT_EQ(logged_operation, "createUser");
}

TEST(AuditLogger, SearchByUser) {
    AuditLogger& logger = AuditLogger::instance();
    logger.clear();
    
    AuditLogEntry entry1;
    entry1.user_id = "user1";
    entry1.operation_name = "op1";
    entry1.timestamp = std::chrono::system_clock::now();
    logger.log(entry1);
    
    AuditLogEntry entry2;
    entry2.user_id = "user2";
    entry2.operation_name = "op2";
    entry2.timestamp = std::chrono::system_clock::now();
    logger.log(entry2);
    
    AuditLogEntry entry3;
    entry3.user_id = "user1";
    entry3.operation_name = "op3";
    entry3.timestamp = std::chrono::system_clock::now();
    logger.log(entry3);
    
    auto user1_logs = logger.searchByUser("user1");
    EXPECT_EQ(user1_logs.size(), 2);
    
    auto user2_logs = logger.searchByUser("user2");
    EXPECT_EQ(user2_logs.size(), 1);
}

TEST(AuditLogger, SearchByEventType) {
    AuditLogger& logger = AuditLogger::instance();
    logger.clear();
    
    AuditLogEntry entry1;
    entry1.event_type = AuditLogEntry::EventType::QueryExecution;
    entry1.timestamp = std::chrono::system_clock::now();
    logger.log(entry1);
    
    AuditLogEntry entry2;
    entry2.event_type = AuditLogEntry::EventType::MutationExecution;
    entry2.timestamp = std::chrono::system_clock::now();
    logger.log(entry2);
    
    auto queries = logger.searchByEventType(AuditLogEntry::EventType::QueryExecution);
    EXPECT_EQ(queries.size(), 1);
    
    auto mutations = logger.searchByEventType(AuditLogEntry::EventType::MutationExecution);
    EXPECT_EQ(mutations.size(), 1);
}

TEST(AuditLogger, Statistics) {
    AuditLogger& logger = AuditLogger::instance();
    logger.clear();
    
    AuditLogEntry success;
    success.success = true;
    success.timestamp = std::chrono::system_clock::now();
    logger.log(success);
    
    AuditLogEntry failure;
    failure.success = false;
    failure.timestamp = std::chrono::system_clock::now();
    logger.log(failure);
    
    auto stats = logger.getStats();
    EXPECT_EQ(stats.total_entries, 2);
    EXPECT_EQ(stats.failure_entries, 1);
    EXPECT_DOUBLE_EQ(stats.failureRate(), 0.5);
}

TEST(AuditLogBuilder, BuilderPattern) {
    AuditLogger& logger = AuditLogger::instance();
    logger.clear();
    
    AuditLogBuilder(AuditLogEntry::EventType::MutationExecution)
        .operationName("createUser")
        .operationType("Mutation")
        .user("user123")
        .tenant("tenant456")
        .ipAddress("192.168.1.1")
        .success(true)
        .complexity(15)
        .metadata("source", "api")
        .log();
    
    auto recent = logger.getRecent(1);
    ASSERT_EQ(recent.size(), 1);
    EXPECT_EQ(recent[0].operation_name, "createUser");
    EXPECT_EQ(recent[0].user_id, "user123");
    EXPECT_EQ(recent[0].query_complexity, 15);
}

TEST(AuditLogEntry, JSONSerialization) {
    AuditLogEntry entry;
    entry.event_type = AuditLogEntry::EventType::QueryExecution;
    entry.operation_name = "testOp";
    entry.user_id = "user1";
    entry.success = true;
    entry.timestamp = std::chrono::system_clock::now();
    entry.metadata["key1"] = "value1";
    
    std::string json = entry.toJSON();
    
    EXPECT_TRUE(json.find("\"event_type\":\"QueryExecution\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"operation_name\":\"testOp\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"user_id\":\"user1\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"success\":true") != std::string::npos);
    EXPECT_TRUE(json.find("\"key1\":\"value1\"") != std::string::npos);
}
