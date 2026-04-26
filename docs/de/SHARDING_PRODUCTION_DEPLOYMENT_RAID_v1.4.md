# RAID-Themis Sharding - Produktions-Deployment-Anleitung

**Version:** 1.4 (RAID-Angepasst)  
**Stand:** 6. April 2026  
**Status:** ✅ Für internes RAID-Themis System optimiert  
**Kategorie:** 🔀 Sharding | 🛡️ RAID-Redundanz | 🚀 Production

---

## Executive Summary

Diese Anleitung beschreibt die **schrittweise Implementierung eines RAID-Themis Sharding-Clusters** in der Produktion. Basierend auf den internen Spezifikationen aus `docs/de/sharding/` wird ein **URN-basiertes, PKI-gesichertes Shard-Mesh** mit konfigurierbaren RAID-ähnlichen Redundanzmodi aufgebaut.

### Besonderheiten des RAID-Themis Systems

- ✅ **6 Redundanzmodi:** NONE, MIRROR, STRIPE, STRIPE_MIRROR, PARITY, GEO_MIRROR
- ✅ **PKI-Sicherheit:** Mutual TLS zwischen Shards, X.509 Shard-Identität
- ✅ **URN-Sharding:** Konsistentes Hashing mit xxHash64
- ✅ **Raft Consensus:** Strong Consistency, automatisches Failover
- ✅ **WAL Replication:** Zero Data Loss Guarantee
- ✅ **6 Phasen Implementation:** Alle abgeschlossen (Phase 1-6 + P0/P1.1/P1.2)

---

## 📋 Inhaltsverzeichnis

1. [Architektur-Übersicht](#1-architektur-übersicht)
2. [Pre-Deployment Checklist](#2-pre-deployment-checklist)
3. [Redundanzmodus-Auswahl](#3-redundanzmodus-auswahl)
4. [Infrastructure Setup](#4-infrastructure-setup)
5. [Shard-Konfiguration](#5-shard-konfiguration)
6. [PKI & TLS Setup](#6-pki--tls-setup)
7. [Shard Initialization](#7-shard-initialization)
8. [Verification & Testing](#8-verification--testing)
9. [Production Cutover](#9-production-cutover)
10. [Post-Deployment Operations](#10-post-deployment-operations)
11. [Troubleshooting](#11-troubleshooting)
12. [Rollback Procedures](#12-rollback-procedures)

---

## 1. Architektur-Übersicht

### 1.1 RAID-Themis Cluster-Topologie

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        VCC-PKI Trust Root                                │
│                  (themis-root-ca.crt, themis-cluster-ca.crt)             │
└─────────────────────────────────────────────────────────────────────────┘
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│              PKI-Secured Routing & Load Balancing Layer                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                   │
│  │ URN Resolver │  │ Shard Router │  │ mTLS Gateway │                   │
│  │(Hash-Ring)   │  │(Consistent   │  │(Cert Verify) │                   │
│  │              │  │ Hash Lookup) │  │              │                   │
│  └──────────────┘  └──────────────┘  └──────────────┘                   │
│  - URN-Format: urn:themis:{model}:{ns}:{collection}:{uuid}             │
│  - xxHash64 für Consistent-Hash-Lookup                                  │
│  - mTLS für alle Inter-Shard-Kommunikation                              │
└─────────────────────────────────────────────────────────────────────────┘
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                    Shard Mesh (Redundanzmodus-abhängig)                  │
│                                                                          │
│  ┌──────────┐ mTLS  ┌──────────┐ mTLS  ┌──────────┐ mTLS  ┌──────────┐ │
│  │Shard001  │◄─────►│Shard002  │◄─────►│Shard003  │◄─────►│Shard00N  │ │
│  │Primary   │       │Replica   │       │Primary   │       │Replica   │ │
│  │RocksDB   │       │RocksDB   │       │RocksDB   │       │RocksDB   │ │
│  │+ PKI     │       │+ PKI     │       │+ PKI     │       │+ PKI     │ │
│  └────┬─────┘       └────┬─────┘       └────┬─────┘       └────┬─────┘ │
│       │                  │                  │                  │        │
│  ┌────▼──────┐      ┌────▼──────┐      ┌────▼──────┐      ┌────▼──────┐│
│  │Replica1   │      │Replica1   │      │Replica1   │      │Replica1   ││
│  │RocksDB    │      │RocksDB    │      │RocksDB    │      │RocksDB    ││
│  │WAL Sync   │      │WAL Sync   │      │WAL Sync   │      │WAL Sync   ││
│  └───────────┘      └───────────┘      └───────────┘      └───────────┘│
│                                                                          │
│  Redundanzmodus: [MIRROR | STRIPE | STRIPE_MIRROR | PARITY | GEO_...]  │
└─────────────────────────────────────────────────────────────────────────┘
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│           PKI-Secured Metadata Store (etcd oder RocksDB)                 │
│  - Shard Topology mit Certificates                                      │
│  - Rebalancing Operations (signiert)                                    │
│  - Certificate Revocation List (CRL)                                    │
│  - Raft State Machine für Membership Changes                            │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Datenfluss URN → Shard

```
Client Request
    │
    ▼
URN-Format: urn:themis:relational:customers:users:550e8400-...
    │
    ├─► Hash(URN) via xxHash64
    │
    ├─► Consistent Hash Ring Lookup
    │   - 150 virtuelle Knoten pro Shard (konfigurierbar)
    │   - O(log N) Lookup-Performance
    │
    ├─► Shard Determination (z.B. "Shard002")
    │
    ├─► mTLS Connection zu Primary + Replicas (je nach Modus)
    │   - Client TLS: CA validiert Shard-Certificate
    │   - Shard TLS: Validiert Client-Certificate
    │
    ├─► Redundanzmodus-abhängig:
    │   - NONE: Primary nur
    │   - MIRROR: Primary → Replicas (RF=2-3)
    │   - STRIPE: Daten-Chunks über mehrere Shards
    │   - STRIPE_MIRROR: Striped Replikation
    │   - PARITY: Reed-Solomon EC über N+k Shards
    │   - GEO_MIRROR: Multi-DC Replikation
    │
    ▼
Data stored in RocksDB with WAL Replication
```

### 1.3 Redundanzmodi im Detail

| Modus | RF | Redundanz | Storage | Performance | Ausfalltoleranz |
|-------|----|-----------|---------|-------------|-----------------|
| **NONE** | 1 | 0 | 100% | Baseline | 0 Shards |
| **MIRROR** | 2-3 | N | 100/N% | Read: N×, Write: Baseline | N-1 Shards |
| **STRIPE** | 1 | 0 | 100% | N× besser | 0 Shards (Datenverlust) |
| **STRIPE_MIRROR** | 2 | 2 | 50% | Sehr gut | 1 Shard/Stripe |
| **PARITY** | 4+2 | k | 67% (4+2) | Gut | k Shards (EC) |
| **GEO_MIRROR** | 3 | 3 | 33% | Lokal gut | 2 DCs |

---

## 2. Pre-Deployment Checklist

### 2.1 Planungsphase (2-3 Wochen vor Deployment)

- [ ] **Redundanzmodus auswählen**
  - [ ] Use Case analysieren (Performance vs. Redundanz)
  - [ ] RTO/RPO Anforderungen definieren
  - [ ] Speicher-Budget festlegen
  - [ ] Entscheidung dokumentieren

- [ ] **Shard-Anzahl & Topologie**
  - [ ] Cluster-Größe: 4, 8, 16, 32 Shards?
  - [ ] Datengröße pro Shard: 100GB, 500GB, 1TB?
  - [ ] Multi-Region? Single DC?
  - [ ] Replication Factor (2 oder 3)?

- [ ] **Stakeholder-Alignment**
  - [ ] Engineering Team Review
  - [ ] Operations Team Review
  - [ ] Security/PKI Team Review
  - [ ] Go/No-Go Decision dokumentieren

### 2.2 Infrastruktur-Checklist

- [ ] **Hardware-Kapazität**
  - [ ] CPU-Kerne: min. 16 (empfohlen: 32+)
  - [ ] RAM: min. 64GB (empfohlen: 256GB+ für RF=3)
  - [ ] Storage: SSD mit min. 500GB/Shard (10TB für 8 Shards)
  - [ ] Network: 10GbE min., Latency RTT < 2ms (same DC)

- [ ] **Netzwerk-Topologie**
  - [ ] Firewall-Regeln für Shard-Ports (default: 8080-8087)
  - [ ] MTU-Größe: 1500 Bytes (oder Jumbo Frames 9000)
  - [ ] Network Monitoring aktiviert
  - [ ] Load Balancer konfiguriert (für mTLS)

- [ ] **Storage Setup**
  - [ ] LVM/RAID-Hardware bereit (falls lokal RAID)
  - [ ] RocksDB Directory vorbereitet (/var/lib/themis/shards/*)
  - [ ] WAL Directory vorbereitet (/var/lib/themis/wal/*)
  - [ ] Backup-Storage bereit (1.5× Cluster-Größe)

- [ ] **Monitoring & Logging**
  - [ ] Prometheus Server deployed
  - [ ] Grafana Dashboards vorbereitet
  - [ ] AlertManager konfiguriert
  - [ ] ELK Stack bereit (optional, für Log Aggregation)

### 2.3 Sicherheits-Checklist (PKI)

- [ ] **CA-Zertifikate erstellen**
  - [ ] Root CA generiert (themis-root-ca.crt)
  - [ ] Intermediate CA generiert (themis-cluster-ca.crt)
  - [ ] Private Keys gesichert (HSM oder encrypted storage)

- [ ] **Shard-Zertifikate**
  - [ ] CSR für jeden Shard generiert
  - [ ] Shard-Certificates von Cluster-CA signiert
  - [ ] URN-Extension in Certificates enthalten
  - [ ] Certificate Validity: min. 1 Jahr

- [ ] **Client-Zertifikate**
  - [ ] Proxy/Router Client-Certificate
  - [ ] Monitoring Client-Certificate
  - [ ] Operations/Admin Client-Certificate

- [ ] **CRL & OCSP**
  - [ ] Certificate Revocation List vorbereitet
  - [ ] OCSP Responder konfiguriert
  - [ ] Revocation Testing durchgeführt

---

## 3. Redundanzmodus-Auswahl

### 3.1 Entscheidungsmatrix

```yaml
# MIRROR Mode - Für kritische Produktionsysssteme
use_case_mirror: |
  - High Availability erforderlich (99.99% Uptime)
  - Datenverlust nicht akzeptabel
  - Read-Skalierung wichtig
  - Speicher-Kosten nicht kritisch

config_mirror:
  redundancy_mode: MIRROR
  replication_factor: 3          # Production Standard
  read_preference: NEAREST       # Lokale Reads für Latenz
  write_concern: MAJORITY        # Quorum Writes (2/3)
  
expected_performance:
  read_throughput: "3× vs. NONE"  # 3 Replicas für Reads
  write_throughput: "0.8× vs. NONE" # Quorum Latency
  storage_overhead: "200%"        # 3× Kopien

---

# STRIPE_MIRROR Mode - Für balanced Performance/Redundancy
use_case_stripe_mirror: |
  - High Throughput + Redundancy erforderlich
  - Balanced Trade-off wichtig
  - RAID-10-ähnliche Anforderungen
  - Large Document Support

config_stripe_mirror:
  redundancy_mode: STRIPE_MIRROR
  stripe_size: 64KB              # Chunk-Größe
  replication_factor: 2           # Mirror + Stripe
  min_stripe_shards: 4           # Min. 4 Shards für Striping
  stripe_large_docs: true
  large_doc_threshold: 1MB

expected_performance:
  read_throughput: "2-3×"        # Parallel reads + replicas
  write_throughput: "2×"         # Parallel writes
  storage_overhead: "100%"       # 2 Kopien

---

# PARITY Mode - Für große Datenmengen mit Kostenoptimierung
use_case_parity: |
  - Große Datenvolumen (TB-TB+)
  - Kostenoptimierung wichtig
  - Multi-Shard Ausfälle tolerieren (bis zu k)
  - Schreib-Performance nicht kritisch

config_parity:
  redundancy_mode: PARITY
  erasure_coding:
    algorithm: REED_SOLOMON     # oder CAUCHY, LRC
    data_shards: 4              # k = Daten-Chunks
    parity_shards: 2            # m = Parity-Chunks
  min_doc_size: 1MB            # Nur für große Dokumente
  
expected_performance:
  read_throughput: "Gut (Parallel over k+m Shards)"
  write_throughput: "Langsamer (EC-Overhead)"
  storage_overhead: "50%"       # 4+2 = 6, also 4/6 = 67% effizienz

---

# GEO_MIRROR Mode - Für Multi-DC/Multi-Region
use_case_geo_mirror: |
  - Multi-Region Deployment
  - Disaster Recovery erforderlich
  - Lokale Reads mit Remote Replication
  - Geo-redundancy erforderlich

config_geo_mirror:
  redundancy_mode: GEO_MIRROR
  replication_factor: 3
  datacenters:
    - name: "DC1"
      shards: 4
      location: "us-east-1"
    - name: "DC2"
      shards: 4
      location: "eu-west-1"
  sync_replication: ASYNC       # Remote DC Replikation
  conflict_resolution: LWW      # Last-Write-Wins

expected_performance:
  local_read_latency: "<5ms"
  remote_read_latency: "100-200ms"
  failover_time: "<1min"
```

### 3.2 Empfehlungen nach Use Case

**Für Enterprise Production:**
```yaml
recommended_config:
  redundancy_mode: STRIPE_MIRROR
  replication_factor: 2
  shards: 8                      # Start mit 8, scale zu 16/32 später
  stripe_size: 64KB
  read_preference: NEAREST
  write_concern: MAJORITY
  
rationale:
  - Best-of-Both-Worlds: Throughput + Redundancy
  - Toleriert 1 Shard-Ausfall (pro Stripe-Gruppe)
  - 2× Storage Overhead (akzeptabel)
  - RAID-10-ähnlich (etabliertes Muster)
```

**Für High-Performance Analytics:**
```yaml
recommended_config:
  redundancy_mode: STRIPE
  replication_factor: 1
  shards: 16
  stripe_size: 64KB
  backup_strategy: ASYNC_SNAPSHOT  # Backup statt RAID-Redundanz
  
rationale:
  - Maximale Performance
  - Redundanz via Backups
  - Bessere für RTO > 1 Stunde akzeptabel
```

**Für Large-Scale Data Warehouses:**
```yaml
recommended_config:
  redundancy_mode: PARITY
  erasure_coding:
    data_shards: 8
    parity_shards: 3            # RAID-6 ähnlich (Reed-Solomon)
  shards: 32
  min_doc_size: 10MB            # Nur große Dokumenter stripen
  
rationale:
  - Kostenoptimiert (37% Overhead statt 100%)
  - Hochgradig ausfallsicher (3 Shard-Ausfälle tolerieren)
  - Skalierbar zu 64+ Shards
```

---

## 4. Infrastructure Setup

### 4.1 Hardware-Provisioning

```bash
# Für 8-Shard Cluster mit STRIPE_MIRROR Modus

SHARD_COUNT=8
STORAGE_PER_SHARD_GB=500        # 500GB = 4TB gesamt

for i in $(seq 1 $SHARD_COUNT); do
  SHARD_ID=$(printf "shard_%03d" $i)
  
  # RocksDB Directory vorbereiten
  mkdir -p /data/themis/rocksdb/$SHARD_ID
  mkdir -p /data/themis/wal/$SHARD_ID
  mkdir -p /data/themis/backup/$SHARD_ID
  
  # Permissions setzen
  chown -R themis:themis /data/themis/rocksdb/$SHARD_ID
  chown -R themis:themis /data/themis/wal/$SHARD_ID
  chmod 750 /data/themis/rocksdb/$SHARD_ID
  chmod 750 /data/themis/wal/$SHARD_ID
  
  # Filesystem-Check
  df -h /data/themis/rocksdb/$SHARD_ID
done
```

### 4.2 OS-Konfiguration

```bash
# System-Limits erhöhen (für RocksDB + Replication)
cat >> /etc/security/limits.conf <<'EOF'
themis soft nofile 1048576
themis hard nofile 1048576
themis soft nproc 65536
themis hard nproc 65536
EOF

# Kernel-Parameter optimieren
cat >> /etc/sysctl.conf <<'EOF'
# Netzwerk-Buffer für Replication
net.core.rmem_max=134217728
net.core.wmem_max=134217728
net.ipv4.tcp_rmem=4096 87380 134217728
net.ipv4.tcp_wmem=4096 65536 134217728

# TCP Keepalive für lange Connections
net.ipv4.tcp_keepalives_time=600
net.ipv4.tcp_keepalives_probes=3
net.ipv4.tcp_keepalives_intvl=15

# Synchronisierte State Buckets (für Raft)
net.ipv4.tcp_max_syn_backlog=8192
net.core.netdev_max_backlog=65536
EOF

sysctl -p
```

### 4.3 Netzwerk-Topologie

```yaml
# /etc/themis/network-config.yaml
network:
  # Shard-Port Range
  shard_ports:
    start: 8080
    end: 8087
    
  # mTLS Gateway (Load Balancer vor Shards)
  gateway:
    listen_address: 0.0.0.0:443
    cert_file: /etc/themis/certs/gateway.crt
    key_file: /etc/themis/certs/gateway.key
    
  # Inter-Shard Communication
  inter_shard:
    protocol: mTLS
    timeout_ms: 5000
    keepalive_interval_ms: 30000
    
  # Raft Consensus Communication
  raft:
    heartbeat_timeout_ms: 150
    election_timeout_ms: 300
    snapshot_interval_logs: 10000
```

---

## 5. Shard-Konfiguration

### 5.1 Shard-Konfiguration Template (YAML)

```yaml
# /etc/themis/shard-001-config.yaml

shard:
  id: "shard_001"
  model: relational              # relational, graph, vector, timeseries, document
  namespace: "production"
  
  # RocksDB Backend
  storage:
    engine: rocksdb
    data_dir: /data/themis/rocksdb/shard_001
    wal_dir: /data/themis/wal/shard_001
    backup_dir: /data/themis/backup/shard_001
    
    # Performance Tuning
    block_cache_size_gb: 32         # L1 Cache in Memory
    write_buffer_size_mb: 256       # Memtable Size
    max_open_files: 65536
    compression: lz4                # Optionen: none, lz4, snappy, zstd
    
  # Replication Settings
  replication:
    mode: STRIPE_MIRROR             # NONE, MIRROR, STRIPE, STRIPE_MIRROR, PARITY, GEO_MIRROR
    replication_factor: 2
    
  # Für STRIPE_MIRROR Modus
  striping:
    enabled: true
    stripe_size: 65536              # 64KB
    stripe_min_shards: 4
    min_doc_size_mb: 1              # Nur Docs > 1MB stripen
    
  # Für PARITY Modus (falls verwendet)
  erasure_coding:
    enabled: false
    algorithm: REED_SOLOMON         # CAUCHY, LRC
    data_shards: 4
    parity_shards: 2
    
  # Network Endpoints
  network:
    # Primary Endpoint (für Writes)
    primary:
      host: themis-shard-001.prod.internal
      port: 8080
      
    # Replica Endpoints (für Reads bei MIRROR)
    replicas:
      - host: themis-shard-002.prod.internal
        port: 8080
        priority: 0
      - host: themis-shard-003.prod.internal
        port: 8080
        priority: 1
        
    # Raft Consensus Endpoint
    raft:
      host: themis-shard-001.prod.internal
      port: 8090
      
  # Consensus Settings
  consensus:
    engine: raft                    # PAXOS, RAFT, VIEWSTAMPED_REPLICATION
    heartbeat_timeout_ms: 150
    election_timeout_ms: 300
    snapshot_interval_logs: 10000
    log_replication: WAL            # WAL-basierte Replication
    
  # Failover Settings
  failover:
    auto_failover: true
    failover_timeout_ms: 5000
    max_failover_attempts: 3
    failover_backoff_ms: 1000
    
  # Circuit Breaker (Cascade Failure Prevention)
  circuit_breaker:
    enabled: true
    failure_threshold: 50           # % Fehler bevor Circuit öffnet
    timeout_ms: 30000               # Wie lange Circuit offen bleibt
    success_threshold: 2            # Erfolgreiche Requests zum Schließen
    
  # Health Checking
  health_check:
    enabled: true
    interval_ms: 10000
    timeout_ms: 5000
    failure_threshold: 3            # Failures bevor UNHEALTHY
    
  # Monitoring
  monitoring:
    prometheus_port: 9090
    metrics_enabled: true
    histograms_enabled: true        # Latency Histograms
    profiling_enabled: false        # CPU Profiling bei Bedarf
```

### 5.2 Konfiguration für verschiedene Modi

**MIRROR Mode (High Availability):**
```yaml
replication:
  mode: MIRROR
  replication_factor: 3
  read_preference: NEAREST           # Load-Balance Reads
  write_concern: MAJORITY            # Quorum Writes
```

**STRIPE_MIRROR Mode (Balanced):**
```yaml
replication:
  mode: STRIPE_MIRROR
  replication_factor: 2
  
striping:
  enabled: true
  stripe_size: 65536
  stripe_min_shards: 4
  min_doc_size_mb: 1
```

**PARITY Mode (Cost-Optimized):**
```yaml
replication:
  mode: PARITY

erasure_coding:
  enabled: true
  algorithm: REED_SOLOMON
  data_shards: 4
  parity_shards: 2                  # 4+2 = 67% Effizienz
  min_doc_size_mb: 10
```

---

## 6. PKI & TLS Setup

### 6.1 Certificate Hierarchy

```
┌──────────────────────────────────────────────────────┐
│         Root CA (themis-root-ca.crt)                 │
│   - Self-signed, valid 10 years                      │
│   - Private Key in HSM/encrypted storage             │
└────────────────┬─────────────────────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────────────────────┐
│    Intermediate CA (themis-cluster-ca.crt)           │
│   - Signiert von Root CA, valid 5 Jahre             │
│   - Private Key für Shard-Certificate-Signing        │
└────────────────┬─────────────────────────────────────┘
                 │
         ┌───────┼───────┬─────────┐
         ▼       ▼       ▼         ▼
   ┌──────────┐ ┌──────────┐   ┌──────────┐  ┌──────────┐
   │Shard-001 │ │Shard-002 │...│Gateway   │  │Proxy     │
   │.crt      │ │.crt      │   │.crt      │  │.crt      │
   │+ URN Ext │ │+ URN Ext │   │          │  │          │
   └──────────┘ └──────────┘   └──────────┘  └──────────┘
```

### 6.2 CA Setup Script

```bash
#!/bin/bash
set -e

CERT_DIR="/etc/themis/certs"
mkdir -p $CERT_DIR

# 1. Root CA generieren
echo "=== Generating Root CA ==="
openssl genrsa -out $CERT_DIR/themis-root-ca.key 4096

openssl req -new -x509 -days 3650 \
  -key $CERT_DIR/themis-root-ca.key \
  -out $CERT_DIR/themis-root-ca.crt \
  -subj "/CN=Themis-Root-CA/O=Themis/C=DE"

# 2. Intermediate CA generieren
echo "=== Generating Intermediate CA ==="
openssl genrsa -out $CERT_DIR/themis-cluster-ca.key 4096

openssl req -new \
  -key $CERT_DIR/themis-cluster-ca.key \
  -out $CERT_DIR/themis-cluster-ca.csr \
  -subj "/CN=Themis-Cluster-CA/O=Themis/C=DE"

# Von Root CA signieren
openssl x509 -req -days 1825 \
  -in $CERT_DIR/themis-cluster-ca.csr \
  -CA $CERT_DIR/themis-root-ca.crt \
  -CAkey $CERT_DIR/themis-root-ca.key \
  -CAcreateserial -out $CERT_DIR/themis-cluster-ca.crt \
  -extfile /dev/stdin <<'EOF'
basicConstraints=CA:TRUE,pathlen:0
keyUsage=keyCertSign,cRLSign
EOF

rm $CERT_DIR/themis-cluster-ca.csr

# 3. Shard Certificates generieren (für alle Shards)
for SHARD_ID in shard_{001..008}; do
  echo "=== Generating Certificate for $SHARD_ID ==="
  
  # Shard Key
  openssl genrsa -out $CERT_DIR/$SHARD_ID.key 2048
  
  # Shard CSR mit URN-Extension
  openssl req -new \
    -key $CERT_DIR/$SHARD_ID.key \
    -out $CERT_DIR/$SHARD_ID.csr \
    -subj "/CN=$SHARD_ID/O=Themis/C=DE"
  
  # Signieren von Cluster-CA
  openssl x509 -req -days 365 \
    -in $CERT_DIR/$SHARD_ID.csr \
    -CA $CERT_DIR/themis-cluster-ca.crt \
    -CAkey $CERT_DIR/themis-cluster-ca.key \
    -CAcreateserial -out $CERT_DIR/$SHARD_ID.crt \
    -extfile /dev/stdin <<EOF
subjectAltName=DNS:$SHARD_ID.prod.internal,DNS:$SHARD_ID
extendedKeyUsage=serverAuth,clientAuth
EOF

  rm $CERT_DIR/$SHARD_ID.csr
done

# 4. Gateway Certificate (für Client-facing mTLS)
echo "=== Generating Gateway Certificate ==="
openssl genrsa -out $CERT_DIR/gateway.key 2048
openssl req -new -key $CERT_DIR/gateway.key -out $CERT_DIR/gateway.csr \
  -subj "/CN=themis-gateway.prod.internal/O=Themis/C=DE"

openssl x509 -req -days 365 \
  -in $CERT_DIR/gateway.csr \
  -CA $CERT_DIR/themis-cluster-ca.crt \
  -CAkey $CERT_DIR/themis-cluster-ca.key \
  -CAcreateserial -out $CERT_DIR/gateway.crt \
  -extfile /dev/stdin <<'EOF'
subjectAltName=DNS:themis-gateway.prod.internal
extendedKeyUsage=serverAuth
EOF

rm $CERT_DIR/gateway.csr

# Permissions
chmod 600 $CERT_DIR/*.key
chown themis:themis $CERT_DIR/*

echo "=== Certificates generated in $CERT_DIR ==="
ls -la $CERT_DIR/*.crt
```

### 6.3 mTLS Configuration

```yaml
# /etc/themis/mtls-config.yaml

mtls:
  enabled: true
  
  # Server-side TLS (Shard als Server)
  server:
    cert_file: /etc/themis/certs/shard_001.crt
    key_file: /etc/themis/certs/shard_001.key
    ca_file: /etc/themis/certs/themis-cluster-ca.crt
    client_auth: REQUIRE           # Require Client Certificate
    min_tls_version: "1.2"
    ciphers:
      - TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
      - TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
      - TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
      
  # Client-side TLS (Shard als Client, für Inter-Shard Comm)
  client:
    cert_file: /etc/themis/certs/shard_001.crt
    key_file: /etc/themis/certs/shard_001.key
    ca_file: /etc/themis/certs/themis-cluster-ca.crt
    server_verify: true            # Verify Server Certificate
    min_tls_version: "1.2"
    
  # Certificate Validation
  validation:
    verify_hostname: true
    check_urn_extension: true      # Verify URN im Shard-Cert
    
  # Certificate Rotation
  rotation:
    enabled: true
    check_interval_days: 30        # Check 30 Tage vor Expiration
    rotate_before_expiry_days: 14  # Rotate 14 Tage vor Expiration
```

---

## 7. Shard Initialization

### 7.1 Bootstrap Script

```bash
#!/bin/bash
set -e

SHARD_ID="shard_001"
SHARD_PORT=8080
RAFT_PORT=8090
CONFIG_FILE="/etc/themis/${SHARD_ID}-config.yaml"

echo "=== Initializing $SHARD_ID ==="

# 1. RocksDB initialisieren
themis-cli shard init \
  --shard-id $SHARD_ID \
  --model relational \
  --namespace production \
  --config $CONFIG_FILE

# 2. Shard registrieren in Cluster
themis-cli shard register \
  --shard-id $SHARD_ID \
  --primary-endpoint themis-shard-001.prod.internal:$SHARD_PORT \
  --raft-endpoint themis-shard-001.prod.internal:$RAFT_PORT \
  --cert-file /etc/themis/certs/$SHARD_ID.crt \
  --capabilities read,write,replicate

# 3. Shard starten
systemctl start themis-shard@$SHARD_ID

# 4. Health Check
sleep 5
themis-cli shard health --shard-id $SHARD_ID

echo "=== $SHARD_ID initialized and started ==="
```

### 7.2 Systemd Service für Shards

```ini
# /etc/systemd/system/themis-shard@.service

[Unit]
Description=ThemisDB Shard %i
Documentation=file:///etc/themis/README.md
After=network.target

[Service]
Type=simple
User=themis
Group=themis
WorkingDirectory=/data/themis

# Shard-spezifisch (%i = shard_001, shard_002, etc.)
Environment="SHARD_ID=%i"
EnvironmentFile=/etc/themis/%i-config.yaml
ExecStart=/usr/local/bin/themis-shard-server \
  --config /etc/themis/%i-config.yaml \
  --log-level info \
  --prometheus-port 909%i

# Resource Limits
LimitNOFILE=1048576
LimitNPROC=65536

# Restart Policy
Restart=on-failure
RestartSec=10s
StartLimitInterval=600s
StartLimitBurst=3

# Logging
StandardOutput=journal
StandardError=journal
SyslogIdentifier=themis-shard

[Install]
WantedBy=multi-user.target
```

### 7.3 Cluster Startup Sequence

```bash
#!/bin/bash
# Start all shards in correct order

echo "=== Starting RAID-Themis Cluster ==="

# 1. Alle Shards zeitgleich starten (parallel)
for SHARD_ID in shard_{001..008}; do
  echo "Starting $SHARD_ID..."
  systemctl start themis-shard@$SHARD_ID &
done
wait

# 2. Auf Cluster-Formation warten (Raft Consensus)
sleep 10
echo "Waiting for Raft consensus..."
for SHARD_ID in shard_{001..008}; do
  themis-cli shard health --shard-id $SHARD_ID --wait 60 || exit 1
done

# 3. Cluster-Status anzeigen
echo "=== Cluster Status ==="
themis-cli cluster topology
themis-cli cluster health
themis-cli metrics summary

echo "=== RAID-Themis Cluster is READY ==="
```

---

## 8. Verification & Testing

### 8.1 Health Checks

```bash
# URN-based Routing Test
themis-cli test urn \
  --urn "urn:themis:relational:customers:users:550e8400-..." \
  --expected-shard shard_001

# mTLS Connection Test
themis-cli test mtls \
  --shard shard_001 \
  --cert /etc/themis/certs/shard_001.crt

# Replication Test
themis-cli test replication \
  --primary shard_001 \
  --replicas shard_002,shard_003 \
  --duration 60s

# Consensus Test (Raft)
themis-cli test raft \
  --cluster shard_001,shard_002,shard_003 \
  --test heartbeat,election,failover
```

### 8.2 Load Test

```bash
#!/bin/bash

# Workload Mix A: OLTP (Read Heavy)
themis-bench --workload-mix A \
  --shards 8 \
  --duration 120s \
  --threads 32 \
  --target-throughput 100000

# Workload Mix B: OLTP (Balanced)
themis-bench --workload-mix B \
  --shards 8 \
  --duration 120s \
  --threads 32 \
  --target-throughput 50000

# Workload Mix C: Analytical
themis-bench --workload-mix C \
  --shards 8 \
  --duration 60s \
  --threads 8
```

### 8.3 Failover Test

```bash
#!/bin/bash

echo "=== Testing Shard Failover ==="

# 1. Baseline-Throughput messen
BASELINE=$(themis-cli metrics get throughput)
echo "Baseline: $BASELINE ops/sec"

# 2. Eine Replica killen (für MIRROR Mode)
docker kill themis-shard-002

# 3. Automatisches Failover beobachten
sleep 5
NEW_STATUS=$(themis-cli shard health --shard-id shard_001)
echo "Shard Status nach Failover: $NEW_STATUS"

# 4. Durchsatz-Degradation messen
sleep 10
DEGRADED=$(themis-cli metrics get throughput)
echo "Throughput während Failover: $DEGRADED ops/sec"
echo "Degradation: $(( (BASELINE - DEGRADED) * 100 / BASELINE ))%"

# 5. Recovery-Zeit messen
docker start themis-shard-002
RECOVERY_START=$(date +%s)
themis-cli shard health --shard-id shard_001 --wait 120
RECOVERY_END=$(date +%s)
RECOVERY_TIME=$(( RECOVERY_END - RECOVERY_START ))

echo "Recovery Time: ${RECOVERY_TIME}s"
```

---

## 9. Production Cutover

### 9.1 Dual-Write Strategy

```
Phase 1: Start Dual-Write (1 Woche vorher)
┌──────────────┐       ┌──────────────┐
│Old System    │       │RAID-Themis   │
│(Single-Node) │       │(New Cluster)  │
└──────┬───────┘       └────────┬──────┘
       │                        │
       └────────────┬───────────┘
                    │
            All Writes go to BOTH
            (Versioning für Conflicts)

Phase 2: Validation (1-2 Wochen)
       │
       ├─► Datensynchronisation überprüfen
       ├─► Durchsatz-Vergleich
       ├─► Latenz-Vergleich
       └─► Query-Validation

Phase 3: Read Switching (Schrittweise über 1-2 Wochen)
       │
       ├─► 10% Reads auf RAID-Themis
       ├─► 25% Reads auf RAID-Themis
       ├─► 50% Reads auf RAID-Themis
       ├─► 75% Reads auf RAID-Themis
       └─► 100% Reads auf RAID-Themis

Phase 4: Full Cutover
       │
       ├─► Alle Writes auf RAID-Themis
       ├─► Dual-Write stoppen
       └─► Old System in Read-Only Mode
```

### 9.2 Cutover Playbook

```yaml
cutover:
  pre_cutover_checklist:
    - [ ] Alle Shards healthy (health check bestanden)
    - [ ] Cluster-Topology korrekt
    - [ ] PKI-Zertifikate gültig
    - [ ] Monitoring und Alerting aktiv
    - [ ] Backups aktuell
    - [ ] Rollback-Procedure getestet
    - [ ] Stakeholder briefing abgeschlossen
    
  cutover_steps:
    - step: 1
      description: "Enable Dual-Write Mode"
      action: |
        themis-cli cluster enable dual-write
      verification:
        - "All writes go to both systems"
        - "Version vectors tracked"
      rollback: "themis-cli cluster disable dual-write"
      
    - step: 2
      description: "Wait for Data Sync (1 hour)"
      action: |
        sleep 3600
        themis-cli cluster check-sync
      verification:
        - "Data sync lag < 1 second"
        - "No unsync'd records"
        
    - step: 3
      description: "Start Read Switching"
      action: |
        themis-cli cluster switch-reads --percentage 10
        sleep 600
        themis-cli cluster switch-reads --percentage 50
        sleep 600
        themis-cli cluster switch-reads --percentage 100
      verification:
        - "Read latency acceptable"
        - "Read throughput stable"
        
    - step: 4
      description: "Monitor for 1 hour"
      action: "Monitor dashboards"
      verification:
        - "No increase in errors"
        - "Latency p99 < 10ms"
        - "Throughput meets SLA"
        
    - step: 5
      description: "Disable Dual-Write"
      action: |
        themis-cli cluster disable dual-write
        themis-cli cluster decommission old-system
      verification:
        - "All writes only on RAID-Themis"
        - "Old system offline"
```

---

## 10. Post-Deployment Operations

### 10.1 Rebalancing

```bash
# Rebalancing bei neuen Shards triggern
themis-cli cluster rebalance \
  --target-shards 16 \
  --method consistent-hash \
  --data-migration-rate 100MB/s

# Rebalancing-Status beobachten
themis-cli cluster rebalance-status --watch
```

### 10.2 Backup Strategy

```bash
#!/bin/bash

# Tägliche Snapshots (für alle Shards)
for SHARD_ID in shard_{001..008}; do
  themis-cli shard backup create \
    --shard-id $SHARD_ID \
    --type snapshot \
    --destination /data/themis/backup/$SHARD_ID/daily-$(date +%Y%m%d).snap
done

# Backup zu S3 für langfristige Archivierung
aws s3 sync /data/themis/backup/ \
  s3://themis-backups/prod/$(date +%Y%m%d)/ \
  --delete
```

### 10.3 Monitoring Dashboards

Siehe: **SHARDING_MONITORING_OBSERVABILITY_v1.4.md**

---

## 11. Troubleshooting

### 11.1 Häufige Probleme

| Problem | Ursache | Lösung |
|---------|---------|--------|
| Shard unhealthy | Network Timeout | Check mTLS Config, Firewall rules |
| High Latency | Consensus Slowness | Increase Election Timeout, Check Network |
| Replication Lag | Slow Replicas | Scale up Replica, Check IO |
| Certificate Expired | Rotation failed | Manually rotate, check permissions |
| Data Skew | Hash Ring imbalanced | Rebalance, check stripe configuration |

### 11.2 Debug Commands

```bash
# Cluster-State anzeigen
themis-cli cluster topology
themis-cli cluster health
themis-cli raft state --shard shard_001

# Metriken live
themis-cli metrics stream --filter latency,throughput,errors

# Logs anzeigen
journalctl -u themis-shard@shard_001 -f

# TLS Debug
openssl s_client -connect themis-shard-001.prod.internal:8080 \
  -cert /etc/themis/certs/shard_001.crt \
  -key /etc/themis/certs/shard_001.key \
  -CAfile /etc/themis/certs/themis-cluster-ca.crt
```

---

## 12. Rollback Procedures

### 12.1 Sofortiger Rollback (< 1 Stunde)

```bash
#!/bin/bash

echo "=== Initiating Rollback to Single-Node ==="

# 1. Alle RAID-Themis Shards stoppen
for SHARD_ID in shard_{001..008}; do
  systemctl stop themis-shard@$SHARD_ID
done

# 2. Dual-Write deaktivieren (auf Old System zurück)
themis-cli cluster disable dual-write

# 3. Old Single-Node System hochfahren
systemctl start themis-single-node

# 4. Health Check
themis-cli health --wait 60

echo "=== Rollback completed ==="
```

### 12.2 Partial Rollback (bestimmte Daten zurück)

```bash
# Spezifische Shard-Range aus Backup wiederherstellen
themis-cli shard restore \
  --shard-id shard_003 \
  --from-backup /data/themis/backup/shard_003/latest.snap \
  --verify-urn-range "urn:themis:relational:customers:*"
```

---

## Zusammenfassung

Diese Anleitung deckt den vollständigen Lebenszyklus eines **RAID-Themis Sharding-Clusters** ab:

- ✅ Architektur & Redundanzwahl
- ✅ Pre-Deployment Planning
- ✅ Infrastructure & PKI Setup
- ✅ Shard-Konfiguration & Initialization
- ✅ Testing & Validation
- ✅ Production Cutover
- ✅ Operations & Troubleshooting
- ✅ Rollback Procedures

**Nächste Schritte:**
1. Redundanzmodus auswählen (STRIPE_MIRROR empfohlen)
2. Pre-Deployment Checklist durcharbeiten
3. PKI Setup durchführen
4. Test-Cluster deployen
5. Load Tests durchführen
6. Production Cutover nach diesem Playbook

---

**Kontakt & Support:**
- Engineering: engineering@themis.io
- Operations: ops@themis.io
- Security: security@themis.io
