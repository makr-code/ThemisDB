# Kapitel-zu-Dokumentation Mapping

**Version:** 1.0.0  
**Stand:** Dezember 2025

Dieses Dokument bildet jedes Buchkapitel auf die entsprechenden Quell-Dokumente im Repository ab.

---

## TEIL I: GRUNDLAGEN UND MOTIVATION

### Kapitel 1: Einführung in ThemisDB

**Primäre Quellen:**
- `/README.md` - Projekt-Übersicht
- `/docs/architecture/architecture_overview.md` - System-Architektur
- `/docs/reports/themis_sachstandsbericht_2025.md` - Sachstandsbericht

**Sekundäre Quellen:**
- `/docs/development/DEVELOPMENT_SUMMARY.md` - Entwicklungsstand
- `/VERSION` - Aktuelle Version
- `/CHANGELOG.md` - Release History

**Code-Beispiele:**
- Minimal-Beispiel für Entity-Erstellung
- REST API Curl-Beispiele

---

### Kapitel 2: Theoretische Grundlagen

**Primäre Quellen:**
- `/docs/architecture/architecture_mvcc.md` - MVCC-Theorie
- `/docs/storage/storage_rocksdb.md` - LSM-Tree Details
- `/docs/index/index_overview.md` - Index-Strukturen

**Sekundäre Quellen:**
- `/docs/transaction/transaction_overview.md` - Transaction Semantics
- `/docs/architecture/architecture_strategic.md` - Design Patterns

**Code-Beispiele:**
- MVCC Version Chain
- LSM Compaction Trigger

---

### Kapitel 3: Technologie-Entscheidungen

**Primäre Quellen:**
- `/docs/guides/guides_build_strategy.md` - Build Toolchain
- `/CMakeLists.txt` - Build-Konfiguration
- `/vcpkg.json` - Dependencies

**Sekundäre Quellen:**
- `/docs/guides/guides_build.md` - Build-Anleitung
- `/.clang-format` - Code Style
- `/.clang-tidy` - Static Analysis

**Code-Beispiele:**
- CMake-Konfiguration
- vcpkg-Integration

---

## TEIL II: ARCHITEKTUR UND DESIGN

### Kapitel 4: Systemarchitektur

**Primäre Quellen:**
- `/docs/architecture/architecture_overview.md` - System-Übersicht
- `/docs/architecture/architecture_strategic.md` - Strategische Architektur
- `/docs/architecture/architecture_ecosystem.md` - Ecosystem-Integration

**Sekundäre Quellen:**
- `/docs/architecture/architecture_multi_model.md` - Multi-Model Design
- `/docs/server/server_overview.md` - Server-Architektur

**Diagramme:**
- System-Schichtendiagramm (bereits in architecture_overview.md)
- Datenfluss-Diagramm (Write Path, Read Path)

**Code-Beispiele:**
- Server-Initialisierung
- Request-Handling

---

### Kapitel 5: Base Entity Design

**Primäre Quellen:**
- `/docs/architecture/architecture_base_entity.md` - Base Entity Konzept
- `/include/storage/base_entity.hpp` - Header-Datei
- `/src/storage/base_entity.cpp` - Implementierung

**Sekundäre Quellen:**
- `/docs/storage/storage_overview.md` - Storage-Layer
- `/tests/test_base_entity.cpp` - Unit Tests

**Code-Beispiele:**
```cpp
// Entity-Serialisierung
auto entity = BaseEntity::from_json(json_blob);
db->put("users:123", entity.serialize());

// Entity-Deserialisierung
auto data = db->get("users:123");
auto entity = BaseEntity::deserialize(data);
```

---

### Kapitel 6: MVCC Transaction Design

**Primäre Quellen:**
- `/docs/architecture/architecture_mvcc.md` - MVCC Design
- `/docs/transaction/transaction_overview.md` - Transaction Handling
- `/include/transaction/mvcc_transaction.hpp` - MVCC Header

**Sekundäre Quellen:**
- `/src/transaction/mvcc_transaction.cpp` - MVCC Implementation
- `/tests/test_mvcc.cpp` - MVCC Tests (468 Tests)

**Code-Beispiele:**
```cpp
// Transaction begin
auto tx = txn_manager->begin();

// Read with snapshot isolation
auto value = tx->get("users:123");

// Write with version tracking
tx->put("users:123", new_value);

// Commit
tx->commit();
```

---

### Kapitel 7: Query Engine und AQL

**Primäre Quellen:**
- `/docs/aql/aql_syntax.md` - AQL Syntax
- `/docs/aql/aql_query_engine.md` - Query Engine
- `/docs/query/query_optimizer.md` - Optimizer

**Sekundäre Quellen:**
- `/include/query/aql_parser.hpp` - Parser
- `/src/query/query_executor.cpp` - Execution Engine
- `/tests/test_aql.cpp` - AQL Tests

**Code-Beispiele:**
```aql
FOR user IN users
  FILTER user.age > 18
  COLLECT country = user.country
  AGGREGATE count = COUNT(1)
  SORT count DESC
  RETURN { country, count }
```

---

## TEIL III: KERN-KOMPONENTEN

### Kapitel 8: Storage Layer

**Primäre Quellen:**
- `/docs/storage/storage_rocksdb.md` - RocksDB Integration
- `/docs/storage/storage_tuning.md` - Performance Tuning
- `/include/storage/storage_engine.hpp` - Storage Engine Interface

**Sekundäre Quellen:**
- `/src/storage/rocksdb_wrapper.cpp` - RocksDB Wrapper
- `/config/rocksdb_options.json` - RocksDB Konfiguration

**Code-Beispiele:**
```cpp
// RocksDB Options
rocksdb::Options options;
options.create_if_missing = true;
options.write_buffer_size = 256 * 1024 * 1024; // 256 MB
options.max_write_buffer_number = 4;
options.compression = rocksdb::kLZ4Compression;
options.bottommost_compression = rocksdb::kZSTD;
```

---

### Kapitel 9: Indexierung

**Primäre Quellen:**
- `/docs/index/index_overview.md` - Index-Architektur
- `/docs/index/index_secondary.md` - Secondary Indexes
- `/include/index/index_manager.hpp` - Index Manager

**Sekundäre Quellen:**
- `/src/index/secondary_index.cpp` - Secondary Index Implementation
- `/src/index/fulltext_index.cpp` - Fulltext Index
- `/tests/test_indexes.cpp` - Index Tests

**Code-Beispiele:**
```cpp
// Create secondary index
index_manager->create_index(
  "users",
  "idx_email",
  {"email"},
  IndexType::UNIQUE
);

// Query with index
auto results = index_manager->query(
  "users",
  "idx_email",
  Condition::equals("email", "user@example.com")
);
```

---

### Kapitel 10: HTTP Server

**Primäre Quellen:**
- `/docs/server/server_overview.md` - Server-Architektur
- `/docs/api/api_reference.md` - REST API
- `/include/server/http_server.hpp` - HTTP Server Header

**Sekundäre Quellen:**
- `/src/server/http_server.cpp` - Server Implementation
- `/src/server/request_handler.cpp` - Request Handler
- `/openapi/openapi.yaml` - OpenAPI Spec

**Code-Beispiele:**
```cpp
// Server initialization
auto server = HttpServer::create(
  "0.0.0.0",
  8765,
  request_handler
);

// Start server
server->start();
```

**API-Beispiele:**
```bash
# Create entity
curl -X POST http://localhost:8765/entities/users:123 \
  -H "Content-Type: application/json" \
  -d '{"name": "Alice", "age": 30}'

# Query with AQL
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{"query": "FOR u IN users FILTER u.age > 25 RETURN u"}'
```

---

### Kapitel 11: Security

**Primäre Quellen:**
- `/docs/security/security_overview.md` - Security-Übersicht
- `/docs/security/security_encryption_strategy.md` - Encryption
- `/docs/security/security_key_management.md` - Key Management

**Sekundäre Quellen:**
- `/include/security/encryption_manager.hpp` - Encryption Manager
- `/src/security/field_encryption.cpp` - Field-Level Encryption
- `/docs/security/security_threat_model.md` - Threat Model
- `/SECURITY.md` - Security Policy

**Code-Beispiele:**
```cpp
// Field-level encryption
auto encrypted = encryption_mgr->encrypt_field(
  "users",
  "ssn",
  sensitive_data,
  key_id
);

// RBAC check
if (auth_mgr->has_permission(user_id, "users", Permission::READ)) {
  // Grant access
}
```

---

### Kapitel 12: Content Pipeline

**Primäre Quellen:**
- `/docs/architecture/architecture_content_pipeline.md` - Pipeline Architecture
- `/docs/content/content_overview.md` - Content Processing
- `/include/content/content_processor.hpp` - Processor Interface

**Sekundäre Quellen:**
- `/src/content/text_extractor.cpp` - Text Extraction
- `/src/content/entity_extractor.cpp` - Entity Extraction
- `/src/content/embedding_generator.cpp` - Embeddings

**Code-Beispiele:**
```cpp
// Content pipeline
auto pipeline = ContentPipeline::create();
pipeline->add_processor(TextExtractor::create());
pipeline->add_processor(EntityExtractor::create());
pipeline->add_processor(EmbeddingGenerator::create());

// Process document
auto result = pipeline->process(document);
```

---

## TEIL IV: MULTI-MODEL-FÄHIGKEITEN

### Kapitel 13: Graph Database

**Primäre Quellen:**
- `/docs/features/features_graph.md` - Graph Features
- `/docs/index/index_graph.md` - Graph Indexing
- `/include/index/graph_index.hpp` - Graph Index Header

**Sekundäre Quellen:**
- `/src/index/graph_traversal.cpp` - Traversal Algorithms
- `/tests/test_graph.cpp` - Graph Tests

**Code-Beispiele:**
```aql
// Create graph edge
{
  "_from": "users:alice",
  "_to": "users:bob",
  "type": "FOLLOWS",
  "since": "2025-01-01"
}

// Graph traversal
FOR vertex, edge, path IN 1..3 OUTBOUND 'users:alice' GRAPH 'social'
  RETURN path
```

---

### Kapitel 14: Vector Database

**Primäre Quellen:**
- `/docs/features/features_vector_ops.md` - Vector Operations
- `/docs/index/index_vector.md` - Vector Indexing
- `/include/index/vector_index.hpp` - HNSW Index

**Sekundäre Quellen:**
- `/src/index/hnsw_index.cpp` - HNSW Implementation
- `/tests/test_vector.cpp` - Vector Tests

**Code-Beispiele:**
```cpp
// Create vector index
index_mgr->create_vector_index(
  "documents",
  "embedding",
  VectorIndexType::HNSW,
  {
    {"metric", "cosine"},
    {"ef_construction", 200},
    {"M", 16}
  }
);

// Vector similarity search
auto results = index_mgr->vector_search(
  "documents",
  "embedding",
  query_vector,
  k=10
);
```

---

### Kapitel 15: Time Series

**Primäre Quellen:**
- `/docs/features/features_time_series.md` - Time Series Features
- `/docs/timeseries/timeseries_overview.md` - TS Architecture
- `/include/timeseries/tsstore.hpp` - TS Store

**Sekundäre Quellen:**
- `/src/timeseries/gorilla_compression.cpp` - Gorilla Codec
- `/src/timeseries/continuous_aggregate.cpp` - Aggregates
- `/tests/test_timeseries.cpp` - TS Tests

**Code-Beispiele:**
```cpp
// Insert time series data
tsstore->insert("sensor_1", timestamp, value);

// Query with aggregation
auto result = tsstore->query(
  "sensor_1",
  start_time,
  end_time,
  Aggregation::AVG,
  interval_5min
);
```

---

### Kapitel 16: Geospatial

**Primäre Quellen:**
- `/docs/geo/geo_architecture.md` - Geo Architecture
- `/docs/features/features_geo.md` - Geo Features
- `/include/geo/spatial_index.hpp` - R*-Tree Index

**Sekundäre Quellen:**
- `/src/geo/rtree_index.cpp` - R*-Tree Implementation
- `/tests/test_geo.cpp` - Geo Tests

**Code-Beispiele:**
```aql
// Geospatial query
FOR location IN locations
  FILTER GEO_DISTANCE(location.coords, [52.52, 13.40]) < 5000
  RETURN location
```

---

### Kapitel 17: Hybrid Search

**Primäre Quellen:**
- `/docs/search/hybrid_search_design.md` - Hybrid Search Architecture
- `/include/search/hybrid_search.hpp` - Hybrid Search Interface

**Sekundäre Quellen:**
- `/src/search/ranking_fusion.cpp` - Score Fusion
- `/tests/test_hybrid_search.cpp` - Hybrid Search Tests

**Code-Beispiele:**
```cpp
// Hybrid search: Full-text + Vector + Graph
auto results = hybrid_search->search({
  fulltext: "machine learning",
  vector: embedding,
  graph: {start: "papers:123", depth: 2},
  weights: {fulltext: 0.3, vector: 0.5, graph: 0.2}
});
```

---

## TEIL V: ENTERPRISE-FEATURES

### Kapitel 18: Sharding und Horizontale Skalierung

**Primäre Quellen:**
- `/docs/sharding/sharding_overview.md` - Sharding-Übersicht
- `/docs/sharding/sharding_vcc_urn.md` - VCC-URN Hashing
- `/docs/reports/SHARDING_AUTO_REBALANCING.md` - Auto-Rebalancing

**Sekundäre Quellen:**
- `/include/sharding/shard_coordinator.hpp` - Shard Coordinator
- `/src/sharding/consistent_hash.cpp` - Consistent Hashing
- `/src/sharding/gossip_protocol.cpp` - P2P Gossip

**Code-Beispiele:**
```cpp
// Shard configuration
{
  "shards": [
    {"id": "shard_1", "host": "node1:8765", "vnodes": 128},
    {"id": "shard_2", "host": "node2:8765", "vnodes": 128},
    {"id": "shard_3", "host": "node3:8765", "vnodes": 128}
  ]
}

// Route request to shard
auto shard = coordinator->get_shard_for_key("users:123");
auto result = shard->execute(request);
```

---

### Kapitel 19: Replication

**Primäre Quellen:**
- `/docs/replication/README.md` - Replication-Übersicht
- `/docs/replication/replication_crdt.md` - CRDT-basierte Replikation
- `/include/replication/replication_manager.hpp` - Replication Manager

**Sekundäre Quellen:**
- `/src/replication/vector_clock.cpp` - Vector Clocks
- `/src/replication/hlc.cpp` - Hybrid Logical Clocks
- `/src/replication/crdt.cpp` - CRDT Implementation

**Code-Beispiele:**
```cpp
// Leader-Follower setup
replication_mgr->configure({
  mode: ReplicationMode::LEADER_FOLLOWER,
  leader: "node1:8765",
  followers: ["node2:8765", "node3:8765"],
  sync_mode: SyncMode::ASYNC
});

// CRDT merge
auto merged = crdt->merge(local_version, remote_version);
```

---

### Kapitel 20: GPU Acceleration

**Primäre Quellen:**
- `/docs/performance/performance_gpu.md` - GPU Performance
- `/docs/features/features_gpu.md` - GPU Features
- `/include/gpu/gpu_backend.hpp` - GPU Backend Interface

**Sekundäre Quellen:**
- `/src/gpu/cuda_backend.cpp` - CUDA Implementation
- `/src/gpu/vulkan_backend.cpp` - Vulkan Implementation
- `/benchmarks/benchmark_gpu.cpp` - GPU Benchmarks

**Code-Beispiele:**
```cpp
// Enable GPU acceleration
gpu_mgr->enable_backend(GPUBackend::CUDA);

// GPU-accelerated vector search
auto results = vector_index->search_gpu(
  query_vector,
  k=1000,
  batch_size=10000
);
```

---

### Kapitel 21: Analytics (CEP und OLAP)

**Primäre Quellen:**
- `/docs/analytics/analytics_cep.md` - Complex Event Processing
- `/docs/analytics/analytics_olap.md` - OLAP Analytics
- `/include/analytics/cep_engine.hpp` - CEP Engine

**Sekundäre Quellen:**
- `/src/analytics/epl_parser.cpp` - EPL Parser
- `/src/analytics/columnar_store.cpp` - Columnar Storage
- `/src/analytics/window_functions.cpp` - Window Functions

**Code-Beispiele:**
```sql
-- OLAP query with CUBE
SELECT country, city, product, SUM(revenue)
FROM sales
GROUP BY CUBE(country, city, product);

-- CEP pattern
SELECT *
FROM events
MATCH_RECOGNIZE (
  ORDER BY timestamp
  MEASURES A.id AS id
  PATTERN (A B+ C)
  DEFINE
    A AS A.type = 'start',
    B AS B.type = 'middle',
    C AS C.type = 'end'
);
```

---

### Kapitel 22: Multi-Tenancy und Rate Limiting

**Primäre Quellen:**
- `/docs/enterprise/README.md` - Enterprise Features
- `/docs/enterprise/enterprise_scalability.md` - Scalability Features
- `/include/enterprise/rate_limiter.hpp` - Rate Limiter

**Sekundäre Quellen:**
- `/src/enterprise/token_bucket.cpp` - Token Bucket
- `/src/enterprise/load_shedder.cpp` - Load Shedding
- `/tests/test_enterprise.cpp` - Enterprise Tests (20/20)

**Code-Beispiele:**
```cpp
// Rate limiting configuration
rate_limiter->configure({
  global_rate: 1000,  // req/sec
  per_client_rate: 100,
  burst_size: 200
});

// Multi-tenancy
tenant_mgr->create_tenant("acme_corp", {
  max_storage: "100GB",
  max_collections: 1000,
  max_requests_per_sec: 500
});
```

---

## TEIL VI: ECOSYSTEM UND ZUKUNFT

### Kapitel 23: Client SDKs

**Primäre Quellen:**
- `/docs/clients/README.md` - SDK-Übersicht
- `/clients/python/README.md` - Python SDK
- `/clients/javascript/README.md` - JavaScript SDK
- `/clients/rust/README.md` - Rust SDK

**Weitere SDKs:**
- `/clients/go/README.md` - Go SDK
- `/clients/java/README.md` - Java SDK
- `/clients/csharp/README.md` - C# SDK
- `/clients/swift/README.md` - Swift SDK

**Code-Beispiele (Python):**
```python
from themisdb import ThemisClient

# Connect
client = ThemisClient("http://localhost:8765")

# Insert
client.insert("users:123", {
    "name": "Alice",
    "age": 30
})

# Query with AQL
results = client.query("""
    FOR user IN users
    FILTER user.age > 25
    RETURN user
""")
```

---

### Kapitel 24: Admin Tools und Operations

**Primäre Quellen:**
- `/docs/admin_tools/README.md` - Admin Tools Overview
- `/docs/observability/README.md` - Monitoring & Observability
- `/docs/guides/guides_operations_runbook.md` - Operations Runbook

**Sekundäre Quellen:**
- `/tools/admin/README.md` - WPF Admin Tools (7 Tools)
- `/docs/observability/observability_tracing.md` - Distributed Tracing
- `/docs/deployment/backup_recovery.md` - Backup/Recovery

**Admin Tools:**
1. Entity Browser
2. Index Manager
3. Query Console
4. Performance Monitor
5. Shard Coordinator
6. Backup Manager
7. Security Auditor

---

### Kapitel 25: Zukunft und Roadmap

**Primäre Quellen:**
- `/docs/roadmap/roadmap_overview.md` - Roadmap
- `/docs/development/DEVELOPMENT_SUMMARY.md` - Development Status
- `/docs/reports/database_capabilities_roadmap.md` - Capabilities Roadmap

**Sekundäre Quellen:**
- `/CHANGELOG.md` - Release History
- `/docs/reports/competitive_gap_analysis.md` - Competitive Analysis

**Geplante Features:**
- SDK Publishing (NPM, PyPI, Maven, etc.)
- Penetration Testing
- Multi-DC Replication (Production)
- Kubernetes Operator
- ML Integration (GNN)

---

## Anhänge

### Anhang A: API-Referenz

**Primäre Quellen:**
- `/docs/api/api_reference.md` - REST API Complete Reference
- `/openapi/openapi.yaml` - OpenAPI 3.0 Specification
- `/docs/wire_protocol_v1.md` - Wire Protocol

**GraphQL:**
- `/docs/api/api_graphql.md` - GraphQL Schema & Queries

---

### Anhang B: Code-Metriken

**Primäre Quellen:**
- `/docs/development/SOURCE_CODE_AUDIT.md` - Source Code Audit
- `/docs/reports/BENCHMARK_AND_TEST_AUDIT.md` - Benchmarks & Tests

**Metriken:**
- 90,829 LOC (132 Headers, 124 Sources)
- 468/468 MVCC Tests passing
- 100% Test Coverage für Enterprise Features

---

### Anhang C: Compliance und Security

**Primäre Quellen:**
- `/docs/compliance/compliance_full_checklist.md` - Full Compliance Checklist
- `/docs/compliance/compliance_dashboard.md` - Compliance Dashboard
- `/docs/security/SECURITY_AUDIT_REPORT.md` - Security Audit Report
- `/SECURITY.md` - Security Policy

**Standards:**
- BSI C5
- ISO 27001
- DSGVO (GDPR)
- eIDAS
- SOC 2
- ISO 22301 (BCP/DRP)

---

### Anhang D: Glossar

**Primäre Quellen:**
- `/docs/glossary.md` - Technical Glossary

**Begriffe:**
- AQL (Aranado Query Language)
- CRDT (Conflict-Free Replicated Data Type)
- CEP (Complex Event Processing)
- HNSW (Hierarchical Navigable Small World)
- LSM (Log-Structured Merge)
- MVCC (Multi-Version Concurrency Control)
- VCC-URN (Virtual Cluster Coordinator - Uniform Resource Name)
- ... (über 100 Begriffe)

---

## Verwendungshinweise

### Für Autoren
1. **Kapitel-Vorlage**: Verwenden Sie die Referenzdokumente als Basis
2. **Code-Validierung**: Alle Code-Beispiele müssen kompilieren
3. **Aktualität**: Prüfen Sie Versionsnummern und Status
4. **Cross-References**: Verlinken Sie verwandte Kapitel

### Für Reviewer
1. **Technical Accuracy**: Verifizieren Sie technische Details
2. **Code Validation**: Testen Sie alle Code-Beispiele
3. **Link Validation**: Prüfen Sie alle Dokumenten-Links
4. **Consistency**: Achten Sie auf einheitliche Terminologie

### Für Leser
1. **Navigation**: Nutzen Sie die Lesepfade (siehe README.md)
2. **Deep Dive**: Folgen Sie den Referenzdokument-Links
3. **Hands-On**: Probieren Sie die Code-Beispiele aus
4. **Feedback**: Melden Sie Fehler oder Unklarheiten

---

**Version History:**
- 1.0.0 (Dezember 2025): Initiales Mapping für alle 25 Kapitel + Anhänge
