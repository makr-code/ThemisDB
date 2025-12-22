# Sharding & Horizontal Scaling Documentation

**Stand:** 19. Dezember 2025  
**Version:** 1.2.0  
**Kategorie:** Sharding

---

Documentation for ThemisDB's sharding and horizontal scaling capabilities.

## 🎯 Wichtige Dokumente

### Komplexitätsanalyse & Implementierung ✅
- **[SHARDING_ANALYSIS_EXECUTIVE_SUMMARY.md](SHARDING_ANALYSIS_EXECUTIVE_SUMMARY.md)** - 📊 **Executive Summary** für Management/Stakeholder
- **[SHARDING_COMPLEXITY_ANALYSIS.md](SHARDING_COMPLEXITY_ANALYSIS.md)** - 🔬 **Vollständige technische Analyse** (1,449 Zeilen)
- **[P0_IMPLEMENTATION_SUMMARY.md](P0_IMPLEMENTATION_SUMMARY.md)** - 🛠️ **P0 Implementierungsdetails** (Circuit Breaker + Idempotent Migration)
- **[ipv6_support.md](ipv6_support.md)** - 🌐 **IPv6 Support** in URN System (NEU)

> **Analyse & Implementierung (8. Dez 2025):** Umfassende Bewertung der Sharding-Komplexität mit 6 Risikobereichen, 13 Mitigation-Strategien und **vollständige Implementierung von P0 + P1.1 + P1.2** für enterprise-ready Sharding mit automatischem Failover.

> **IPv6 Support (19. Dez 2025):** Vollständige IPv6-Kompatibilität im URN System mit RFC 3986-konformer Endpoint-Parsing. Unterstützt IPv6, IPv4 und Hostnamen in allen Formaten mit vollständiger Rückwärtskompatibilität.

## Contents

> **📋 Autoritative Dokumentation:** [sharding_overview.md](sharding_overview.md)

### Hauptdokumentation
- **[sharding_overview.md](sharding_overview.md)** - **Autoritative Quelle** für Implementierungsstand (Phase 1-6)
- **[sharding_strategy.md](sharding_strategy.md)** - Vollständige Implementierungsstrategie mit PKI-Integration
- **[ipv6_support.md](ipv6_support.md)** - IPv6-Kompatibilität und Deployment-Guide

### Archivierte Dokumente (historisch)
- **implementation_summary.md** - Phase 1 Zusammenfassung (veraltet, siehe Overview)
- **phase1_report.md** - Phase 1 Report (historisch)
- **phases_1-3_summary.md** - Phasen 1-3 Summary (historisch)
- **horizontal_scaling_strategy.md** - Strategie-Dokument (historisch)

### Weitere Ressourcen
- **[sharding_scaling_todo.md](sharding_scaling_todo.md)** - Vollständige Audit-Dokumentation und TODO-Liste
- **[sharding_redundancy.md](sharding_redundancy.md)** - RAID-ähnliche Redundanz-Modi + **Cross-Shard Graph/Hybrid Search** mit Hub-Shard Knoten
- **[sharding_streaming.md](sharding_streaming.md)** - Cassandra-inspired Streaming Protocol

## Features

✅ **Phase 1: Foundation** (Completed)
- Horizontal data partitioning
- Consistent hashing for shard assignment
- PKI-based mutual TLS authentication
- Cross-shard query execution
- **IPv6 endpoint support** (NEU - 19. Dez 2025)

✅ **Phase 2-3: Auto-Rebalancing** (Completed)
- Multi-criteria load detection (Storage, Request, Latency, Resource)
- Automatic rebalancing coordination
- Safety mechanisms (Cooldown, Concurrency limits, Daily limits)
- Full observability (Prometheus + OpenTelemetry)

✅ **Phase 4: Advanced Features** (Completed)
- etcd v3 Metadata Store Integration
- Real Health Check System (Cert validity, Storage, Network)
- Multi-DC Cloud Agent with Scatter-Gather
- Cross-Shard Join Optimization (Hash Join, Co-Located Join)
- P2P Gossip Protocol (SWIM-based, optional)

✅ **Cloud Agent** (Completed)
- Remote operation delegation across shards
- Parallel scatter-gather execution
- Health monitoring and reporting
- Cloud service integration interface (AWS, Azure, GCP)
- Async operation handling with progress tracking

✅ **Phase 5: Testing** (Completed)
- Integration Tests (test_sharding_integration.cpp) - 14 Tests
- E2E Tests (test_sharding_e2e.cpp) - 11 Tests
- Chaos Tests (test_sharding_chaos.cpp) - 13 Tests

⚠️ **Phase 6: Monitoring** (Partial)
- Prometheus Metrics (basic structure)
- Health Checks (integrated)
- Admin Endpoints (partial)

✅ **P0: Production Readiness** (Completed)
- Circuit Breaker Pattern - Prevents cascade failures
- Idempotent Data Migration - Retry-safe operations
- 50+ comprehensive tests

✅ **P1.1: WAL-based Replica Sync** (Completed)
- WAL Manager - Sequential log management with LSN tracking
- WAL Shipper - Async background replication to replicas
- WAL Applier - Replica-side log application
- 70+ integration tests

✅ **P1.2: Raft Consensus** (Completed)
- State Machine - Automatic leader election
- Log Replication - AppendEntries RPC with strong consistency
- Membership Changes - Dynamic cluster scaling (joint consensus)
- WAL Integration - Quorum-based writes with automatic failover
- 62+ comprehensive tests

## IPv6 Support

ThemisDB now fully supports IPv6 addresses for shard endpoints:

### Supported Formats

```yaml
# IPv6 with brackets and port (RFC 3986)
primary_endpoint: "[2001:db8::1]:8080"

# IPv6 localhost
primary_endpoint: "[::1]:8080"

# IPv4 (backward compatible)
primary_endpoint: "192.168.1.1:8080"

# Hostname (backward compatible)
primary_endpoint: "shard-001.dc1.example.com:8080"
```

### Key Features

- ✅ RFC 3986 compliant endpoint parsing
- ✅ IPv6 bracket notation support
- ✅ Full backward compatibility with IPv4 and hostnames
- ✅ Protocol prefix handling (`https://[::1]:8080`)
- ✅ Default port support (8080)
- ✅ 20+ comprehensive tests

**See:** [ipv6_support.md](ipv6_support.md) for complete documentation

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
