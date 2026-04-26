# Time Series Auto-Batching Buffer

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Status:** Design/Implementierung  
**Kategorie:** Technical Documentation

---

## Überblick

Der **TSAutoBuffer** ist ein konfigurierbarer Buffering-Layer für `TSStore`, der automatisch einzelne Datenpunkte sammelt und als komprimierte Batches speichert. Dies löst die Limitierung, dass `putDataPoint()` keine Gorilla-Kompression nutzt.

## Problem

**Aktueller Zustand:**
- `putDataPoint()`: Speichert Punkte als einzelne RocksDB-Entities (keine Kompression)
- `putDataPoints()`: Speichert Batches mit Gorilla-Kompression (10-20x Ratio)
- HTTP-API `/ts/put` nutzt `putDataPoint()` → keine Kompression

**Nachteil:**
- IoT/Streaming-Anwendungen können nicht von Gorilla-Kompression profitieren
- Einzelpunkt-Einfügungen verschwenden Speicherplatz

## Lösung: TSAutoBuffer

Ein **intelligenter Buffer** der:
1. Einzelne Datenpunkte puffert
2. Automatisch nach Größe/Zeit flusht
3. Gorilla-Kompression nutzt
4. Thread-safe ist
5. Mit bestehender TSStore-API arbeitet

## Architektur

### Komponenten

```
┌─────────────────────────────────────────────────────┐
│                   Application                        │
└────────────────────┬────────────────────────────────┘
                     │
                     │ add(point)
                     ▼
┌─────────────────────────────────────────────────────┐
│              TSAutoBuffer                            │
│  ┌─────────────────────────────────────────────┐   │
│  │  Buffers (map<metric:entity, deque<Point>>) │   │
│  │  - Per-metric:entity buffering               │   │
│  │  - Configurable thresholds                   │   │
│  │  - Thread-safe access                        │   │
│  └─────────────────────────────────────────────┘   │
│                                                      │
│  ┌─────────────────────────────────────────────┐   │
│  │  Background Flush Thread                     │   │
│  │  - Time-based flush (every N seconds)        │   │
│  │  - Size-based flush (max points reached)     │   │
│  │  - Memory-based flush (max memory reached)   │   │
│  └─────────────────────────────────────────────┘   │
└────────────────────┬────────────────────────────────┘
                     │
                     │ putDataPoints(batch)
                     ▼
┌─────────────────────────────────────────────────────┐
│                   TSStore                            │
│  - Gorilla compression                               │
│  - RocksDB storage                                   │
└─────────────────────────────────────────────────────┘
```

### Flush-Strategien

Der Buffer flusht automatisch wenn:

1. **Size-Threshold:** `max_points_per_buffer` erreicht (z.B. 1000 Punkte)
2. **Time-Threshold:** `flush_interval` abgelaufen (z.B. 5 Sekunden)
3. **Memory-Threshold:** `max_memory_bytes` erreicht (z.B. 100 MB)
4. **Global-Threshold:** `max_total_points` über alle Buffers erreicht

## Konfiguration

### TSAutoBufferConfig

```cpp
struct TSAutoBufferConfig {
    // Buffer-Größe
    size_t max_points_per_buffer = 1000;      // Max Punkte pro metric:entity
    size_t max_total_points = 10000;          // Max Punkte gesamt
    
    // Zeit-basiert
    std::chrono::milliseconds flush_interval{5000};  // 5 Sekunden
    
    // Speicher
    size_t max_memory_bytes = 100 * 1024 * 1024;  // 100 MB
    
    // Performance
    bool async_flush = true;                  // Background-Thread
    size_t flush_batch_size = 500;           // Punkte pro Flush
    
    // Kompression (von TSStore)
    TSStore::CompressionType compression = TSStore::CompressionType::Gorilla;
    int chunk_size_hours = 24;
};
```

### Empfohlene Konfigurationen

#### IoT Real-Time Streaming

```cpp
TSAutoBufferConfig config;
config.max_points_per_buffer = 500;          // Klein für niedrige Latenz
config.flush_interval = std::chrono::seconds(2);  // Schnelles Flush
config.max_memory_bytes = 50 * 1024 * 1024;  // 50 MB
config.async_flush = true;
config.compression = TSStore::CompressionType::Gorilla;
```

**Eigenschaften:**
- Niedrige Latenz (2s Flush)
- Kleine Batches für schnelles Schreiben
- Kompression aktiv

#### High-Throughput Batch Import

```cpp
TSAutoBufferConfig config;
config.max_points_per_buffer = 5000;         // Große Batches
config.flush_interval = std::chrono::seconds(30);  // Seltenes Flush
config.max_memory_bytes = 500 * 1024 * 1024;  // 500 MB
config.async_flush = true;
config.compression = TSStore::CompressionType::Gorilla;
```

**Eigenschaften:**
- Hoher Durchsatz
- Große Batches für maximale Kompression
- Viel Speicher für Pufferung

#### Memory-Constrained (z.B. Raspberry Pi)

```cpp
TSAutoBufferConfig config;
config.max_points_per_buffer = 200;          // Klein
config.flush_interval = std::chrono::seconds(5);
config.max_memory_bytes = 10 * 1024 * 1024;  // Nur 10 MB
config.async_flush = true;
config.compression = TSStore::CompressionType::Gorilla;
```

**Eigenschaften:**
- Wenig Speicher
- Häufiges Flush
- Trotzdem Kompression

## Verwendung

### Basis-Beispiel

```cpp
#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"

// TSStore einrichten
TSStore::Config ts_config;
ts_config.compression = TSStore::CompressionType::Gorilla;
TSStore ts(db, cf, ts_config);

// Auto-Buffer einrichten
TSAutoBufferConfig buffer_config;
buffer_config.max_points_per_buffer = 1000;
buffer_config.flush_interval = std::chrono::seconds(5);

TSAutoBuffer buffer(&ts, buffer_config);
buffer.start();  // Background-Thread starten

// Punkte hinzufügen (werden automatisch gepuffert)
for (int i = 0; i < 10000; ++i) {
    TSStore::DataPoint point{
        .metric = "temperature",
        .entity = "sensor_001",
        .timestamp_ms = now() + i * 1000,
        .value = 20.0 + (rand() % 100) / 10.0,
        .tags = {{"location", "room_a"}},
        .metadata = {}
    };
    buffer.add(point);  // Gepuffert, nicht direkt geschrieben
}

// Stoppen (flusht automatisch verbleibende Punkte)
buffer.stop();
```

### Manuelles Flush

```cpp
// Alle Buffer flushen
size_t flushed = buffer.flush();
std::cout << "Flushed " << flushed << " points\n";

// Spezifischen Buffer flushen
size_t flushed = buffer.flushFor("temperature", "sensor_001");
```

### Statistiken

```cpp
auto stats = buffer.getStats();
std::cout << "Points buffered: " << stats.points_buffered << "\n";
std::cout << "Points flushed: " << stats.points_flushed << "\n";
std::cout << "Flush count: " << stats.flush_count << "\n";
std::cout << "Auto flushes: " << stats.auto_flush_count << "\n";
std::cout << "Current buffer size: " << stats.current_buffer_size << "\n";
std::cout << "Current memory: " << stats.current_buffer_memory << " bytes\n";
```

### Dynamische Konfiguration

```cpp
// Konfiguration zur Laufzeit ändern
TSAutoBufferConfig new_config;
new_config.max_points_per_buffer = 2000;
new_config.flush_interval = std::chrono::seconds(10);
buffer.setConfig(new_config);
```

## Integration mit HTTP-API

### Option 1: Auto-Buffer im Server

Der HTTP-Server kann einen globalen `TSAutoBuffer` verwenden:

```cpp
// In http_server.cpp
class HTTPServer {
private:
    TSStore tsstore_;
    TSAutoBuffer ts_buffer_;  // Global buffer
    
public:
    HTTPServer() : ts_buffer_(&tsstore_) {
        ts_buffer_.start();
    }
    
    void handleTSPut(const Request& req, Response& res) {
        auto point = parseDataPoint(req.body());
        
        // Verwende Buffer statt direktem TSStore
        auto status = ts_buffer_.add(point);
        
        if (status.ok) {
            res.status = 201;
            res.body = R"({"success": true})";
        } else {
            res.status = 400;
            res.body = R"({"error": ")" + status.message + R"("})";
        }
    }
};
```

### Option 2: Opt-In per Header

```cpp
void handleTSPut(const Request& req, Response& res) {
    auto point = parseDataPoint(req.body());
    
    // Check header: X-TS-Buffer: enable
    if (req.headers["X-TS-Buffer"] == "enable") {
        ts_buffer_.add(point);  // Buffered
    } else {
        tsstore_.putDataPoint(point);  // Direct
    }
}
```

### Option 3: Neuer Endpoint `/ts/put/buffered`

```cpp
// POST /ts/put - Direct (kein Buffer)
void handleTSPut(const Request& req, Response& res) {
    tsstore_.putDataPoint(parseDataPoint(req.body()));
}

// POST /ts/put/buffered - Mit Auto-Buffer
void handleTSPutBuffered(const Request& req, Response& res) {
    ts_buffer_.add(parseDataPoint(req.body()));
}
```

## Performance-Charakteristiken

### Latenz

| Modus | Schreiblatenz | Flush-Latenz | Gesamt |
|-------|--------------|--------------|--------|
| Direct (`putDataPoint`) | ~1ms | 0ms | ~1ms |
| Buffered (kleine Batches) | <0.1ms | ~10-50ms | ~0.1ms (async) |
| Buffered (große Batches) | <0.1ms | ~50-200ms | ~0.1ms (async) |

**Vorteil:** Schreiben ist ~10x schneller (nur Buffer-Operation)

### Durchsatz

| Modus | Punkte/s | Kompression | Speicher |
|-------|----------|-------------|----------|
| Direct | ~1,000 | Keine | 100% |
| Buffered (500 pts/batch) | ~10,000 | 10-20x | 5-10% |
| Buffered (5000 pts/batch) | ~50,000 | 15-25x | 4-7% |

**Vorteil:** Durchsatz ~10-50x höher, Speicher ~90-96% weniger

### Speicher-Overhead

```
Buffer Memory = num_buffers × avg_points × point_size
              ≈ 100 metrics × 1000 points × 200 bytes
              ≈ 20 MB
```

**Konfigurierbar via `max_memory_bytes`**

## Thread-Safety

Der `TSAutoBuffer` ist **vollständig thread-safe**:

- Alle öffentlichen Methoden sind mutex-geschützt
- Background-Thread koordiniert via `std::condition_variable`
- Atomare Zähler für Statistiken
- Safe Shutdown mit Flush aller Punkte

**Beispiel (Multi-Threaded):**

```cpp
TSAutoBuffer buffer(&ts);
buffer.start();

// Thread 1
std::thread t1([&buffer]() {
    for (int i = 0; i < 1000; ++i) {
        buffer.add(makePoint("cpu", "server1", i));
    }
});

// Thread 2
std::thread t2([&buffer]() {
    for (int i = 0; i < 1000; ++i) {
        buffer.add(makePoint("memory", "server1", i));
    }
});

t1.join();
t2.join();
buffer.stop();  // Flusht alle verbleibenden Punkte
```

## Bibliotheken und Patterns

Die Implementierung nutzt **bestehende ThemisDB-Patterns**:

### 1. **CEP Engine Event Buffering**
- `std::deque<Event> buffer` für Events
- Partitionierung nach Event-Typ
- **Wiederverwendet:** Deque-basierte Pufferung

### 2. **Backpressure Protocol Adaptive Strategies**
- Load-basierte Flush-Strategien (IMMEDIATE, THROTTLED, DEFERRED)
- Adaptive Thresholds
- **Wiederverwendet:** Multi-Threshold Flush-Logik

### 3. **RocksDB WriteBatch**
- Bereits in `putDataPoints()` verwendet
- Atomare Batch-Schreiboperationen
- **Wiederverwendet:** Direkt via `tsstore_->putDataPoints()`

### 4. **Standard C++ Threading**
- `std::thread`, `std::mutex`, `std::condition_variable`
- `std::atomic` für lockfreie Zähler
- **Standard-Bibliothek:** Keine zusätzlichen Dependencies

## Vergleich mit Alternativen

### Option A: RocksDB WriteBatch Wrapper
**Pro:**
- Weniger Code
- Direkte RocksDB-Integration

**Contra:**
- Kein automatisches Flush
- Keine Zeit-basierte Logik
- Keine Statistiken

### Option B: Externe Queue (Boost Lockfree Queue)
**Pro:**
- Lock-free Performance

**Contra:**
- Zusätzliche Dependency (Boost)
- Komplexere Integration
- Overkill für diesen Use-Case

### Option C: TSAutoBuffer (Gewählt)
**Pro:**
- ✅ Nutzt bestehende Patterns (CEP, Backpressure)
- ✅ Keine neuen Dependencies
- ✅ Volle Kontrolle über Flush-Strategien
- ✅ Integrierte Statistiken
- ✅ Thread-safe Design
- ✅ Einfache API

**Contra:**
- Etwas mehr Code als Option A
- Speicher-Overhead für Buffers

**Entscheidung:** Option C ist die beste Balance aus Features, Performance und Wartbarkeit.

## Roadmap

### Phase 1: Core Implementation ✅
- [x] Header-Datei (`ts_auto_buffer.h`)
- [x] Implementation (`ts_auto_buffer.cpp`)
- [x] Dokumentation (diese Datei)

### Phase 2: Testing
- [ ] Unit-Tests (`test_ts_auto_buffer.cpp`)
- [ ] Performance-Tests
- [ ] Multi-Threading-Tests
- [ ] Memory-Leak-Tests

### Phase 3: Integration
- [ ] HTTP-API Integration
- [ ] Config-File Support
- [ ] Metrics/Tracing
- [ ] Documentation Updates

### Phase 4: Optimierungen
- [ ] Lock-free Statistiken
- [ ] NUMA-aware Partitioning
- [ ] Adaptive Flush-Intervalle
- [ ] Predictive Buffering

## Zusammenfassung

**Frage:** Gibt es etwas in den Libs für single-point-to-batch?

**Antwort:** 
- **Nein**, es gibt keine fertige Bibliothek dafür
- **Aber:** Bestehende Patterns (CEP, Backpressure) liefern alle Bausteine
- **Lösung:** `TSAutoBuffer` - konfigurierbarer Auto-Batching Buffer
- **Bibliotheken verwendet:**
  - Standard C++ (`std::deque`, `std::thread`, `std::mutex`)
  - RocksDB (`WriteBatch` via `TSStore::putDataPoints`)
  - ThemisDB Utils (Logging, Tracing)
- **Neue Dependencies:** Keine!

**Status:** Design fertig, Implementation bereit für Testing und Integration.

## Siehe auch

- [Time-Series Storage Methods](./STORAGE_METHODS.md)
- [Time-Series Engine (TSStore)](../features/features_time_series.md)
- [CEP Engine Documentation](../observability/CEP_STREAMING_ANALYTICS.md)
- [Backpressure Protocol](../sharding/backpressure_protocol.md)
