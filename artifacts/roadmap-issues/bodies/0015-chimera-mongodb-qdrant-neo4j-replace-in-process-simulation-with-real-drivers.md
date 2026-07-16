### Context

This issue implements the roadmap item 'MongoDB / Qdrant / Neo4j: Replace In-Process Simulation with Real Drivers' for the chimera domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.2.0.

Primary detail section: MongoDB / Qdrant / Neo4j: Replace In-Process Simulation with Real Drivers

### Goal

Deliver the scoped changes for MongoDB / Qdrant / Neo4j: Replace In-Process Simulation with Real Drivers in src/chimera/ and complete the linked detail section in a release-ready state for v1.2.0.

### Detailed Scope

### MongoDB / Qdrant / Neo4j: Replace In-Process Simulation with Real Drivers
**Priority:** High
**Target Version:** v1.2.0

`mongodb_adapter.cpp`, `qdrant_adapter.cpp`, and `neo4j_adapter.cpp` all document explicitly that they "operate in an in-process simulation mode backed by `std::unordered_map`" when the respective native driver is absent. The simulation passes tests but does not exercise real network I/O, serialization, or back-pressure. Production benchmarks against these backends are meaningless without real driver integration.

**Implementation Notes:**
- `[ ]` **MongoDB**: Wire `#ifdef THEMIS_ENABLE_MONGODB` blocks in `mongodb_adapter.cpp` to actual `mongocxx::client` calls; add `mongocxx::instance` singleton initialization in `AdapterFactory::create()`.
- `[ ]` **Qdrant**: Wire `#ifdef THEMIS_ENABLE_QDRANT` blocks to real HTTP calls (cpp-httplib or cpr); replace simulation `std::unordered_map` with REST API calls to `POST /collections/{name}/points`.
- `[ ]` **Neo4j**: Wire `#ifdef THEMIS_ENABLE_NEO4J` blocks to Bolt protocol client (`neo4j-cpp-driver` or libneo4j-client); replace simulation path traversal with real Cypher `MATCH` queries.
- `[ ]` All three adapters return `metrics.cpu.thread_count = 0` (hardcoded) — populate from the driver's connection pool stats.
- `[ ]` Add CI integration tests using Docker Compose with real MongoDB/Qdrant/Neo4j containers.

---


**Priority:** High  
**Target Version:** v1.1.0

Efficient connection pooling for high-throughput benchmarks.

**Features:**
- Configurable pool size
- Connection health checking
- Automatic reconnection on failures
- Connection timeout management
- Pool statistics and monitoring

**Configuration:**
```cpp
ConnectionPoolConfig pool_config;
pool_config.min_connections = 5;
pool_config.max_connections = 50;
pool_config.connection_timeout = std::chrono::seconds(30);
pool_config.idle_timeout = std::chrono::minutes(5);
pool_config.health_check_interval = std::chrono::seconds(60);

auto adapter = std::make_unique<ThemisDBAdapter>(pool_config);
```

**API:**
```cpp
class ConnectionPool {
public:
    ConnectionPool(DatabaseClient* client, const ConnectionPoolConfig& config);
    
    // Acquire connection (blocks if all busy)
    std::unique_ptr<Connection> acquire(std::chrono::milliseconds timeout);
    
    // Return connection to pool
    void release(std::unique_ptr<Connection> conn);
    
    // Pool statistics
    PoolStats get_stats() const;
    
    // Health management
    void health_check();
    void remove_unhealthy_connections();
};
```

**Pool Statistics:**
```cpp
struct PoolStats {
    size_t total_connections;
    size_t active_connections;
    size_t idle_connections;
    size_t total_acquires;
    size_t total_releases;
    size_t failed_acquires;
    std::chrono::milliseconds avg_acquire_time;
    std::chrono::milliseconds avg_query_time;
};
```

---

### Acceptance Criteria

- [ ] **MongoDB**: Wire `#ifdef THEMIS_ENABLE_MONGODB` blocks in `mongodb_adapter.cpp` to actual `mongocxx::client` calls; add `mongocxx::instance` singleton initialization in `AdapterFactory::create()`.
- [ ] **Qdrant**: Wire `#ifdef THEMIS_ENABLE_QDRANT` blocks to real HTTP calls (cpp-httplib or cpr); replace simulation `std::unordered_map` with REST API calls to `POST /collections/{name}/points`.
- [ ] **Neo4j**: Wire `#ifdef THEMIS_ENABLE_NEO4J` blocks to Bolt protocol client (`neo4j-cpp-driver` or libneo4j-client); replace simulation path traversal with real Cypher `MATCH` queries.
- [ ] All three adapters return `metrics.cpu.thread_count = 0` (hardcoded) — populate from the driver's connection pool stats.
- [ ] Add CI integration tests using Docker Compose with real MongoDB/Qdrant/Neo4j containers.
- [ ] Configurable pool size
- [ ] Connection health checking
- [ ] Automatic reconnection on failures
- [ ] Connection timeout management
- [ ] Pool statistics and monitoring

### Relationships

- Roadmap row: #15 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/chimera/FUTURE_ENHANCEMENTS.md#mongodb--qdrant--neo4j-replace-in-process-simulation-with-real-drivers
- Source key: roadmap:15:chimera:v1.2.0:mongodb-qdrant-neo4j-replace-in-process-simulation-with-real-drivers

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:15:chimera:v1.2.0:mongodb-qdrant-neo4j-replace-in-process-simulation-with-real-drivers -->
<!-- roadmap-ref: row=15;module=chimera;target=v1.2.0 -->
<!-- roadmap-detail: src/chimera/FUTURE_ENHANCEMENTS.md#mongodb--qdrant--neo4j-replace-in-process-simulation-with-real-drivers -->
