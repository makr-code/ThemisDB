# API Gateway and Query Federation

## Overview

ThemisDB provides an **API Gateway** and **Query Federation** system for unified request handling and distributed query execution across multiple shards in a cluster.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                       Client                            │
└───────────────────────┬─────────────────────────────────┘
                        │
                        ↓
┌─────────────────────────────────────────────────────────┐
│                    API Gateway                          │
│  ┌──────────────────────────────────────────────────┐  │
│  │  • Authentication & Authorization                 │  │
│  │  • Rate Limiting & Load Shedding                 │  │
│  │  • Circuit Breaking                               │  │
│  │  • Request Routing                                │  │
│  └──────────────────────────────────────────────────┘  │
└───────┬─────────────────────────────────┬───────────────┘
        │                                 │
        ↓ (Local)                         ↓ (Distributed)
┌───────────────────┐           ┌─────────────────────────┐
│  Local Handler    │           │   Shard Router          │
└───────────────────┘           │  ┌───────────────────┐  │
                                │  │ Query Federation  │  │
                                │  └───────────────────┘  │
                                └──┬──────────────┬───────┘
                                   │              │
                        ┌──────────┴─────┬────────┴────────┐
                        ↓                ↓                 ↓
                   ┌────────┐      ┌────────┐       ┌────────┐
                   │Shard 1 │      │Shard 2 │       │Shard N │
                   └────────┘      └────────┘       └────────┘
```

## Features

### API Gateway

The API Gateway provides a unified entry point for all API requests with the following capabilities:

- **Authentication & Authorization**: Validates user credentials and permissions
- **Rate Limiting**: Prevents API abuse by limiting request rates per client
- **Load Shedding**: Rejects requests when system is overloaded
- **Circuit Breaking**: Prevents cascading failures by stopping requests to failing backends
- **Request Routing**: Routes requests to appropriate local or remote handlers
- **Metrics & Monitoring**: Tracks request counts, latencies, and errors

### Query Federation

Query Federation enables distributed query execution across multiple shards:

- **Query Decomposition**: Breaks down queries into shard-specific sub-queries
- **Parallel Execution**: Executes queries across shards in parallel
- **Result Aggregation**: Merges results from multiple shards
- **Cross-Shard JOINs**: Optimizes JOINs using broadcast or shuffle strategies
- **Aggregate Pushdown**: Pushes partial aggregations to shards
- **Partition Pruning**: Only queries relevant shards based on predicates

## Usage

### API Gateway Configuration

```cpp
#include "server/api_gateway.h"

using namespace themis::server;

// Configure API Gateway
APIGateway::Config config;
config.gateway_id = "gateway-001";
config.datacenter = "dc1";
config.enable_sharding = true;
config.enable_query_federation = true;
config.enable_rate_limiting = true;
config.enable_load_shedding = true;
config.max_concurrent_requests = 1000;

// Create gateway with dependencies
auto gateway = std::make_shared<APIGateway>(
    config,
    auth_middleware,
    rate_limiter,
    load_shedder,
    shard_router,
    metrics
);
```

### Handling Requests

```cpp
// Define local request handler
auto local_handler = [](const auto& req) {
    // Handle request locally
    return response;
};

// Handle incoming request through gateway
auto response = gateway->handleRequest(request, local_handler);
```

### Executing Federated Queries

```cpp
#include "query/query_federation.h"

using namespace themis::query;

// Configure query federation
QueryFederation::Config config;
config.enable_pushdown = true;
config.enable_parallel_execution = true;
config.max_parallel_shards = 10;

// Create federation engine
auto federation = std::make_shared<QueryFederation>(
    shard_router,
    config
);

// Execute federated query
std::string query = R"(
    FOR user IN users
    FILTER user.age > 25
    RETURN user
)";

auto results = federation->execute(query);
```

### Cross-Shard JOINs

```cpp
// Execute optimized cross-shard JOIN
auto results = federation->executeJoin(
    "orders",           // Left collection
    "customers",        // Right collection  
    "orders.customer_id == customers.id"  // Join condition
);
```

## Query Federation Strategies

### 1. Scatter-Gather

Sends the same query to all shards and merges results.

**Best for**: Queries without partition keys, full table scans

```
Query → [Shard1, Shard2, ..., ShardN] → Merge → Results
```

### 2. Partition Pruning

Analyzes predicates to determine relevant shards only.

**Best for**: Queries with partition key predicates

```
Query + Predicate Analysis → [Shard2, Shard5] → Merge → Results
```

### 3. Broadcast JOIN

Broadcasts the smaller table to all shards for local JOINs.

**Best for**: Small table × Large table JOINs

```
Small Table → [Broadcast to all shards]
Large Table → [Join locally on each shard]
Results → [Merge]
```

### 4. Shuffle JOIN

Redistributes data based on join key for local JOINs.

**Best for**: Large table × Large table JOINs

```
Table A → [Redistribute by join key]
Table B → [Redistribute by join key]
[Local joins on each shard]
Results → [Merge]
```

### 5. Map-Reduce

Performs partial aggregations on shards, combines locally.

**Best for**: Aggregation queries (COUNT, SUM, AVG, GROUP BY)

```
Query → [Map: Partial aggregation on each shard]
      → [Reduce: Combine partial results]
      → Final Results
```

## Performance Considerations

### 1. Query Optimization

- Use predicates to enable partition pruning
- Limit result sets with LIMIT clause
- Use indexes for better performance

### 2. JOIN Optimization

- Keep dimension tables small for broadcast JOINs
- Consider denormalization to avoid cross-shard JOINs
- Use partition key as join key when possible

### 3. Resource Management

- Configure `max_parallel_shards` based on cluster capacity
- Set appropriate timeouts for queries
- Monitor query execution times

### 4. Caching

- Enable result caching for frequently executed queries
- Use semantic cache for similar queries
- Configure cache TTL appropriately

## Monitoring and Metrics

### API Gateway Metrics

```cpp
// Get gateway statistics
auto stats = gateway->getStatistics();

// Request metrics
auto total_requests = stats["requests"]["total"];
auto successful = stats["requests"]["successful"];
auto failed = stats["requests"]["failed"];
auto rate_limited = stats["requests"]["rate_limited"];

// Routing metrics
auto local_requests = stats["routing"]["local"];
auto distributed = stats["routing"]["distributed"];
auto federated = stats["routing"]["federated_queries"];
```

### Query Federation Metrics

```cpp
// Get federation statistics  
auto stats = federation->getStatistics();

// Query execution statistics
auto total_queries = stats["total_queries"];
auto scatter_gather = stats["scatter_gather_queries"];
auto partition_pruned = stats["partition_pruned_queries"];
auto broadcast_joins = stats["broadcast_joins"];
auto shuffle_joins = stats["shuffle_joins"];
```

## Health Checks

```cpp
// Check gateway health
auto health = gateway->getHealthStatus();

if (health["status"] == "healthy") {
    // All components operational
} else if (health["status"] == "degraded") {
    // Some errors occurring
} else {
    // Gateway unhealthy
}
```

## Configuration Options

### API Gateway Options

| Option | Default | Description |
|--------|---------|-------------|
| `enable_sharding` | false | Enable distributed routing |
| `enable_query_federation` | false | Enable cross-shard queries |
| `enable_rate_limiting` | true | Enable rate limiting |
| `enable_load_shedding` | true | Enable load shedding |
| `enable_circuit_breaker` | true | Enable circuit breaking |
| `max_concurrent_requests` | 1000 | Maximum concurrent requests |
| `request_timeout_ms` | 30000 | Request timeout in milliseconds |

### Query Federation Options

| Option | Default | Description |
|--------|---------|-------------|
| `enable_pushdown` | true | Push filters to shards |
| `enable_parallel_execution` | true | Execute queries in parallel |
| `enable_result_streaming` | false | Stream results as they arrive |
| `max_parallel_shards` | 10 | Maximum concurrent shard queries |
| `query_timeout_ms` | 60000 | Query timeout in milliseconds |
| `enable_broadcast_join` | true | Enable broadcast JOIN strategy |
| `broadcast_threshold_bytes` | 10MB | Threshold for broadcast vs shuffle JOIN |

## Error Handling

### Common Errors

1. **Authentication Failed**: Invalid credentials or expired tokens
2. **Rate Limit Exceeded**: Client has exceeded allowed request rate
3. **Service Unavailable**: System overloaded (load shedding active)
4. **Circuit Breaker Open**: Too many failures, requests blocked
5. **Query Timeout**: Federated query exceeded timeout
6. **Shard Unavailable**: One or more shards failed to respond

### Retry Strategies

- Use exponential backoff for transient failures
- Check circuit breaker status before retrying
- Monitor retry success rates

## Best Practices

1. **Design for Distribution**
   - Use partition keys that align with query patterns
   - Minimize cross-shard operations
   - Denormalize where appropriate

2. **Monitor Performance**
   - Track query latencies
   - Monitor shard health
   - Set up alerts for high error rates

3. **Optimize Queries**
   - Use LIMIT to reduce result set sizes
   - Add predicates to enable partition pruning
   - Avoid unbounded queries

4. **Handle Failures Gracefully**
   - Implement circuit breakers
   - Use timeouts appropriately
   - Provide fallback responses

5. **Test at Scale**
   - Test with realistic data volumes
   - Simulate shard failures
   - Load test the gateway

## Examples

See [examples directory](../../examples/) for complete working examples:

- `examples/api_gateway_basic.cpp` - Basic API Gateway setup
- `examples/federated_query.cpp` - Federated query examples
- `examples/cross_shard_join.cpp` - Cross-shard JOIN examples

## Further Reading

- [Sharding Architecture](../architecture/sharding.md)
- [Query Optimization](../performance/query_optimization.md)
- [Distributed Transactions](../features/distributed_transactions.md)
- [Monitoring Guide](../observability/monitoring.md)
