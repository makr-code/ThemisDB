# Time Series Module

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** TimeSeries

---

## Übersicht

Das Time-Series-Modul bietet hochperformante Zeitserien-Speicherung für Metriken und Events mit Gorilla-Kompression und Continuous Aggregates.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| TimeSeriesStore | `timeseries.h` | `timeseries.cpp` | Haupt-API |
| GorillaEncoder | `gorilla.h` | `gorilla.cpp` | Kompression |
| GorillaDecoder | `gorilla.h` | `gorilla.cpp` | Dekompression |
| TSStore | `tsstore.h` | `tsstore.cpp` | Low-Level Store |
| ContinuousAggregateManager | `continuous_agg.h` | `continuous_agg.cpp` | Auto-Aggregation |
| RetentionManager | `retention.h` | `retention.cpp` | Retention Policies |

**Gesamt:** 7 Header, 8 Source-Dateien, ~2,800 LOC

## Implementierte Klassen

### TimeSeriesStore

```cpp
class TimeSeriesStore {
    // Key Schema: ts:{metric}:{entity}:{timestamp_ms}
    // Value: double (für Metriken) oder JSON (für Events)
    
    struct DataPoint {
        int64_t timestamp_ms;
        double value;
        nlohmann::json metadata;
    };
    
    struct RangeQuery {
        int64_t from_ms;
        int64_t to_ms;
        size_t limit = 1000;
        bool descending = false;
    };
    
    struct Aggregation {
        double min, max, avg, sum;
        size_t count;
    };
    
    // API
    Status put(metric, entity, timestamp_ms, value, metadata);
    std::vector<DataPoint> range(metric, entity, RangeQuery);
    Aggregation aggregate(metric, entity, RangeQuery);
};
```

### Gorilla Compression (10-20x)

```cpp
class GorillaEncoder {
    // Paper: "Gorilla: A Fast, Scalable, In-Memory Time Series Database"
    // - XOR-basierte Delta-Kompression für Timestamps
    // - XOR-basierte Float-Kompression für Values
    
    void encodeTimestamp(int64_t timestamp);
    void encodeValue(double value);
    std::vector<uint8_t> finish();
};

class GorillaDecoder {
    bool hasNext();
    DataPoint next();
};
```

### ContinuousAggregateManager

```cpp
class ContinuousAggregateManager {
    // Automatische Voraggregation
    enum class BucketSize { MINUTE, HOUR, DAY, WEEK, MONTH };
    
    void createAggregate(name, metric, entity, BucketSize, AggFunc);
    void refresh(name);
    std::vector<AggregatePoint> query(name, from, to);
};
```

### RetentionManager

```cpp
class RetentionManager {
    // Automatische Datenbereinigung nach TTL
    void setPolicy(metric, retention_days);
    void enforce();  // Background Job
    RetentionStats getStats();
};
```

## Beispiel

```cpp
TimeSeriesStore ts(db);

// Metriken schreiben
ts.put("cpu_usage", "server-1", now_ms(), 75.5);
ts.put("cpu_usage", "server-1", now_ms() + 1000, 78.2);

// Range Query
auto points = ts.range("cpu_usage", "server-1", {
    .from_ms = start,
    .to_ms = end,
    .limit = 1000
});

// Aggregation
auto agg = ts.aggregate("cpu_usage", "server-1", query);
// agg.avg = 76.85, agg.min = 75.5, agg.max = 78.2
```

## Performance

- **Kompression:** 10-20x mit Gorilla
- **Write:** ~100,000 points/sec
- **Read:** ~500,000 points/sec (komprimiert)
- **Aggregation:** O(n) single-pass

## Verwandte Dokumentation

- [Features: Time Series](../features/features_time_series.md) - Feature-Details
