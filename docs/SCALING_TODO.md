# ThemisDB Skalierung - TODO-Liste

**Erstellt:** 1. Dezember 2025  
**Aktualisiert:** 1. Dezember 2025  
**Basierend auf:** VCC-URN Philosophie und Best-Practice Analyse  
**Autor:** Audit durch Code-Review

---

## Executive Summary

Diese TODO-Liste dokumentiert den aktuellen Stand der vertikalen und horizontalen Skalierung im ThemisDB-Projekt und identifiziert Diskrepanzen zwischen Dokumentation und tatsächlicher Implementierung.

**Gesamtstatus:**
- **Horizontale Skalierung:** ~95% implementiert (Phase 1-4 + P2P Gossip + Cross-Shard Joins)
- **Vertikale Skalierung:** ~85% implementiert (Enterprise Features)

---

## 1. Horizontale Skalierung (Sharding)

### 1.1 Implementierungsstand

| Phase | Komponente | Status | Dokumentation | Code-Realität |
|-------|------------|--------|---------------|---------------|
| Phase 1 | URN Parser | ✅ DONE | ✅ Korrekt | `include/sharding/urn.h` vollständig |
| Phase 1 | Consistent Hash Ring | ✅ DONE | ✅ Korrekt | `include/sharding/consistent_hash.h` vollständig |
| Phase 1 | Shard Topology | ✅ DONE | ✅ Korrekt | `include/sharding/shard_topology.h` vollständig |
| Phase 1 | URN Resolver | ✅ DONE | ✅ Korrekt | `include/sharding/urn_resolver.h` vollständig |
| Phase 2 | PKI Shard Certificate | ✅ DONE | ✅ Korrekt | `include/sharding/pki_shard_certificate.h` |
| Phase 2 | mTLS Client | ✅ DONE | ✅ Korrekt | `include/sharding/mtls_client.h` |
| Phase 2 | Signed Request Protocol | ✅ DONE | ✅ Korrekt | `include/sharding/signed_request.h` |
| Phase 3 | Remote Executor | ✅ DONE | ✅ Korrekt | `src/sharding/remote_executor.cpp` |
| Phase 3 | Shard Router | ✅ DONE | ✅ Korrekt | `src/sharding/shard_router.cpp` |
| Phase 4 | Data Migrator | ✅ DONE | ✅ Implementiert | Echte mTLS-Integration mit fetchBatch/writeBatch |
| Phase 4 | Auto Rebalancer | ✅ DONE | ✅ Implementiert | RSA-SHA256 Signierung mit PKI |
| Phase 4 | Shard Topology (etcd) | ✅ DONE | ✅ Implementiert | etcd v3 HTTP API Integration |
| Phase 4 | Health Check System | ✅ DONE | ✅ Implementiert | Echte HTTP Health Checks + Cert Parsing |
| Phase 5 | Integration Tests | ❌ MISSING | 🔴 FEHLT | Keine echten E2E-Tests |
| Phase 6 | Prometheus Metrics | ⚠️ PARTIAL | 🔴 ÜBERTRIEBEN | Grundstruktur, aber nicht integriert |

### 1.2 TODO: Horizontale Skalierung

#### Hohe Priorität (P0)

- [x] **TODO-HS-001:** Data Migrator vollständig implementieren ✅ ERLEDIGT
  - **Implementiert:** Echte mTLS-basierte `fetchBatch()` und `writeBatch()` Funktionen
  - **Datei:** `src/sharding/data_migrator.cpp`
  - **Commit:** Implementiert mit MTLSClient-Integration

- [x] **TODO-HS-002:** Auto Rebalancer produktionsreif machen ✅ ERLEDIGT
  - **Implementiert:** RSA-SHA256 Signierung mit EVP API
  - **Datei:** `src/sharding/auto_rebalancer.cpp`
  - **Commit:** Implementiert mit echter PKI-Signierung

- [x] **TODO-HS-003:** Shard Router Local Execution implementieren ✅ ERLEDIGT
  - **Implementiert:** Vollständige Pfad-basierte Request-Verarbeitung
  - **Datei:** `src/sharding/shard_router.cpp`
  - **Commit:** Implementiert mit URN-Parsing und Response-Generierung

#### Mittlere Priorität (P1)

- [x] **TODO-HS-004:** etcd/Consul Integration für Metadata Store ✅ ERLEDIGT
  - **Implementiert:** Echte etcd v3 HTTP API Integration
  - **Datei:** `src/sharding/shard_topology.cpp`
  - **Details:** 
    - `loadFromMetadataStore()` lädt Shard-Konfiguration von etcd mit Base64 En-/Decoding
    - `saveToMetadataStore()` persistiert Topologie zu etcd
    - Key-Struktur: `/themis/{cluster_name}/shards/{shard_id}`

- [x] **TODO-HS-005:** Health Check Integration ✅ ERLEDIGT
  - **Implementiert:** Echte HTTP-basierte Health Checks
  - **Datei:** `src/sharding/health_check.cpp`
  - **Details:**
    - `checkCertificateValidity()` parst ASN1_TIME für echte Ablaufprüfung
    - `checkStorageCapacity()` ruft `/api/v1/metrics/storage` Endpoint ab
    - `checkNetworkConnectivity()` misst echte HTTP-Latenz zu Shards

- [x] **TODO-HS-006:** Cloud Agent für Multi-DC Deployment ✅ ERLEDIGT
  - **Implementiert:** Parallele Scatter-Gather mit Datacenter-Awareness
  - **Datei:** `src/sharding/cloud_agent.cpp`
  - **Details:**
    - `executeScatterGather()` nutzt `std::async` für parallele Ausführung
    - Shards werden nach Datacenter-Proximität sortiert (lokaler DC zuerst)
    - Thread-sichere Ergebnisaggregation mit Mutex
    - Timeout-Handling für jede Shard-Anfrage

#### Niedrige Priorität (P2)

- [x] **TODO-HS-007:** Scatter-Gather Parallelisierung ✅ ERLEDIGT
  - **Implementiert:** Parallele Query-Execution mit std::async
  - **Datei:** `src/sharding/shard_router.cpp`
  - **Details:**
    - `scatterGather()` nutzt `std::future` für parallele Shard-Anfragen
    - Konfigurierbares Timeout (`scatter_timeout_ms`)
    - Thread-sichere Counter für lokale/remote Requests
    - Fehlerbehandlung für Timeouts und Exceptions

- [x] **TODO-HS-008:** Cross-Shard Join Optimierung ✅ ERLEDIGT
  - **Implementiert:** Broadcast Hash Join und Co-Located Join Strategien
  - **Datei:** `src/sharding/shard_router.cpp`
  - **Details:**
    - Automatische Strategiewahl basierend auf join_field
    - Broadcast Hash Join für non-partition keys
    - Co-Located Join wenn join_field = Partition Key
    - Hash-Table-Aufbau für effiziente Lookups
    - OpenTelemetry Tracing Integration

- [x] **TODO-HS-009:** Peer-to-Peer Gossip-Protokoll ✅ ERLEDIGT 🆕
  - **Implementiert:** SWIM-basiertes Gossip-Protokoll für Peer-Discovery
  - **Dateien:** 
    - `include/sharding/gossip_protocol.h`
    - `src/sharding/gossip_protocol.cpp`
  - **Features:**
    - `GossipProtocol` Klasse mit periodischem Heartbeat
    - Peer-Liste austauschen (JSON-serialisiert)
    - Anti-Entropy mit Version-Vectors
    - Standardmäßig deaktiviert, konfigurierbar (`enabled: false`)
    - Fanout-basierte Peer-Selektion
  - **Konfiguration:**
    ```yaml
    peer_discovery:
      enabled: false
      gossip_interval_sec: 30
      max_peers: 100
      seed_nodes: []
    ```
  - **Sicherheit:**
    - mTLS-authentifizierte Peers
    - RSA-SHA256 Message Signing
    - Rate-Limiting pro Peer
    - Replay-Protection (max_message_age_sec)

---

## 2. Vertikale Skalierung (Enterprise Features)

### 2.1 Implementierungsstand

| Feature | Status | Dokumentation | Code-Realität |
|---------|--------|---------------|---------------|
| Token Bucket Rate Limiter | ✅ DONE | ✅ Korrekt | `src/server/rate_limiter_v2.cpp` |
| Per-Client Rate Limiter | ✅ DONE | ✅ Korrekt | `src/server/rate_limiter_v2.cpp` |
| Load Shedder | ✅ DONE | ✅ Korrekt | `src/server/load_shedder.cpp` |
| HTTP Client Pool | ✅ DONE | ✅ Korrekt | `src/utils/http_client_pool.cpp` |
| Batch CRUD | ✅ DONE | ✅ Korrekt | `src/server/http_server.cpp` |
| TLS 1.3 Hardening | ✅ DONE | ✅ Korrekt | Implementiert |
| RBAC | ✅ DONE | ✅ Korrekt | `src/security/rbac.cpp` |
| Field-Level Encryption | ✅ DONE | ✅ Korrekt | `src/security/field_encryption.cpp` |
| Audit Logging | ✅ DONE | ✅ Korrekt | `src/security/audit_logger.cpp` |
| GPU Acceleration | ✅ DONE (optional) | ⚠️ KORREKTUR | Code vorhanden unter `#ifdef THEMIS_ENABLE_CUDA` |

### 2.2 TODO: Vertikale Skalierung

#### Hohe Priorität (P0)

- [ ] **TODO-VS-001:** GPU Acceleration Dokumentation präzisieren
  - **Problem:** Dokumentation suggeriert "Production-Ready" ohne klaren Hinweis auf opt-in Kompilierung
  - **Datei:** `src/acceleration/cuda_backend.cpp`, `src/acceleration/faiss_gpu_backend.cpp`
  - **VCC-URN Prinzip:** Klare Build-Anleitung für GPU-Features
  - **Aufwand:** 1-2 Tage
  - **Details:**
    - Code ist vorhanden und implementiert (Faiss GPU, CUDA, Vulkan)
    - Nur bei `-DTHEMIS_ENABLE_CUDA=ON` oder `-DTHEMIS_ENABLE_GPU=ON` kompiliert
    - Dokumentation sollte Build-Optionen klar kommunizieren

#### Mittlere Priorität (P1)

- [ ] **TODO-VS-002:** Kubernetes Operator
  - **Problem:** In Roadmap erwähnt aber nicht vorhanden
  - **VCC-URN Prinzip:** Cloud-native Deployment
  - **Aufwand:** 4-6 Wochen

- [ ] **TODO-VS-003:** Multi-Tenancy Quota-Enforcement
  - **Problem:** Quotas definiert aber nicht enforced
  - **VCC-URN Prinzip:** Resource Isolation
  - **Aufwand:** 1-2 Wochen

---

## 3. Dokumentations-Diskrepanzen

### 3.1 Dokumentation > Code (Übertrieben)

| Dokument | Behauptung | Realität |
|----------|------------|----------|
| `docs/FEATURES.md` Zeile 871 | "Distributed Sharding (Phase 1-3) ✅" | Phase 4-6 fehlen komplett |
| `docs/FEATURES.md` Zeile 488-500 | "GPU Acceleration ✅ Production-Ready" | Code vorhanden, aber opt-in Build, keine klare Dokumentation |
| `docs/sharding/implementation_summary.md` | "Data Migration Tool vollständig" | Nur Placeholder-Code |
| `README.md` Abschnitt "Recent changes" | Impliziert vollständige Features | Viele Features unvollständig |

### 3.2 TODO: Dokumentation korrigieren

- [ ] **TODO-DOC-001:** FEATURES.md aktualisieren
  - Status-Indikatoren korrigieren
  - GPU Acceleration Status von ✅ auf "✅ (optional, build flag required)" präzisieren
  - Sharding-Status differenzierter darstellen

- [ ] **TODO-DOC-002:** README.md "Recent changes" präzisieren
  - Klare Unterscheidung zwischen "implementiert" und "geplant"
  - Status-Badges für unvollständige Features

- [ ] **TODO-DOC-003:** Sharding-Dokumentation vereinheitlichen
  - `phases_1-3_summary.md` und `implementation_summary.md` sind redundant
  - Eine autoritative Quelle für Sharding-Status

---

## 4. VCC-URN Best-Practice Empfehlungen

### 4.1 Architektur-Prinzipien

| Prinzip | Aktueller Stand | Empfehlung |
|---------|-----------------|------------|
| **Location Transparency** | ✅ URN-Schema implementiert | Beibehalten |
| **Zero-Trust Security** | ✅ mTLS + Signed Requests | Beibehalten |
| **Consistent Hashing** | ✅ Virtual Nodes (150/Shard) | Beibehalten |
| **PKI-basierte Identität** | ⚠️ Grundstruktur | Operator-Zertifikate vollständig integrieren |
| **Automatisches Rebalancing** | ⚠️ Placeholder | Vollständige Implementierung |
| **Scatter-Gather Queries** | ⚠️ Sequentiell | Parallelisierung |

### 4.2 Priorisierte Implementierungs-Reihenfolge (VCC-URN konform)

1. **Woche 1-3:** Data Migrator vollständig implementieren (TODO-HS-001)
2. **Woche 4-5:** Auto Rebalancer PKI-Signierung (TODO-HS-002)
3. **Woche 6-7:** Local Execution Path (TODO-HS-003)
4. **Woche 8-10:** etcd Integration (TODO-HS-004)
5. **Woche 11-12:** Integration Tests + Dokumentation korrigieren

---

## 5. Test-Abdeckung

### 5.1 Aktueller Stand

| Test-Datei | Tests | Coverage |
|------------|-------|----------|
| `test_sharding_core.cpp` | 30 | Phase 1 komplett |
| `test_pki_shard_certificate.cpp` | ~10 | Phase 2 PKI |
| `test_shard_communication.cpp` | ~10 | Phase 3 Kommunikation |
| Integration Tests | 0 | ❌ FEHLT |
| E2E Tests | 0 | ❌ FEHLT |

### 5.2 TODO: Tests

- [ ] **TODO-TEST-001:** Integration Tests für Sharding
  - Multi-Node Setup mit Docker Compose
  - Rebalancing unter Last testen
  - Failure-Szenarien

- [ ] **TODO-TEST-002:** Performance Benchmarks
  - Scatter-Gather Latenz
  - Rebalancing-Durchsatz
  - Cross-Shard Join Performance

- [ ] **TODO-TEST-003:** Chaos Testing
  - Shard-Ausfall Simulation
  - Netzwerk-Partitionen
  - Certificate Revocation

---

## 6. Zusammenfassung

### Positiv (Was gut ist)

1. ✅ URN-basiertes Sharding-Design ist solide und VCC-URN konform
2. ✅ PKI-Security Layer (Phase 2) ist vollständig implementiert
3. ✅ Vertikale Skalierung (Rate Limiting, Load Shedding) ist produktionsreif
4. ✅ ACID-Transaktionen und MVCC funktionieren

### Kritisch (Was fehlt)

1. ✅ Data Migration vollständig implementiert (mTLS-Integration)
2. ✅ Auto Rebalancer Signing vollständig implementiert (RSA-SHA256)
3. ✅ Shard Router Local Execution vollständig implementiert
4. ❌ Keine Integration Tests für Sharding
5. ⚠️ GPU Acceleration Code vorhanden, aber Build-Dokumentation fehlt

### Empfehlung

Die Dokumentation sollte **ehrlicher** den aktuellen Stand widerspiegeln. Der Code-Kern ist solide, aber die Dokumentation erweckt den Eindruck einer vollständigeren Implementierung als tatsächlich vorhanden ist.

**Geschätzter Aufwand für Vollständigkeit:**
- Horizontale Skalierung vollständig: 8-12 Wochen
- Dokumentation korrigieren: 1-2 Tage
- Test-Abdeckung erhöhen: 2-4 Wochen

---

## Anhang: Implementierte Funktionen

Die folgenden Placeholder-Funktionen wurden mit echtem Code implementiert:

### 1. `src/sharding/data_migrator.cpp`
- **fetchBatch()**: Verwendet MTLSClient für sichere Shard-zu-Shard-Kommunikation
- **writeBatch()**: POST-Request mit Retry-Logik und Fehlerbehandlung

### 2. `src/sharding/auto_rebalancer.cpp`  
- **signOperation()**: RSA-SHA256 Signierung mit OpenSSL EVP API
  - Lädt Private Key aus PEM-Datei
  - Erstellt kanonische Nachricht mit Timestamp
  - Signiert mit PKCS#1 v1.5 Padding
  - Kodiert Signatur als Base64

### 3. `src/sharding/shard_router.cpp`
- **executeLocal()**: Vollständige Request-Verarbeitung
  - Parst HTTP-Methode und Pfad
  - Unterstützt GET, PUT, POST, DELETE
  - Verarbeitet URN-basierte Entity-Operationen
  - Unterstützt Migration-Endpoints
  - Misst Ausführungszeit
- **scatterGather()**: Parallele Query-Execution ✅ NEU
  - Nutzt `std::async` und `std::future` für Parallelisierung
  - Konfigurierbares Timeout pro Shard
  - Thread-sichere Counter für Statistiken
  - Fehlerbehandlung für Timeouts und Exceptions
- **executeCrossShardJoin()**: Optimierte Cross-Shard Joins ✅ NEU
  - Broadcast Hash Join für non-partition keys
  - Co-Located Join für Partition-Key-basierte Joins
  - Hash-Table-Aufbau für effiziente Lookups
  - OpenTelemetry Tracing Integration

### 4. `src/sharding/shard_topology.cpp`
- **loadFromMetadataStore()**: etcd v3 HTTP API Integration
  - Lädt Shard-Konfiguration mit Range-Query
  - Base64 En-/Decoding für etcd Keys/Values
  - JSON-Parsing der Shard-Informationen
- **saveToMetadataStore()**: Persistiert Topologie zu etcd
  - Iteriert über alle Shards
  - Schreibt JSON-serialisierte ShardInfo

### 5. `src/sharding/health_check.cpp`
- **checkCertificateValidity()**: Echte X.509 Zertifikatsprüfung
  - Parst ASN1_TIME (UTC/GeneralizedTime)
  - Berechnet echte Sekunden bis Ablauf
- **checkStorageCapacity()**: HTTP-basierte Speicherabfrage
  - Ruft `/api/v1/metrics/storage` Endpoint ab
  - Parst JSON-Antwort für Speichernutzung
- **checkNetworkConnectivity()**: Latenz-Messung
  - HTTP GET auf `/health` Endpoint
  - Misst Round-Trip-Zeit

### 6. `src/sharding/cloud_agent.cpp` ✅ NEU
- **executeScatterGather()**: Parallele Multi-DC-Aware Scatter-Gather
  - Nutzt `std::async` für parallele Shard-Anfragen
  - Sortiert Shards nach Datacenter-Proximität
  - Thread-sichere Ergebnisaggregation
  - Timeout-Handling mit konfigurierbarer Dauer
  - Fügt DC/Region-Metadaten zu Ergebnissen hinzu

### 7. `src/sharding/gossip_protocol.cpp` ✅ NEU
- **GossipProtocol-Klasse**: SWIM-basiertes P2P Gossip-Protokoll
  - Periodischer Heartbeat mit konfigurierbarem Intervall
  - Fanout-basierte Peer-Selektion
  - Peer-Liste austauschen (JSON-serialisiert)
  - Version-Vectors für Anti-Entropy
- **handleMessage()**: Verarbeitet eingehende Gossip-Nachrichten
  - Heartbeat, Peer-Liste, Leave-Messages
  - Replay-Protection durch Timestamp-Validierung
  - Rate-Limiting pro Peer
- **Sicherheit**:
  - RSA-SHA256 Message Signing
  - mTLS-basierte Peer-Kommunikation
  - Certificate-Chain-Validierung

---

**Letzte Aktualisierung:** 2. Dezember 2025  
**Review-Status:** Awaiting Review
