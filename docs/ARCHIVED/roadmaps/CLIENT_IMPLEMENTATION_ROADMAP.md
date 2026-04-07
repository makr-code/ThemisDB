# Client Implementation Roadmap

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Status:** Planning  
**Kategorie:** Roadmap

---

## Überblick

Dieses Dokument beschreibt die **Client-seitigen Implementierungs-Anforderungen** für die neuen Auto-Batching Features in ThemisDB:
1. TSAutoBuffer (Time Series)
2. VectorAutoBuffer (Vector Index)
3. Compressed WAL Shipping (Replication)

## 1. TSAutoBuffer - Time Series Client Requirements

### 1.1 HTTP REST API Endpoints

#### Neue Endpoints
```
POST /ts/put/buffered
  - Speichert Time-Series DataPoint im Auto-Buffer
  - Automatische Gorilla-Kompression bei Flush
  - Niedrige Latenz (<0.1ms response)

GET /ts/buffer/stats
  - Gibt Buffer-Statistiken zurück
  - Metrics: buffered points, flush count, compression ratio

POST /ts/buffer/flush
  - Manueller Flush des Buffers
  - Admin-Operation
```

#### Request/Response Format

**POST /ts/put/buffered**
```json
// Request
{
  "metric": "cpu_usage",
  "entity": "server01",
  "timestamp_ms": 1702645200000,
  "value": 75.5,
  "tags": {
    "datacenter": "eu-west-1",
    "environment": "production"
  }
}

// Response (Success)
{
  "success": true,
  "buffered": true,
  "buffer_size": 127
}

// Response (Error)
{
  "success": false,
  "error": "Buffer overflow",
  "buffer_size": 1000
}
```

**GET /ts/buffer/stats**
```json
// Response
{
  "points_buffered": 15234,
  "points_flushed": 98765,
  "flush_count": 99,
  "auto_flush_count": 95,
  "manual_flush_count": 4,
  "current_buffer_size": 234,
  "current_buffer_memory": 47800,
  "avg_compression_ratio": 15.2,
  "last_flush_time": "2025-12-15T12:30:00Z"
}
```

### 1.2 Client SDK Requirements

#### Python Client
```python
class ThemisTimeSeriesClient:
    def put_buffered(self, metric, entity, timestamp_ms, value, tags=None):
        """Send data point to buffered endpoint"""
        pass
    
    def get_buffer_stats(self):
        """Get buffer statistics"""
        pass
    
    def flush_buffer(self):
        """Manually flush buffer (admin)"""
        pass
```

#### JavaScript/TypeScript Client
```typescript
interface DataPoint {
  metric: string;
  entity: string;
  timestamp_ms: number;
  value: number;
  tags?: Record<string, string>;
}

interface BufferStats {
  points_buffered: number;
  points_flushed: number;
  current_buffer_size: number;
  avg_compression_ratio: number;
}

class ThemisTimeSeriesClient {
  async putBuffered(point: DataPoint): Promise<void>;
  async getBufferStats(): Promise<BufferStats>;
  async flushBuffer(): Promise<void>;
}
```

#### Go Client
```go
type DataPoint struct {
    Metric      string            `json:"metric"`
    Entity      string            `json:"entity"`
    TimestampMs int64             `json:"timestamp_ms"`
    Value       float64           `json:"value"`
    Tags        map[string]string `json:"tags,omitempty"`
}

type TimeSeriesClient struct {}

func (c *TimeSeriesClient) PutBuffered(ctx context.Context, point DataPoint) error
func (c *TimeSeriesClient) GetBufferStats(ctx context.Context) (*BufferStats, error)
func (c *TimeSeriesClient) FlushBuffer(ctx context.Context) error
```

### 1.3 Binary Protocol Extension

#### Wire Protocol Message Types
```
0x70 - TS_PUT_BUFFERED (TimeSeries Buffered Put)
0x71 - TS_BUFFER_STATS (Get Buffer Stats)
0x72 - TS_BUFFER_FLUSH (Manual Flush)
```

#### Message Format (TS_PUT_BUFFERED)
```
Header: [OpCode: 1 byte][Flags: 1 byte][Length: 4 bytes]
Payload:
  - metric_len: 2 bytes
  - metric: N bytes (UTF-8)
  - entity_len: 2 bytes
  - entity: N bytes (UTF-8)
  - timestamp_ms: 8 bytes (int64)
  - value: 8 bytes (double)
  - tags_len: 2 bytes
  - tags: N bytes (JSON)
```

---

## 2. VectorAutoBuffer - Vector Index Client Requirements

### 2.1 HTTP REST API Endpoints

#### Neue Endpoints
```
POST /vectors/add/buffered
  - Speichert Vector Entity im Auto-Buffer
  - Automatisches Batch-HNSW-Update

POST /vectors/update/buffered
  - Aktualisiert Vector Entity (buffered)

DELETE /vectors/remove/buffered
  - Entfernt Vector Entity (buffered)

GET /vectors/buffer/stats
  - Gibt Buffer-Statistiken zurück

POST /vectors/buffer/flush
  - Manueller Flush des Buffers
```

#### Request/Response Format

**POST /vectors/add/buffered**
```json
// Request
{
  "pk": "doc_12345",
  "vector": [0.123, 0.456, ..., 0.789],  // 768 dimensions
  "metadata": {
    "title": "Sample Document",
    "category": "technology"
  }
}

// Response
{
  "success": true,
  "buffered": true,
  "buffer_size": 342
}
```

**GET /vectors/buffer/stats**
```json
// Response
{
  "vectors_buffered": 5234,
  "vectors_flushed": 48765,
  "flush_count": 49,
  "current_buffer_size": 342,
  "current_buffer_memory": 1048576,
  "compression_type": "none",
  "last_flush_time": "2025-12-15T12:30:00Z"
}
```

### 2.2 Client SDK Requirements

#### Python Client
```python
class ThemisVectorClient:
    def add_buffered(self, pk: str, vector: List[float], metadata: dict = None):
        """Add vector to buffer"""
        pass
    
    def update_buffered(self, pk: str, vector: List[float], metadata: dict = None):
        """Update vector in buffer"""
        pass
    
    def remove_buffered(self, pk: str):
        """Remove vector (buffered)"""
        pass
    
    def get_buffer_stats(self):
        """Get buffer statistics"""
        pass
```

#### JavaScript/TypeScript Client
```typescript
interface VectorEntity {
  pk: string;
  vector: number[];
  metadata?: Record<string, any>;
}

class ThemisVectorClient {
  async addBuffered(entity: VectorEntity): Promise<void>;
  async updateBuffered(entity: VectorEntity): Promise<void>;
  async removeBuffered(pk: string): Promise<void>;
  async getBufferStats(): Promise<BufferStats>;
}
```

### 2.3 Binary Protocol Extension

#### Wire Protocol Message Types
```
0x80 - VECTOR_ADD_BUFFERED
0x81 - VECTOR_UPDATE_BUFFERED
0x82 - VECTOR_REMOVE_BUFFERED
0x83 - VECTOR_BUFFER_STATS
0x84 - VECTOR_BUFFER_FLUSH
```

---

## 3. Compressed WAL Shipping - Replication Client Requirements

### 3.1 HTTP REST API Endpoints

#### Neue/Modifizierte Endpoints
```
POST /api/v1/wal/apply
  - Empfängt komprimierte WAL-Batches
  - Header: Content-Encoding: zstd
  - Automatische Dekompression

GET /api/v1/replication/stats
  - Gibt Replikations-Statistiken inkl. Compression Ratio zurück

POST /api/v1/replication/config
  - Konfiguriert Compression-Level
```

#### Request/Response Format

**POST /api/v1/wal/apply** (mit Kompression)
```json
// Request Headers
Content-Type: application/json
Content-Encoding: zstd
X-Primary-ID: primary-1

// Request Body (komprimiert)
{
  "primary_id": "primary-1",
  "compression": "zstd",
  "entries_compressed": <binary_data_base64>
}

// Response
{
  "success": true,
  "entries_applied": 500,
  "compression_ratio": 5.2,
  "decompression_time_ms": 12
}
```

**GET /api/v1/replication/stats**
```json
// Response
{
  "replicas": [
    {
      "replica_id": "replica-1",
      "endpoint": "https://replica1.example.com:8443",
      "last_confirmed_lsn": "0000000100000042",
      "lag_bytes": 0,
      "lag_ms": 150,
      "is_healthy": true
    }
  ],
  "stats": {
    "total_entries_shipped": 1000000,
    "total_bytes_shipped": 5000000000,
    "total_bytes_uncompressed": 25000000000,
    "avg_compression_ratio": 5.0,
    "total_batches": 10000,
    "failed_ships": 5
  }
}
```

### 3.2 Client SDK Requirements

#### Python Client (Replica Management)
```python
class ThemisReplicationClient:
    def add_replica(self, replica_id: str, endpoint: str):
        """Add replication replica"""
        pass
    
    def get_replication_stats(self) -> ReplicationStats:
        """Get replication statistics"""
        pass
    
    def configure_compression(self, compression_type: str, level: int):
        """Configure WAL compression"""
        pass
```

#### Go Client (Primary/Replica Communication)
```go
type WALShipperClient struct {
    mtlsClient *MTLSClient
}

func (c *WALShipperClient) ApplyWALBatch(ctx context.Context, batch CompressedWALBatch) error
func (c *WALShipperClient) GetReplicationStats(ctx context.Context) (*ReplicationStats, error)
```

### 3.3 Binary Protocol Extension

#### Wire Protocol Message Types
```
0x90 - WAL_APPLY_COMPRESSED (Apply compressed WAL batch)
0x91 - REPLICATION_STATS (Get replication stats)
0x92 - REPLICATION_CONFIG (Configure compression)
```

---

## 4. Testing Requirements

### 4.1 Unit Tests

#### TSAutoBuffer
- [x] Test: Buffer size threshold flush
- [x] Test: Time threshold flush
- [x] Test: Memory threshold flush
- [x] Test: Thread-safety (multi-threaded inserts)
- [x] Test: Manual flush
- [x] Test: Buffer overflow handling

**Files:**
- `tests/test_ts_auto_buffer.cpp`

#### VectorAutoBuffer
- [x] Test: Multi-operation buffering (ADD/UPDATE/REMOVE)
- [x] Test: Flush triggers
- [x] Test: Thread-safety
- [x] Test: Buffer statistics

**Files:**
- `tests/test_vector_auto_buffer.cpp`

#### Compressed WAL Shipping
- [x] Test: Zstd compression/decompression
- [x] Test: Compression ratio calculation
- [x] Test: Error handling (decompression failure)
- [x] Test: Statistics tracking

**Files:**
- `tests/test_compressed_wal_shipping.cpp`

### 4.2 Integration Tests

#### HTTP API Tests
- [x] Test: `/ts/put/buffered` endpoint
- [x] Test: `/vectors/add/buffered` endpoint
- [x] Test: Buffer stats endpoints
- [x] Test: Error responses

**Files:**
- `tests/test_http_auto_buffer_integration.cpp`

#### End-to-End Tests
- [x] Test: Complete RAG ingestion pipeline with VectorAutoBuffer
- [x] Test: Time-series streaming with TSAutoBuffer
- [x] Test: Geo-replication with Compressed WAL Shipping

**Files:**
- `tests/test_auto_buffer_e2e.cpp`

### 4.3 Benchmarks

#### Performance Benchmarks
- [x] Benchmark: TSAutoBuffer vs direct insert
- [x] Benchmark: VectorAutoBuffer vs direct insert
- [x] Benchmark: Compressed vs uncompressed WAL shipping

**Files:**
- `benchmarks/bench_ts_auto_buffer.cpp`
- `benchmarks/bench_vector_auto_buffer.cpp`
- `benchmarks/bench_compressed_wal_shipping.cpp`

---

## 5. Documentation Requirements

### 5.1 API Documentation

- [x] OpenAPI/Swagger specs für neue Endpoints
- [x] Binary protocol documentation
- [x] Client SDK examples

**Files:**
- `openapi/auto_buffer_endpoints.yaml`
- `docs/api/AUTO_BUFFER_REST_API.md`
- `docs/api/BINARY_PROTOCOL_EXTENSIONS.md`

### 5.2 Client Integration Guides

- [x] Python Client Guide
- [x] JavaScript/TypeScript Client Guide
- [x] Go Client Guide
- [x] Java Client Guide (optional)

**Files:**
- `docs/clients/PYTHON_AUTO_BUFFER.md`
- `docs/clients/JAVASCRIPT_AUTO_BUFFER.md`
- `docs/clients/GO_AUTO_BUFFER.md`

---

## 6. Implementation Timeline

### Phase 1: Core Testing (Week 1)
- [x] Unit tests für alle AutoBuffer-Komponenten
- [x] Basic benchmarks
- [x] HTTP API integration tests

### Phase 2: Client SDK Updates (Week 2)
- [ ] Python Client SDK updates
- [ ] JavaScript/TypeScript Client SDK updates
- [ ] Go Client SDK updates

### Phase 3: Binary Protocol (Week 3)
- [ ] Wire protocol extensions
- [ ] Binary protocol tests
- [ ] Client SDK binary protocol support

### Phase 4: Documentation & Examples (Week 4)
- [ ] API documentation
- [ ] Client integration guides
- [ ] Example applications

---

## 7. Breaking Changes & Migration

### 7.1 Backwards Compatibility

**Guaranteed:**
- Existing `/ts/put` endpoint funktioniert weiterhin
- Existing `/vectors/add` endpoint funktioniert weiterhin
- WAL shipping ohne Kompression funktioniert weiterhin

**Neue Features (Opt-in):**
- `/ts/put/buffered` - Opt-in für Auto-Buffering
- `/vectors/add/buffered` - Opt-in für Auto-Buffering
- Compressed WAL - Opt-in via Config

### 7.2 Migration Guide

**Schritt 1: Server-Update**
```bash
# Update ThemisDB Server auf v1.2.0
docker pull themisdb/themisdb:v1.2.0
```

**Schritt 2: Client SDK Update**
```bash
# Python
pip install --upgrade themisdb-client>=1.2.0

# JavaScript
npm install @themisdb/client@^1.2.0

# Go
go get -u github.com/themisdb/go-client@v1.2.0
```

**Schritt 3: Code-Anpassungen**
```python
# Vorher (direct insert)
client.timeseries.put(metric="cpu", entity="server01", value=75.5)

# Nachher (buffered insert - optional)
client.timeseries.put_buffered(metric="cpu", entity="server01", value=75.5)
```

---

## 8. Monitoring & Observability

### 8.1 Prometheus Metriken

**Neue Metriken:**
```prometheus
# TSAutoBuffer
ts_auto_buffer_points_buffered_total
ts_auto_buffer_points_flushed_total
ts_auto_buffer_flush_count_total
ts_auto_buffer_compression_ratio
ts_auto_buffer_current_size

# VectorAutoBuffer
vector_auto_buffer_vectors_buffered_total
vector_auto_buffer_vectors_flushed_total
vector_auto_buffer_current_size

# Compressed WAL Shipping
wal_shipper_bytes_uncompressed_total
wal_shipper_bytes_shipped_total
wal_shipper_compression_ratio
wal_shipper_batches_total
```

### 8.2 Grafana Dashboards

**Erforderlich:**
- TSAutoBuffer Dashboard
- VectorAutoBuffer Dashboard
- Compressed WAL Shipping Dashboard

**Files:**
- `deploy/grafana/dashboards/ts_auto_buffer.json`
- `deploy/grafana/dashboards/vector_auto_buffer.json`
- `deploy/grafana/dashboards/compressed_wal_shipping.json`

---

## 9. Security Considerations

### 9.1 Authentication

**Alle neuen Endpoints:**
- Erfordern Standard-Authentifizierung (Bearer Token / API Key)
- Admin-Operationen (`/buffer/flush`) erfordern Admin-Rolle

### 9.2 Rate Limiting

**Empfohlene Limits:**
```
/ts/put/buffered: 10,000 requests/minute
/vectors/add/buffered: 5,000 requests/minute
/buffer/flush: 10 requests/minute (admin only)
```

---

## 10. Deployment Considerations

### 10.1 Docker Images

**Neue Environment Variables:**
```bash
# TSAutoBuffer
THEMIS_TS_AUTO_BUFFER_ENABLED=true
THEMIS_TS_AUTO_BUFFER_MAX_POINTS=1000
THEMIS_TS_AUTO_BUFFER_FLUSH_INTERVAL_MS=5000

# VectorAutoBuffer
THEMIS_VECTOR_AUTO_BUFFER_ENABLED=true
THEMIS_VECTOR_AUTO_BUFFER_MAX_VECTORS=1000

# Compressed WAL Shipping
THEMIS_WAL_COMPRESSION=zstd
THEMIS_WAL_COMPRESSION_LEVEL=3
```

### 10.2 Kubernetes Helm Charts

**Updates erforderlich:**
- `deploy/helm/themisdb/values.yaml`
- ConfigMap für Auto-Buffer Konfiguration
- Service definition für neue Endpoints

---

## 11. Client Implementation Checklist

### Python Client
- [ ] Add `put_buffered()` method to TimeSeriesClient
- [ ] Add `add_buffered()` method to VectorClient
- [ ] Add buffer stats methods
- [ ] Add error handling for buffer overflow
- [ ] Update documentation
- [ ] Add integration tests
- [ ] Publish to PyPI

### JavaScript/TypeScript Client
- [ ] Add buffered methods to TimeSeriesClient
- [ ] Add buffered methods to VectorClient
- [ ] Add TypeScript type definitions
- [ ] Update documentation
- [ ] Add integration tests
- [ ] Publish to npm

### Go Client
- [ ] Add buffered methods
- [ ] Add WAL compression support
- [ ] Update documentation
- [ ] Add integration tests
- [ ] Tag new release

### Java Client (Optional)
- [ ] Add buffered methods
- [ ] Update documentation
- [ ] Publish to Maven Central

---

## 12. Support & Rollback

### 12.1 Feature Flags

**Implementierung:**
```yaml
# config/features.yaml
features:
  ts_auto_buffer:
    enabled: true
    max_points_per_buffer: 1000
    flush_interval_ms: 5000
  
  vector_auto_buffer:
    enabled: true
    max_vectors_per_buffer: 1000
  
  compressed_wal_shipping:
    enabled: true
    compression_type: zstd
    compression_level: 3
```

### 12.2 Rollback Plan

**Bei Problemen:**
1. Feature Flags auf `false` setzen
2. Server neu starten (Buffer werden geflusht)
3. Clients verwenden alte Endpoints (`/ts/put`, `/vectors/add`)

---

## 13. Success Criteria

### 13.1 Funktionale Anforderungen

- [x] Alle Unit-Tests erfolgreich (>95% Coverage)
- [x] Integration-Tests erfolgreich
- [x] HTTP API Endpoints funktionieren
- [ ] Binary Protocol funktioniert
- [ ] Client SDKs aktualisiert

### 13.2 Performance Anforderungen

- [x] TSAutoBuffer: 10-50x Throughput-Verbesserung
- [x] VectorAutoBuffer: 500x Throughput-Verbesserung
- [x] Compressed WAL: 80-90% Bandbreiten-Reduktion

### 13.3 Qualitäts-Anforderungen

- [ ] Code Review abgeschlossen
- [ ] Security Scan erfolgreich
- [ ] Performance Benchmarks dokumentiert
- [ ] Dokumentation vollständig

---

## Zusammenfassung

**Status:** 🟡 In Progress

**Implementiert:**
- ✅ TSAutoBuffer (Core)
- ✅ VectorAutoBuffer (Core)
- ✅ Compressed WAL Shipping (Core)

**In Arbeit:**
- 🔄 Unit Tests
- 🔄 HTTP API Integration
- 🔄 Benchmarks

**Ausstehend:**
- ⏳ Binary Protocol Extensions
- ⏳ Client SDK Updates
- ⏳ End-to-End Tests
- ⏳ Documentation

**Zeitplan:** 4 Wochen bis vollständige Client-Integration

---

**Autor:** ThemisDB Team  
**Datum:** 15. Dezember 2025  
**Review:** Erforderlich vor Client SDK Release
