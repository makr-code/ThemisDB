# ThemisDB Sharding - Unified Documentation

**Version:** 3.0  
**Letzte Aktualisierung:** 2. Dezember 2025  
**Status:** Phase 1-4 Abgeschlossen ✅, Phase 5-6 In Progress 🔄

---

## Executive Summary

Dieses Dokument ist die **autoritative Quelle** für den aktuellen Stand der horizontalen Skalierung in ThemisDB. Es ersetzt und konsolidiert:

- `implementation_summary.md` (Phase 1 Zusammenfassung)
- `phase1_report.md` (Phase 1 Report)
- `phases_1-3_summary.md` (Phasen 1-3 Summary)
- `horizontal_scaling_strategy.md` (Strategiedokument)

---

## Implementierungsstand

### Übersicht

| Phase | Status | Komponenten | Tests |
|-------|--------|-------------|-------|
| Phase 1: Core Infrastructure | ✅ DONE | URN, ConsistentHash, Topology, Resolver | 30 |
| Phase 2: PKI Security Layer | ✅ DONE | PKI Certificate, mTLS, SignedRequest | 24 |
| Phase 3: Shard Communication | ✅ DONE | RemoteExecutor, ShardRouter | 10 |
| Phase 4: Data Migration | ✅ DONE | DataMigrator, AutoRebalancer, etcd, HealthCheck, CloudAgent, GossipProtocol, CrossShardJoin | 40+ |
| Phase 5: Testing | 🔄 IN PROGRESS | Integration, E2E, Chaos | WIP |
| Phase 6: Monitoring | ⚠️ PARTIAL | Prometheus Metrics | Grundstruktur |

**Gesamtfortschritt:** ~95% der Kern-Implementierung abgeschlossen

---

## Phase 1: Core Infrastructure ✅

### 1.1 URN (Uniform Resource Name)

**Dateien:** `include/sharding/urn.h`, `src/sharding/urn.cpp`

**URN-Format:**
```
urn:themis:{model}:{namespace}:{collection}:{uuid}
```

**Unterstützte Modelle:**
- `relational` - Relationale Tabellen
- `graph` - Property Graph
- `vector` - Embedding-Vektoren
- `timeseries` - Zeitserien
- `document` - JSON-Dokumente

**Beispiel:**
```cpp
auto urn = URN::parse("urn:themis:relational:customers:users:550e8400-e29b-41d4-a716-446655440000");
uint64_t hash = urn->hash();  // xxHash64 für Sharding
```

### 1.2 Consistent Hash Ring

**Dateien:** `include/sharding/consistent_hash.h`, `src/sharding/consistent_hash.cpp`

**Features:**
- 150 virtuelle Knoten pro Shard (konfigurierbar)
- O(log N) Lookup-Performance
- Balance-Faktor < 5% Standardabweichung
- Thread-safe Add/Remove

**Beispiel:**
```cpp
ConsistentHashRing ring;
ring.addShard("shard_001", 150);
ring.addShard("shard_002", 150);

std::string shard = ring.getShardForURN(urn);
auto replicas = ring.getSuccessors(hash, 2);  // 2 Replicas
```

### 1.3 Shard Topology Manager

**Dateien:** `include/sharding/shard_topology.h`, `src/sharding/shard_topology.cpp`

**Features:**
- Shard-Registry mit Health-Tracking
- etcd v3 HTTP API Integration (Phase 4)
- Capability-basierte Zugriffskontrolle
- PKI Certificate Serial Tracking

### 1.4 URN Resolver

**Dateien:** `include/sharding/urn_resolver.h`, `src/sharding/urn_resolver.cpp`

**Features:**
- URN → Primary Shard Resolution
- Replica Shard Discovery
- Locality Check (isLocal)

---

## Phase 2: PKI Security Layer ✅

### 2.1 PKI Shard Certificate

**Dateien:** `include/sharding/pki_shard_certificate.h`, `src/sharding/pki_shard_certificate.cpp`

**Features:**
- X.509 Certificate Parsing mit OpenSSL
- Custom Extensions: shardID, datacenter, tokenRange, capabilities
- CA Verification und CRL Checking

### 2.2 mTLS Client

**Dateien:** `include/sharding/mtls_client.h`, `src/sharding/mtls_client.cpp`

**Features:**
- Mutual TLS mit TLS 1.3 (Fallback TLS 1.2)
- SNI (Server Name Indication)
- Retry-Logik mit Exponential Backoff
- Connection Pooling (via HTTP Client Pool)

### 2.3 Signed Request Protocol

**Dateien:** `include/sharding/signed_request.h`, `src/sharding/signed_request.cpp`

**Features:**
- RSA-SHA256 Request Signing
- Replay Protection (Timestamp + Nonce)
- Defense-in-Depth (zusätzlich zu mTLS)

---

## Phase 3: Shard Communication ✅

### 3.1 Remote Executor

**Dateien:** `include/sharding/remote_executor.h`, `src/sharding/remote_executor.cpp`

**Features:**
- mTLS-basierte Shard-zu-Shard-Kommunikation
- Signed Envelope Option
- Query Execution Endpoint

### 3.2 Shard Router

**Dateien:** `include/sharding/shard_router.h`, `src/sharding/shard_router.cpp`

**Routing-Strategien:**
- `SINGLE_SHARD`: URN-basiert (GET/PUT/DELETE)
- `SCATTER_GATHER`: Parallel über alle Shards
- `NAMESPACE_LOCAL`: Namespace-spezifisch
- `CROSS_SHARD_JOIN`: Optimierte Joins

**Beispiel:**
```cpp
ShardRouter router(resolver, executor, config);

// Single-shard operation
auto data = router.get(*urn);
router.put(*urn, json_data);

// Scatter-gather query
auto results = router.executeQuery("FOR doc IN users RETURN doc");
```

---

## Phase 4: Data Migration & Advanced Features ✅

### 4.1 Data Migrator

**Dateien:** `include/sharding/data_migrator.h`, `src/sharding/data_migrator.cpp`

**Features:**
- `fetchBatch()`: mTLS-basierte Batch-Abfrage vom Quell-Shard
- `writeBatch()`: POST-Request mit Retry-Logik an Ziel-Shard
- Progress Tracking und Checkpointing

### 4.2 Auto Rebalancer

**Dateien:** `include/sharding/auto_rebalancer.h`, `src/sharding/auto_rebalancer.cpp`

**Features:**
- Multi-Criteria Load Detection (Storage, Request, Latency, Resource)
- RSA-SHA256 Operation Signing mit EVP API
- Safety Mechanisms: Cooldown, Concurrency Limits, Daily Limits
- OpenTelemetry Tracing Integration

### 4.3 etcd Integration

**Features (in `shard_topology.cpp`):**
- `loadFromMetadataStore()`: Range-Query für Shard-Discovery
- `saveToMetadataStore()`: Persistierung der Topologie
- Base64 En-/Decoding für etcd v3 API

### 4.4 Health Check System

**Dateien:** `include/sharding/health_check.h`, `src/sharding/health_check.cpp`

**Features:**
- `checkCertificateValidity()`: X.509 Zertifikatsablauf-Prüfung
- `checkStorageCapacity()`: HTTP-basierte Speicherabfrage
- `checkNetworkConnectivity()`: Latenz-Messung

### 4.5 Cloud Agent

**Dateien:** `include/sharding/cloud_agent.h`, `src/sharding/cloud_agent.cpp`

**Features:**
- Multi-DC-Aware Scatter-Gather
- Datacenter-Proximität-Sortierung
- Async Operation mit Progress Tracking
- Cloud Provider Interface (AWS, Azure, GCP)

### 4.6 Cross-Shard Join Optimierung

**Features (in `shard_router.cpp`):**
- **Broadcast Hash Join**: Für non-partition keys
- **Co-Located Join**: Für Partition-Key-basierte Joins
- Automatische Strategiewahl

### 4.7 P2P Gossip-Protokoll 🆕

**Dateien:** `include/sharding/gossip_protocol.h`, `src/sharding/gossip_protocol.cpp`

**Features:**
- SWIM-basiertes Gossip-Protokoll
- Periodischer Heartbeat (konfigurierbar)
- Fanout-basierte Peer-Selektion
- Version-Vectors für Anti-Entropy
- **Standardmäßig deaktiviert** (`enabled: false`)

**Konfiguration:**
```yaml
peer_discovery:
  enabled: false
  gossip_interval_sec: 30
  max_peers: 100
  seed_nodes:
    - "shard-001.dc1.example.com:8080"
    - "shard-002.dc2.example.com:8080"
```

**Sicherheit:**
- mTLS-authentifizierte Peers
- RSA-SHA256 Message Signing
- Rate-Limiting pro Peer
- Replay-Protection

---

## Phase 5: Testing 🔄

### 5.1 Bestehende Tests

| Test-Datei | Tests | Status |
|------------|-------|--------|
| `test_sharding_core.cpp` | 30 | ✅ PASS |
| `test_pki_shard_certificate.cpp` | 10+ | ✅ PASS |
| `test_shard_communication.cpp` | 10 | ✅ PASS |
| `test_cloud_agent.cpp` | 20+ | ✅ PASS |

### 5.2 Neue Tests

| Test-Datei | Typ | Status |
|------------|-----|--------|
| `test_sharding_integration.cpp` | Integration | 🆕 NEU |
| `test_sharding_e2e.cpp` | E2E | 🆕 NEU |
| `test_sharding_chaos.cpp` | Chaos | 🆕 NEU |

---

## Phase 6: Monitoring ⚠️

### 6.1 Prometheus Metrics (Grundstruktur)

```prometheus
# Shard Routing
themis_shard_requests_total{shard="shard_001", type="local"}
themis_shard_requests_total{shard="shard_001", type="remote"}
themis_shard_latency_seconds{shard="shard_001", quantile="0.99"}

# Rebalancing
themis_rebalance_operations_total{status="success"}
themis_rebalance_data_bytes_migrated

# Health
themis_shard_health{shard="shard_001"} 1
themis_certificate_expiry_days{shard="shard_001"}

# Gossip Protocol
themis_gossip_peers_total
themis_gossip_messages_total{type="heartbeat"}
```

---

## Architektur

```
┌────────────────────────────────────────────────────────────────────────┐
│                           Client Applications                           │
│                      (URN-based API Requests)                          │
└─────────────────────────────┬──────────────────────────────────────────┘
                              │
                              ▼
┌────────────────────────────────────────────────────────────────────────┐
│                           Shard Router                                  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐ │
│  │ Query Analysis  │  │ Routing Strategy│  │ Cross-Shard Join Opt.  │ │
│  │                 │  │ Selection       │  │ (Hash/CoLocated)       │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────────────┘ │
└──────────────┬────────────────────────────────────────┬────────────────┘
               │                                        │
               ▼                                        ▼
┌──────────────────────────┐              ┌──────────────────────────────┐
│      URN Resolver        │              │      Remote Executor         │
│  • Primary Resolution    │              │  • mTLS Transport            │
│  • Replica Discovery     │              │  • Signed Envelope           │
│  • Locality Check        │              │  • Retry Logic               │
└──────────────────────────┘              └──────────────────────────────┘
               │                                        │
               ▼                                        ▼
┌────────────────────────────────────────────────────────────────────────┐
│                    PKI-Secured Shard Mesh (Zero-Trust)                  │
│                                                                         │
│  ┌──────────┐ mTLS ┌──────────┐ mTLS ┌──────────┐ mTLS ┌──────────┐   │
│  │ Shard 1  │◄────►│ Shard 2  │◄────►│ Shard 3  │◄────►│ Shard N  │   │
│  │          │      │          │      │          │      │          │   │
│  │ RocksDB  │      │ RocksDB  │      │ RocksDB  │      │ RocksDB  │   │
│  │ + Gossip │      │ + Gossip │      │ + Gossip │      │ + Gossip │   │
│  └──────────┘      └──────────┘      └──────────┘      └──────────┘   │
│                                                                         │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                  P2P Gossip Protocol (Optional)                  │   │
│  │  • SWIM-based • Version Vectors • Fanout Propagation            │   │
│  └─────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────────┘
               │
               ▼
┌────────────────────────────────────────────────────────────────────────┐
│                          Metadata Store (etcd)                          │
│  • Shard Topology     • Health Status     • PKI CRL                    │
└────────────────────────────────────────────────────────────────────────┘
```

---

## Security Model

### Zero-Trust Architecture

```
Layer 1: mTLS (Transport Security)
  ↓ Certificate-based authentication
  ↓ Encrypted TLS 1.3 communication

Layer 2: Signed Requests (Application Security)
  ↓ RSA-SHA256 request signing
  ↓ Timestamp + Nonce replay prevention

Layer 3: Capability-Based Access (Authorization)
  ↓ read, write, replicate, admin
  ↓ Per-shard capability enforcement

Layer 4: PKI Identity
  ↓ X.509 certificates with custom extensions
  ↓ Certificate Revocation List (CRL)
```

### Threat Mitigations

| Bedrohung | Mitigation |
|-----------|------------|
| Kompromittierter Shard | Certificate Revocation (CRL) |
| Man-in-the-Middle | mTLS + Signed Requests |
| Unauthorized Rebalancing | Operator-Certificate Required |
| Replay Attack | Timestamp + Nonce |
| Certificate Theft | Encrypted Keys (HSM optional) |

---

## Verwendung

### Quick Start

```cpp
#include "sharding/shard_router.h"

// 1. Configure cluster
auto topology = std::make_shared<ShardTopology>(topology_config);
auto hash_ring = std::make_shared<ConsistentHashRing>();

// 2. Add shards
hash_ring->addShard("shard_001", 150);
topology->addShard(ShardInfo{
    .shard_id = "shard_001",
    .primary_endpoint = "themis-shard001.dc1:8080",
    .is_healthy = true
});

// 3. Create resolver and router
auto resolver = std::make_shared<URNResolver>(topology, hash_ring, "shard_001");
ShardRouter router(resolver, executor, router_config);

// 4. Execute operations
auto urn = URN::parse("urn:themis:relational:customers:users:550e8400-...");
auto data = router.get(*urn);
```

### Kubernetes Deployment

```bash
# Apply CRDs
kubectl apply -f deploy/kubernetes/crds/

# Deploy 3-node cluster
kubectl apply -f deploy/kubernetes/examples/themisdb-cluster.yaml
```

### P2P Gossip aktivieren

```yaml
# In themisdb-cluster.yaml spec.sharding:
sharding:
  enabled: true
  peerDiscovery:
    enabled: true
    gossipIntervalSec: 30
    maxPeers: 100
    seedNodes:
      - shard-001.themisdb.svc.cluster.local:8080
```

---

## API Reference

### URN

```cpp
class URN {
    static std::optional<URN> parse(std::string_view urn_string);
    std::string toString() const;
    uint64_t hash() const;
    std::string getResourceId() const;
    bool isValidUUID() const;
};
```

### ConsistentHashRing

```cpp
class ConsistentHashRing {
    void addShard(const std::string& shard_id, uint32_t virtual_nodes = 150);
    void removeShard(const std::string& shard_id);
    std::string getShardForHash(uint64_t hash) const;
    std::string getShardForURN(const URN& urn) const;
    std::vector<std::string> getSuccessors(uint64_t hash, size_t count) const;
    double getBalanceFactor() const;
};
```

### ShardRouter

```cpp
class ShardRouter {
    std::optional<nlohmann::json> get(const URN& urn);
    bool put(const URN& urn, const nlohmann::json& data);
    bool delete_(const URN& urn);
    std::vector<ShardResult> scatterGather(const std::string& query);
    std::vector<ShardResult> executeCrossShardJoin(...);
    nlohmann::json getStatistics() const;
};
```

### GossipProtocol

```cpp
class GossipProtocol {
    void start();
    void stop();
    void addPeer(const std::string& peer_endpoint);
    void removePeer(const std::string& peer_endpoint);
    std::vector<std::string> getKnownPeers() const;
    nlohmann::json getStatistics() const;
};
```

---

## Changelog

### v3.0 (2. Dezember 2025)
- ✅ P2P Gossip-Protokoll implementiert
- ✅ Cross-Shard Join Optimierung
- ✅ Cloud Agent Multi-DC
- ✅ etcd Integration
- ✅ Health Check System
- ✅ Kubernetes CRDs

### v2.0 (20. November 2025)
- ✅ Phase 1-3 abgeschlossen
- ✅ mTLS + Signed Requests
- ✅ Shard Router mit Scatter-Gather

### v1.0 (15. November 2025)
- ✅ Initial URN-Schema
- ✅ Consistent Hash Ring

---

## Referenzen

### Source Code
- `include/sharding/*.h` - Header Files
- `src/sharding/*.cpp` - Implementation
- `tests/test_sharding_*.cpp` - Tests

### Related Documentation
- [SCALING_TODO.md](../SCALING_TODO.md) - Vollständige Audit-Dokumentation
- [FEATURES.md](../FEATURES.md) - Feature-Übersicht
- [Kubernetes README](../../deploy/kubernetes/README.md) - K8s Deployment

---

**Autor:** GitHub Copilot  
**Review:** makr-code  
**Status:** Authoritative Documentation
