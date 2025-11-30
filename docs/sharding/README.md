# Sharding & Horizontal Scaling Documentation

Documentation for ThemisDB's sharding and horizontal scaling capabilities.

## Contents

- **implementation_summary.md** - Sharding implementation overview
- **phase1_report.md** - Phase 1 implementation report
- **phases_1-3_summary.md** - Phases 1-3 summary
- **horizontal_scaling_strategy.md** - Horizontal scaling strategy
- **[Auto-Rebalancing Report](../reports/SHARDING_AUTO_REBALANCING.md)** - Phase 2-3: Automatic Rebalancing (Q4 2025)

## Features

✅ **Phase 1: Foundation** (Completed)
- Horizontal data partitioning
- Consistent hashing for shard assignment
- PKI-based mutual TLS authentication
- Cross-shard query execution

✅ **Phase 2-3: Auto-Rebalancing** (Completed)
- Multi-criteria load detection (Storage, Request, Latency, Resource)
- Automatic rebalancing coordination
- Safety mechanisms (Cooldown, Concurrency limits, Daily limits)
- Full observability (Prometheus + OpenTelemetry)

✅ **Cloud Agent** (Completed)
- Remote operation delegation across shards
- Parallel scatter-gather execution
- Health monitoring and reporting
- Cloud service integration interface (AWS, Azure, GCP)
- Async operation handling with progress tracking

📋 **Phase 4: Advanced Features** (Planned Q1 2026)
- Multi-DC replication
- Geo-aware sharding
- Read replicas
- Automatic failover

## Cloud Agent

The Cloud Agent component provides a cloud-based coordination layer for managing distributed operations across ThemisDB shards.

### Features

- **Operation Delegation**: Delegate operations to remote shards with automatic retry logic
- **Scatter-Gather**: Execute queries across multiple shards in parallel and merge results
- **Health Monitoring**: Continuous health checks on all shards
- **Cloud Integration**: Interface for AWS, Azure, and GCP cloud services
- **Async Operations**: Submit operations asynchronously with progress tracking
- **Prometheus Metrics**: Full observability of cloud agent operations

### Usage Example

```cpp
#include "sharding/cloud_agent.h"

// Configure the cloud agent
CloudAgent::Config config;
config.agent_id = "agent_001";
config.datacenter = "dc1";
config.region = "eu-central-1";
config.cloud_provider = "aws";

// Create and start the agent
auto agent = std::make_unique<CloudAgent>(
    topology, executor, metrics, config
);
agent->start();

// Delegate an operation
CloudAgentOperation op;
op.operation_type = "query";
op.parameters = {{"aql", "FOR doc IN users RETURN doc"}};
op.target_shards = {};  // All shards

auto result = agent->delegate(op);
if (result.success) {
    std::cout << "Query results: " << result.result.dump() << std::endl;
}

// Or delegate asynchronously
std::string op_id = agent->delegateAsync(op);
// ... later ...
auto status = agent->getOperationStatus(op_id);

agent->stop();
```

### API Reference

| Method | Description |
|--------|-------------|
| `start()` | Start the cloud agent |
| `stop()` | Stop the cloud agent |
| `delegate(op)` | Execute operation synchronously |
| `delegateAsync(op)` | Execute operation asynchronously |
| `getOperationStatus(id)` | Get status of async operation |
| `cancelOperation(id)` | Cancel pending operation |
| `executeHealthCheck()` | Run health check on all shards |
| `getStatistics()` | Get agent statistics |
| `getHealthStatus()` | Get agent health status |

## See Also

- [Architecture](../architecture/)
- [Performance](../performance/)
- [Source Code: sharding module](../src/sharding/)
