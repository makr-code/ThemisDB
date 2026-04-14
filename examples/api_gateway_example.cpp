/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_gateway_example.cpp                            ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     374                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9564da32a9  2026-02-24  fix: processVersionHeaders query-strip, update docs and R... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file api_gateway_example.cpp
 * @brief Example demonstrating API Gateway and Query Federation usage
 * 
 * This example shows how to:
 * 1. Set up an API Gateway
 * 2. Configure Query Federation
 * 3. Execute federated queries
 * 4. Handle cross-shard operations
 */

#include "server/api_gateway.h"
#include "query/query_federation.h"
#include "server/auth_middleware.h"
#include "server/rate_limiter.h"
#include "server/load_shedder.h"
#include "sharding/shard_router.h"
#include <iostream>
#include <memory>

using namespace themis;
using namespace themis::server;
using namespace themis::query;
using namespace themis::sharding;

/**
 * @brief Example 1: Basic API Gateway Setup
 */
void example_basic_gateway() {
    std::cout << "\n=== Example 1: Basic API Gateway Setup ===\n";
    
    // Create dependencies
    auto auth = std::make_shared<AuthMiddleware>();
    auto rate_limiter = std::make_shared<RateLimiter>();
    auto load_shedder = std::make_shared<LoadShedder>();
    
    // Configure API Gateway
    APIGateway::Config config;
    config.gateway_id = "gateway-001";
    config.datacenter = "dc1";
    config.enable_sharding = false;  // Single node for this example
    config.enable_rate_limiting = true;
    config.max_concurrent_requests = 1000;
    
    // Create gateway
    auto gateway = std::make_shared<APIGateway>(
        config,
        auth,
        rate_limiter,
        load_shedder
    );
    
    std::cout << "API Gateway created: " << config.gateway_id << "\n";
    
    // Check health
    auto health = gateway->getHealthStatus();
    std::cout << "Gateway status: " << health["status"] << "\n";
    
    // Get statistics
    auto stats = gateway->getStatistics();
    std::cout << "Total requests: " << stats["requests"]["total"] << "\n";
}

/**
 * @brief Example 2: Distributed Gateway with Sharding
 */
void example_distributed_gateway() {
    std::cout << "\n=== Example 2: Distributed Gateway with Sharding ===\n";
    
    // Create shard router (requires URN resolver and remote executor)
    // Note: This example shows the structure; actual setup requires more components
    
    auto auth = std::make_shared<AuthMiddleware>();
    auto rate_limiter = std::make_shared<RateLimiter>();
    auto load_shedder = std::make_shared<LoadShedder>();
    
    // In a real deployment, you would create:
    // - URN resolver for shard location
    // - Remote executor for shard communication
    // - Shard router for query distribution
    
    APIGateway::Config config;
    config.gateway_id = "gateway-distributed";
    config.datacenter = "dc1";
    config.enable_sharding = true;
    config.enable_query_federation = true;
    config.enable_circuit_breaker = true;
    
    std::cout << "Distributed gateway configuration:\n";
    std::cout << "  - Sharding: " << (config.enable_sharding ? "enabled" : "disabled") << "\n";
    std::cout << "  - Query Federation: " << (config.enable_query_federation ? "enabled" : "disabled") << "\n";
    std::cout << "  - Circuit Breaker: " << (config.enable_circuit_breaker ? "enabled" : "disabled") << "\n";
}

/**
 * @brief Example 3: Query Federation Setup
 */
void example_query_federation() {
    std::cout << "\n=== Example 3: Query Federation Setup ===\n";
    
    // Configure query federation
    QueryFederation::Config config;
    config.enable_pushdown = true;
    config.enable_parallel_execution = true;
    config.max_parallel_shards = 10;
    config.query_timeout_ms = 60000;  // 60 seconds
    config.enable_broadcast_join = true;
    config.broadcast_threshold_bytes = 10 * 1024 * 1024;  // 10 MB
    
    std::cout << "Query Federation configuration:\n";
    std::cout << "  - Pushdown: " << (config.enable_pushdown ? "enabled" : "disabled") << "\n";
    std::cout << "  - Parallel execution: " << (config.enable_parallel_execution ? "enabled" : "disabled") << "\n";
    std::cout << "  - Max parallel shards: " << config.max_parallel_shards << "\n";
    std::cout << "  - Query timeout: " << config.query_timeout_ms << " ms\n";
    std::cout << "  - Broadcast JOIN: " << (config.enable_broadcast_join ? "enabled" : "disabled") << "\n";
}

/**
 * @brief Example 4: Executing Federated Queries
 */
void example_execute_federated_query() {
    std::cout << "\n=== Example 4: Executing Federated Queries ===\n";
    
    // Example queries that benefit from federation
    
    std::cout << "\n1. Simple SELECT query:\n";
    std::string query1 = R"(
        FOR user IN users
        FILTER user.age > 25
        RETURN user
    )";
    std::cout << "Query: " << query1 << "\n";
    std::cout << "Strategy: Scatter-gather (queries all shards)\n";
    
    std::cout << "\n2. Query with partition key:\n";
    std::string query2 = R"(
        FOR user IN users
        FILTER user.country == 'Germany'
        RETURN user
    )";
    std::cout << "Query: " << query2 << "\n";
    std::cout << "Strategy: Partition pruning (queries only relevant shards)\n";
    
    std::cout << "\n3. Aggregation query:\n";
    std::string query3 = R"(
        FOR user IN users
        COLLECT country = user.country
        AGGREGATE count = COUNT(1)
        RETURN {country: country, count: count}
    )";
    std::cout << "Query: " << query3 << "\n";
    std::cout << "Strategy: Map-reduce (partial aggregation on shards)\n";
    
    std::cout << "\n4. Query with LIMIT:\n";
    std::string query4 = R"(
        FOR user IN users
        SORT user.created_at DESC
        LIMIT 10
        RETURN user
    )";
    std::cout << "Query: " << query4 << "\n";
    std::cout << "Strategy: Fetch more from shards, apply LIMIT globally\n";
}

/**
 * @brief Example 5: Cross-Shard JOINs
 */
void example_cross_shard_joins() {
    std::cout << "\n=== Example 5: Cross-Shard JOINs ===\n";
    
    std::cout << "\n1. Broadcast JOIN (small × large):\n";
    std::cout << "   Small table (countries): 200 rows\n";
    std::cout << "   Large table (users): 1,000,000 rows\n";
    std::cout << "   Strategy: Broadcast countries to all shards\n";
    std::cout << "   Query:\n";
    std::cout << "     FOR user IN users\n";
    std::cout << "       FOR country IN countries\n";
    std::cout << "         FILTER user.country_code == country.code\n";
    std::cout << "         RETURN {user: user, country: country}\n";
    
    std::cout << "\n2. Shuffle JOIN (large × large):\n";
    std::cout << "   Table 1 (orders): 5,000,000 rows\n";
    std::cout << "   Table 2 (customers): 500,000 rows\n";
    std::cout << "   Strategy: Redistribute by join key\n";
    std::cout << "   Query:\n";
    std::cout << "     FOR order IN orders\n";
    std::cout << "       FOR customer IN customers\n";
    std::cout << "         FILTER order.customer_id == customer.id\n";
    std::cout << "         RETURN {order: order, customer: customer}\n";
}

/**
 * @brief Example 6: Monitoring and Metrics
 */
void example_monitoring() {
    std::cout << "\n=== Example 6: Monitoring and Metrics ===\n";
    
    std::cout << "\nAPI Gateway Metrics:\n";
    std::cout << "  - Total requests: 150,420\n";
    std::cout << "  - Successful: 148,950 (99.0%)\n";
    std::cout << "  - Failed: 1,470 (1.0%)\n";
    std::cout << "  - Rate limited: 450\n";
    std::cout << "  - Load shed: 120\n";
    std::cout << "  - Circuit breaker rejections: 30\n";
    
    std::cout << "\nRouting Distribution:\n";
    std::cout << "  - Local requests: 120,000 (80%)\n";
    std::cout << "  - Distributed requests: 25,000 (17%)\n";
    std::cout << "  - Federated queries: 5,420 (3%)\n";
    
    std::cout << "\nQuery Federation Metrics:\n";
    std::cout << "  - Total queries: 5,420\n";
    std::cout << "  - Scatter-gather: 3,200 (59%)\n";
    std::cout << "  - Partition pruned: 1,850 (34%)\n";
    std::cout << "  - Broadcast JOINs: 280 (5%)\n";
    std::cout << "  - Shuffle JOINs: 90 (2%)\n";
    
    std::cout << "\nPerformance:\n";
    std::cout << "  - Average query latency: 45ms\n";
    std::cout << "  - P95 query latency: 180ms\n";
    std::cout << "  - P99 query latency: 450ms\n";
}

/**
 * @brief Example 7: Error Handling
 */
void example_error_handling() {
    std::cout << "\n=== Example 7: Error Handling ===\n";
    
    std::cout << "\nCommon error scenarios:\n";
    
    std::cout << "\n1. Authentication failure:\n";
    std::cout << "   Status: 401 Unauthorized\n";
    std::cout << "   Message: Invalid or expired authentication token\n";
    
    std::cout << "\n2. Rate limit exceeded:\n";
    std::cout << "   Status: 429 Too Many Requests\n";
    std::cout << "   Message: Request rate limit exceeded, retry after 60s\n";
    
    std::cout << "\n3. Circuit breaker open:\n";
    std::cout << "   Status: 503 Service Unavailable\n";
    std::cout << "   Message: Circuit breaker open for shard-003\n";
    
    std::cout << "\n4. Query timeout:\n";
    std::cout << "   Status: 504 Gateway Timeout\n";
    std::cout << "   Message: Federated query exceeded timeout (60000ms)\n";
    
    std::cout << "\n5. Shard unavailable:\n";
    std::cout << "   Status: 503 Service Unavailable\n";
    std::cout << "   Message: Shard shard-002 is unavailable\n";
    
    std::cout << "\nRetry strategies:\n";
    std::cout << "  - Transient errors: Use exponential backoff\n";
    std::cout << "  - Rate limits: Wait for retry-after period\n";
    std::cout << "  - Circuit breaker: Check status before retry\n";
    std::cout << "  - Timeouts: Consider increasing timeout or optimizing query\n";
}

/**
 * @brief Example 8: API Versioning and Deprecation Headers
 *
 * Shows how to:
 * - Register a deprecated endpoint so the gateway emits
 *   Deprecation, Sunset (RFC 8594), and Link headers automatically.
 * - Use Accept-Version to request a specific API version.
 */
void example_api_versioning() {
    std::cout << "\n=== Example 8: API Versioning and Deprecation Headers ===\n";

    auto auth = std::make_shared<AuthMiddleware>();
    auto rate_limiter = std::make_shared<RateLimiter>();
    auto load_shedder = std::make_shared<LoadShedder>();

    APIGateway::Config config;
    config.gateway_id = "versioning-demo-gateway";
    config.datacenter = "dc1";
    config.enable_api_versioning = true;   // default: true
    config.enforce_version_check = false;  // permissive mode

    auto gateway = std::make_shared<APIGateway>(
        config, auth, rate_limiter, load_shedder
    );

    // Register that /api/v1/old-endpoint is deprecated
    APIDeprecationInfo dep_info;
    dep_info.deprecated_in = APIVersion{1, 0, 0};
    dep_info.removed_in    = APIVersion{2, 0, 0};
    dep_info.deprecation_date = std::chrono::system_clock::now();
    dep_info.removal_date     = std::chrono::system_clock::now()
                                + std::chrono::hours(24 * 730); // 730 days (~2 years, matches DEPRECATION_PERIOD_DAYS)
    dep_info.reason              = "Replaced by /api/v2/old-endpoint";
    dep_info.migration_guide_url = "https://docs.themisdb.com/migration/v1-to-v2";
    dep_info.alternative         = "/api/v2/old-endpoint";

    gateway->registerDeprecation("/api/v1/old-endpoint", dep_info);

    std::cout << "Registered deprecation for /api/v1/old-endpoint\n";
    std::cout << "  Deprecated in : " << dep_info.deprecated_in.toString() << "\n";
    std::cout << "  Removed in    : " << dep_info.removed_in.toString() << "\n";
    std::cout << "  Migration guide: " << dep_info.migration_guide_url << "\n";

    std::cout << "\nWhen a client calls GET /api/v1/old-endpoint, the gateway adds:\n";
    std::cout << "  Deprecation: true; deprecated-version=\"v1.0.0\"; removal-version=\"v2.0.0\"\n";
    std::cout << "  Sunset: <RFC 8594 HTTP-date two years from now>\n";
    std::cout << "  Link: <https://docs.themisdb.com/migration/v1-to-v2>; rel=\"deprecation\"\n";
    std::cout << "  API-Version: v1.4.1\n";

    std::cout << "\nClients can request a specific version via URL path prefix (preferred):\n";
    std::cout << "  GET /v1/entities/123  -> API-Version: v1.4.1 (latest v1)\n";
    std::cout << "  GET /v2/entities/123  -> API-Version: v2.x.x (if v2 is supported)\n";
    std::cout << "  The /v1/ prefix is stripped before reaching the handler:\n";
    std::cout << "    /v1/entities/123  -> handler sees /entities/123\n";
    std::cout << "    /v1/query?aql=... -> handler sees /query?aql=...\n";

    std::cout << "\nOr via Accept-Version header (fallback when no path prefix):\n";
    std::cout << "  Accept-Version: v1.3.0  -> API-Version: v1.3.0 in response\n";
    std::cout << "  Accept-Version: v1      -> resolves to latest v1.x.y\n";
    std::cout << "  Accept-Version: latest  -> current stable version\n";
    std::cout << "  (omitted)               -> current stable version\n";
    std::cout << "\nNote: URL path prefix takes precedence over Accept-Version header.\n";
}

/**
 * @brief Main function
 */
int main() {
    std::cout << "=================================================\n";
    std::cout << " ThemisDB API Gateway and Query Federation Demo \n";
    std::cout << "=================================================\n";
    
    try {
        // Run examples
        example_basic_gateway();
        example_distributed_gateway();
        example_query_federation();
        example_execute_federated_query();
        example_cross_shard_joins();
        example_monitoring();
        example_error_handling();
        example_api_versioning();
        
        std::cout << "\n=== All examples completed successfully ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
