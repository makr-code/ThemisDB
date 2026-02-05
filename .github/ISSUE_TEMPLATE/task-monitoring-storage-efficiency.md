---
name: Storage-Effizienz-Metriken
about: Metriken zur Storage-Effizienz (Kompressionsraten, Dedup-Savings, Tier-Distribution)
title: '[MONITORING] Storage-Effizienz-Metriken'
labels: ['enhancement', 'future', 'content-pipeline', 'monitoring', 'storage', 'priority:low']
assignees: ''
---

## Beschreibung

Implementierung von Metriken zur Überwachung der Storage-Effizienz, einschließlich Kompressionsraten, Dedup-Savings und Tier-Distribution, mit Historical-Tracking und Dashboard-Integration.

## Kontext

Storage-Kosten sind ein wichtiger Faktor für Production-Systeme. Diese Metriken ermöglichen Monitoring von Storage-Effizienz, ROI-Berechnung und Optimierungsmöglichkeiten.

## Ziele

- Berechnung von Storage-Efficiency-Metriken
- Tracking von Savings durch Compression und Deduplication
- Visualisierung von Storage-Tier-Distribution
- Historical-Tracking für Trend-Analyse
- Integration mit Grafana-Dashboards

## Lösungsansatz

### Schritt 1: Storage-Efficiency-Calculator (1 Tag)
- **Tasks**:
  - [ ] Efficiency-Calculation-Engine implementieren:
    - Compression-Ratio-Berechnung
    - Dedup-Savings-Berechnung
    - Tier-Distribution-Berechnung
    - Total-Storage-Savings
  - [ ] Aggregation über Zeit (täglich, wöchentlich, monatlich)
  - [ ] Per-Content-Type-Breakdown

### Schritt 2: Savings-Report-Generation (0.5 Tag)
- **Tasks**:
  - [ ] Report-Generator für Storage-Savings
  - [ ] Vergleich: Actual vs. Uncompressed Storage
  - [ ] Cost-Savings-Calculation (wenn Cost-Model verfügbar)
  - [ ] ROI-Berechnung für Compression/Dedup
  - [ ] Export als JSON/CSV/PDF

### Schritt 3: Historical-Tracking (1 Tag)
- **Tasks**:
  - [ ] Time-Series-Storage für Metriken
  - [ ] Snapshot-basiertes Tracking (täglich)
  - [ ] Trend-Analyse (wachsend/fallend)
  - [ ] Anomaly-Detection (unerwartete Änderungen)
  - [ ] Retention-Policy (z.B. 90 Tage)

### Schritt 4: Dashboard-Integration (0.5 Tag)
- **Tasks**:
  - [ ] Prometheus-Metriken für Storage-Efficiency
  - [ ] Grafana-Dashboard-Panels
  - [ ] Storage-Efficiency-Summary-Widget
  - [ ] Historical-Trend-Visualisierung
  - [ ] Alert-Rules für Low-Efficiency

## Storage-Efficiency-Metriken

### Compression-Metriken
```cpp
class CompressionEfficiencyMetrics {
public:
    struct Metrics {
        uint64_t original_bytes;       // Uncompressed size
        uint64_t compressed_bytes;     // Compressed size
        double compression_ratio;      // Ratio (original/compressed)
        double space_saved_pct;        // Percentage saved
        uint64_t space_saved_bytes;    // Bytes saved
    };
    
    Metrics CalculateForContentType(const std::string& content_type) {
        Metrics m;
        m.original_bytes = GetOriginalBytes(content_type);
        m.compressed_bytes = GetCompressedBytes(content_type);
        m.compression_ratio = (double)m.original_bytes / m.compressed_bytes;
        m.space_saved_bytes = m.original_bytes - m.compressed_bytes;
        m.space_saved_pct = 100.0 * m.space_saved_bytes / m.original_bytes;
        return m;
    }
    
    Metrics CalculateTotal() {
        // Aggregate across all content types
    }
};
```

### Deduplication-Metriken
```cpp
class DeduplicationEfficiencyMetrics {
public:
    struct Metrics {
        uint64_t unique_chunks;        // Number of unique chunks
        uint64_t total_references;     // Total references to chunks
        double dedup_ratio;            // Ratio (references/unique)
        uint64_t space_saved_bytes;    // Bytes saved by dedup
        double space_saved_pct;        // Percentage saved
    };
    
    Metrics Calculate() {
        Metrics m;
        m.unique_chunks = GetUniqueChunkCount();
        m.total_references = GetTotalChunkReferences();
        m.dedup_ratio = (double)m.total_references / m.unique_chunks;
        
        uint64_t avg_chunk_size = GetAverageChunkSize();
        m.space_saved_bytes = (m.total_references - m.unique_chunks) * avg_chunk_size;
        
        uint64_t total_logical_size = m.total_references * avg_chunk_size;
        m.space_saved_pct = 100.0 * m.space_saved_bytes / total_logical_size;
        
        return m;
    }
};
```

### Storage-Tier-Distribution
```cpp
class StorageTierDistribution {
public:
    struct Distribution {
        std::map<StorageTier, uint64_t> bytes_per_tier;
        std::map<StorageTier, uint64_t> chunks_per_tier;
        std::map<StorageTier, double> percentage_per_tier;
    };
    
    Distribution Calculate() {
        Distribution dist;
        uint64_t total_bytes = 0;
        
        for (auto tier : {StorageTier::HOT, StorageTier::WARM, StorageTier::COLD}) {
            dist.bytes_per_tier[tier] = GetBytesInTier(tier);
            dist.chunks_per_tier[tier] = GetChunksInTier(tier);
            total_bytes += dist.bytes_per_tier[tier];
        }
        
        for (auto tier : {StorageTier::HOT, StorageTier::WARM, StorageTier::COLD}) {
            dist.percentage_per_tier[tier] = 
                100.0 * dist.bytes_per_tier[tier] / total_bytes;
        }
        
        return dist;
    }
};
```

### Gesamt-Storage-Effizienz
```cpp
class StorageEfficiencySummary {
public:
    struct Summary {
        uint64_t logical_storage_bytes;    // Without compression/dedup
        uint64_t physical_storage_bytes;   // Actual storage used
        double overall_efficiency_ratio;   // Logical/Physical
        uint64_t total_savings_bytes;      // Bytes saved
        double total_savings_pct;          // Percentage saved
        
        CompressionEfficiencyMetrics::Metrics compression;
        DeduplicationEfficiencyMetrics::Metrics dedup;
        StorageTierDistribution::Distribution tiers;
    };
    
    Summary Calculate() {
        Summary s;
        s.compression = compression_metrics_.CalculateTotal();
        s.dedup = dedup_metrics_.Calculate();
        s.tiers = tier_metrics_.Calculate();
        
        s.logical_storage_bytes = CalculateLogicalStorage();
        s.physical_storage_bytes = CalculatePhysicalStorage();
        s.overall_efficiency_ratio = 
            (double)s.logical_storage_bytes / s.physical_storage_bytes;
        s.total_savings_bytes = s.logical_storage_bytes - s.physical_storage_bytes;
        s.total_savings_pct = 
            100.0 * s.total_savings_bytes / s.logical_storage_bytes;
        
        return s;
    }
};
```

## Savings-Report-Beispiel

```json
{
  "report_date": "2026-02-05",
  "period": "last_30_days",
  "storage_efficiency_summary": {
    "logical_storage": "5.2 TB",
    "physical_storage": "1.8 TB",
    "efficiency_ratio": 2.89,
    "total_savings": "3.4 TB",
    "savings_percentage": "65.4%"
  },
  "compression_savings": {
    "original_size": "5.2 TB",
    "compressed_size": "2.1 TB",
    "compression_ratio": 2.48,
    "space_saved": "3.1 TB",
    "savings_percentage": "59.6%",
    "breakdown_by_type": {
      "text": {"ratio": 2.8, "saved": "1.2 TB"},
      "image": {"ratio": 1.5, "saved": "0.8 TB"},
      "video": {"ratio": 3.2, "saved": "1.1 TB"}
    }
  },
  "deduplication_savings": {
    "unique_chunks": 1250000,
    "total_references": 1750000,
    "dedup_ratio": 1.4,
    "space_saved": "300 GB",
    "savings_percentage": "14.3%"
  },
  "storage_tier_distribution": {
    "hot": {"bytes": "400 GB", "percentage": "22.2%"},
    "warm": {"bytes": "800 GB", "percentage": "44.4%"},
    "cold": {"bytes": "600 GB", "percentage": "33.3%"}
  },
  "cost_savings": {
    "monthly_cost_without_optimization": "$520",
    "monthly_cost_with_optimization": "$180",
    "monthly_savings": "$340",
    "annual_roi": "$4,080"
  }
}
```

## Prometheus-Metriken

```prometheus
# Compression Metrics
content_storage_compression_ratio{content_type="text"} 2.8
content_storage_compression_ratio{content_type="image"} 1.5
content_storage_compression_ratio{content_type="video"} 3.2

# Deduplication Metrics
content_storage_dedup_ratio 1.4
content_storage_unique_chunks 1250000
content_storage_total_references 1750000

# Storage Tier Distribution (bytes)
content_storage_tier_bytes{tier="hot"} 429496729600
content_storage_tier_bytes{tier="warm"} 858993459200
content_storage_tier_bytes{tier="cold"} 644245094400

# Overall Efficiency
content_storage_efficiency_ratio 2.89
content_storage_savings_bytes 3649267441664
content_storage_logical_bytes 5583089352704
content_storage_physical_bytes 1933821911040
```

## Grafana-Dashboard-Panels

### Panel 1: Storage-Efficiency-Summary
```
┌─────────────────────────────────────────┐
│    Storage Efficiency Summary            │
├─────────────────────────────────────────┤
│ Logical Storage:  5.2 TB                │
│ Physical Storage: 1.8 TB                │
│ Efficiency Ratio: 2.89x                 │
│ Total Savings:    3.4 TB (65.4%)        │
└─────────────────────────────────────────┘
```

### Panel 2: Compression-Ratio-Trend
```
[Time Series Chart]
Y-axis: Compression Ratio
X-axis: Time
Lines: Text, Image, Video, Overall
```

### Panel 3: Storage-Tier-Distribution
```
[Pie Chart]
- HOT: 22.2% (400 GB)
- WARM: 44.4% (800 GB)
- COLD: 33.3% (600 GB)
```

### Panel 4: Savings-Over-Time
```
[Area Chart - Stacked]
Y-axis: Bytes
X-axis: Time
Areas: 
  - Physical Storage (bottom)
  - Compression Savings
  - Dedup Savings
```

## Historical-Tracking

```cpp
class StorageEfficiencyHistory {
public:
    void TakeSnapshot() {
        Snapshot snapshot;
        snapshot.timestamp = std::chrono::system_clock::now();
        snapshot.metrics = calculator_.Calculate();
        
        // Store in time-series DB
        time_series_db_.Store("storage_efficiency", snapshot);
    }
    
    std::vector<Snapshot> GetHistory(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) {
        return time_series_db_.Query("storage_efficiency", start, end);
    }
    
    TrendAnalysis AnalyzeTrend(std::chrono::days period) {
        auto end = std::chrono::system_clock::now();
        auto start = end - period;
        auto history = GetHistory(start, end);
        
        TrendAnalysis analysis;
        analysis.compression_ratio_trend = CalculateTrend(
            history, [](auto& s) { return s.compression.compression_ratio; });
        analysis.dedup_ratio_trend = CalculateTrend(
            history, [](auto& s) { return s.dedup.dedup_ratio; });
        
        return analysis;
    }
};
```

## Konfiguration

```yaml
# config/storage_efficiency.yml
storage_efficiency:
  enabled: true
  
  snapshot:
    interval: 24h  # Daily snapshots
    retention: 90d  # Keep 90 days
  
  metrics:
    prometheus_enabled: true
    export_interval: 60s
  
  reports:
    enabled: true
    schedule: "0 9 * * MON"  # Monday 9 AM
    recipients:
      - ops-team@example.com
    format: json  # json, csv, pdf
  
  cost_model:
    enabled: true
    hot_storage_cost_per_gb: 0.023  # $/GB/month
    warm_storage_cost_per_gb: 0.0125
    cold_storage_cost_per_gb: 0.004
```

## Integration Points

- [ ] StorageManager (Physical-Storage-Tracking)
- [ ] ZstdCompression (Compression-Stats)
- [ ] ContentManager (Logical-Storage-Tracking)
- [ ] DeduplicationManager (Dedup-Stats, wenn verfügbar)
- [ ] TieringManager (Tier-Distribution, wenn verfügbar)
- [ ] Prometheus-Exporter (Metriken-Export)

## Testing-Anforderungen

### Unit-Tests
```cpp
TEST(StorageEfficiency, CompressionRatioCalculation) {
    // Test compression ratio calculation
}

TEST(StorageEfficiency, DedupSavingsCalculation) {
    // Test dedup savings calculation
}

TEST(StorageEfficiency, TierDistribution) {
    // Test tier distribution calculation
}

TEST(StorageEfficiency, OverallEfficiency) {
    // Test overall efficiency summary
}
```

### Integration-Tests
- [ ] Metriken werden korrekt berechnet
- [ ] Historical-Snapshots funktionieren
- [ ] Prometheus-Export funktioniert
- [ ] Report-Generation funktioniert
- [ ] Dashboard zeigt korrekte Daten

## Success Criteria

- [ ] Storage-Efficiency-Calculator implementiert
- [ ] Alle wichtigen Metriken erfasst
- [ ] Historical-Tracking funktioniert
- [ ] Savings-Reports generiert
- [ ] Prometheus-Metriken exportiert
- [ ] Grafana-Dashboard funktioniert
- [ ] Trend-Analyse verfügbar
- [ ] Performance-Impact < 0.5%
- [ ] Unit- und Integration-Tests bestehen
- [ ] Dokumentation vollständig

## Priorität

**Niedrig** - Nützlich für Kosten-Optimierung, aber nicht kritisch

## Geschätzter Aufwand

**2-3 Tage**

## Dependencies

- **Benötigt**: Prometheus-Metriken für Content Pipeline (für Export)
- **Optional**: Cost-Model-Konfiguration (für Cost-Savings)
- **Optional**: Deduplication-System (für Dedup-Metriken)
- **Optional**: Tiered-Storage-System (für Tier-Distribution)
- **Related**: Grafana-Dashboards (für Visualisierung)

## Referenzen

- [ ] Storage-Efficiency-Best-Practices
- [ ] ROI-Calculation für Storage-Optimierung
- [ ] ThemisDB Storage-Architektur: `docs/storage/architecture.md`
- [ ] Cost-Model-Dokumentation: `docs/storage/cost_model.md`

---

**Checklist:**
- [ ] Ich habe die Anforderungen verstanden
- [ ] Ich habe Metriken-Berechnung geplant
- [ ] Ich habe Historical-Tracking-Strategie definiert
- [ ] Ich habe Report-Format spezifiziert
- [ ] Ich habe Dashboard-Integration geplant
- [ ] Ich habe Testing-Anforderungen definiert
