# ThemisDB Inter-Shard Streaming Architecture

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Sharding

---


## Aktueller Stand vs. Cassandra-Ansatz

### Cassandra Streaming (Referenz)

Cassandra verwendet ein ausgeklügeltes Streaming-Protokoll für Node-zu-Node-Datentransfer:

1. **StreamSession**: Verwaltung einer einzelnen Streaming-Sitzung zwischen zwei Nodes
2. **StreamPlan**: Orchestrierung mehrerer StreamSessions
3. **StreamCoordinator**: Koordination aller aktiven Streams
4. **StreamResultFuture**: Asynchrone Ergebnisverfolgung
5. **StreamTransferTask / StreamReceiveTask**: Senden/Empfangen von Daten

**Cassandra Features:**
- Zero-Copy Transfers (direkt aus SSTables)
- Kompression während Transfer (LZ4, Snappy)
- Checksummen pro Chunk
- Bandbreitenbegrenzung (Throttling)
- Automatische Wiederaufnahme bei Unterbrechung
- Progress Tracking per SSTable

### ThemisDB Aktueller Ansatz

**Implementiert in:**
- `DataMigrator`: Batch-basierte Migration
- `AutoRebalancer`: Orchestrierung
- `MTLSClient`: Sichere Kommunikation

**Aktuelle Features:**
- ✅ mTLS-gesicherte Kommunikation
- ✅ Batch-basierte Verarbeitung (configurable batch_size)
- ✅ SHA-256 Integrity Verification
- ✅ Retry mit Exponential Backoff
- ✅ Progress Callbacks
- ✅ Token-Range-basierte Migration

**Fehlend (im Vergleich zu Cassandra):**
- ❌ Zero-Copy Transfer
- ❌ Streaming-Kompression
- ❌ Chunk-basiertes Protokoll mit Checksummen
- ❌ Bandbreitenbegrenzung
- ❌ Resume bei Unterbrechung
- ❌ Multi-Stream Parallelisierung
- ❌ Pending Ranges Tracking
- ❌ Snapshot-basierter Transfer

## Vorgeschlagene Architektur

### Streaming-Protokoll Schichten

```
┌─────────────────────────────────────────────────────────────┐
│                    StreamCoordinator                        │
│  - Verwaltet alle aktiven StreamSessions                   │
│  - Globale Bandbreitenbegrenzung                           │
│  - Priorisierung von Streams                               │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      StreamPlan                             │
│  - Plant mehrere Sessions für eine Migration               │
│  - Token-Range Aufteilung                                  │
│  - Parallelisierungsgrad                                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     StreamSession                           │
│  - Eine bidirektionale Verbindung zwischen zwei Shards     │
│  - mTLS-gesichert                                          │
│  - State Machine: INITIALIZED → PREPARING → STREAMING →    │
│                   COMPLETE/FAILED                          │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┴─────────────────────┐
        ▼                                           ▼
┌───────────────────┐                     ┌───────────────────┐
│ StreamTransferTask│                     │ StreamReceiveTask │
│ - Sendet Chunks   │                     │ - Empfängt Chunks │
│ - Komprimiert     │                     │ - Dekomprimiert   │
│ - Checksummen     │                     │ - Verifiziert     │
└───────────────────┘                     └───────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    StreamMessage                            │
│  Protocol Buffer / Binary Format:                          │
│  - Header: type, session_id, sequence_no, flags            │
│  - Payload: compressed data chunks                         │
│  - Trailer: CRC32 checksum                                 │
└─────────────────────────────────────────────────────────────┘
```

### Message Types

```cpp
enum class StreamMessageType : uint8_t {
    // Session Management
    PREPARE_REQUEST = 0x01,     // Initiiere Session
    PREPARE_ACK = 0x02,         // Bestätige Vorbereitung
    
    // Data Transfer
    FILE_HEADER = 0x10,         // Beschreibt zu übertragende Datei/Collection
    DATA_CHUNK = 0x11,          // Datenchunk
    DATA_CHUNK_ACK = 0x12,      // Bestätigung für Chunk
    
    // Control
    RETRY_REQUEST = 0x20,       // Chunk-Wiederholung anfordern
    ABORT = 0x21,               // Session abbrechen
    COMPLETE = 0x22,            // Transfer abgeschlossen
    COMPLETE_ACK = 0x23,        // Abschluss bestätigt
    
    // Keepalive
    HEARTBEAT = 0x30,
    HEARTBEAT_ACK = 0x31
};
```

### Kompression

Unterstützte Algorithmen:
- **LZ4**: Standard (schnell, moderate Kompression)
- **Zstd**: Für hohe Kompressionsraten
- **None**: Für bereits komprimierte Daten

### Bandbreitenbegrenzung

```cpp
struct StreamThrottleConfig {
    uint64_t max_bytes_per_second = 0;           // 0 = unlimited
    uint64_t inter_dc_max_bytes_per_second = 0;  // Für DC-übergreifend
    bool prioritize_local_dc = true;
};
```

## Integration mit bestehender Architektur

### Betroffene Komponenten

1. **DataMigrator** → Verwendet StreamSession
2. **AutoRebalancer** → Verwendet StreamPlan
3. **GossipProtocol** → Kommuniziert Streaming-Status
4. **HealthCheck** → Überwacht Stream-Gesundheit
5. **PrometheusMetrics** → Streaming-Metriken

### Neue Metriken

```
themisdb_streaming_active_sessions
themisdb_streaming_bytes_transferred_total{direction="send|receive"}
themisdb_streaming_chunk_errors_total
themisdb_streaming_throughput_bytes_per_second
themisdb_streaming_compression_ratio
themisdb_streaming_pending_ranges_total
```

## Implementierungsplan

### Phase 1: Basis-Streaming (implementiert)
- [x] Batch-basierte Migration
- [x] mTLS-Kommunikation
- [x] Progress Tracking
- [x] Integrity Verification

### Phase 2: Advanced Streaming
- [ ] StreamSession mit State Machine
- [ ] Chunk-basiertes Protokoll
- [ ] Kompression (LZ4)
- [ ] Checksummen pro Chunk

### Phase 3: Production Features
- [ ] StreamCoordinator
- [ ] Bandbreitenbegrenzung
- [ ] Resume/Retry
- [ ] Multi-Stream Parallelisierung

### Phase 4: Optimierungen
- [ ] Zero-Copy Transfer
- [ ] Adaptive Kompression
- [ ] Predictive Scheduling
