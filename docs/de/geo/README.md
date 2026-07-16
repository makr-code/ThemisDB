# Geospatial Module

**Stand:** 6. April 2026  
**Version:** 2.1.0  
**Kategorie:** Geo

---

## Übersicht

Das Geo-Modul implementiert räumliche Operationen für ThemisDB mit pluggable Backends (CPU/CUDA/ROCm). Es bietet OGC-konforme Spatial-Predicates, vollständige GeoJSON RFC 7946 Unterstützung, R-Tree Indexierung, räumliche JOINs, Geo-Clustering, Raster-Abfragen, temporale Geo-Abfragen und eine Tile-Server-Integration.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| ISpatialComputeBackend | `spatial_backend.h` | — | Backend Interface |
| IGeoRegistry | `spatial_backend.h` | — | Plugin Registry |
| IGeoOpsExtension | `geo_ops_ext.h` | — | Erweiterungs-Operationen |
| CpuExactBackend | — | `cpu_backend.cpp` | Exakte CPU-Implementierung |
| BoostCpuExactBackend | — | `boost_cpu_exact_backend.cpp` | Boost.Geometry CPU-Backend |
| GpuBackendStub | — | `gpu_backend_stub.cpp` | GPU-Dispatcher mit CPU-Fallback |
| GpuBackendCuda | — | `gpu_backend_cuda.cu` | CUDA-Kernel-Dispatch (NVIDIA) |
| GpuBackendHip | — | `gpu_backend_hip.cpp` | ROCm/HIP-Kernel-Dispatch (AMD) |
| GpuBackendProduction | — | `gpu_backend_production.cpp` | Produktions-GPU-Backend (Metrics, Audit) |
| GeoRtree | `geo_rtree.h` | `geo_rtree.cpp` | R-Tree Spatial Index |
| SpatialJoin | `spatial_join.h` | `spatial_join.cpp` | Räumlicher JOIN |
| GeoCluster | `geo_clustering.h` | `geo_clustering.cpp` | DBSCAN + k-Means Clustering |
| RasterGrid | `raster.h` | `raster.cpp` | Raster-Daten-Abfragen |
| TemporalSpatialQuery | `temporal_spatial_query.h` | `temporal_spatial_query.cpp` | Temporale Geo-Abfragen |
| TileServer | `tile_server.h` | `tile_server.cpp` | Tile-Server-Integration |
| DeviceDetector | `device_detector.h` | `device_detector.cpp` | GPU-Geräteerkennung |
| GpuKernelDispatcher | `gpu_kernel_dispatcher.h` | `gpu_kernel_dispatcher_cpu.cpp` | Kernel-Dispatcher |

**Gesamt:** 10 Header, 14 Source-Dateien, ~4 800 LOC

## Implementierte Klassen

### ISpatialComputeBackend

```cpp
class ISpatialComputeBackend {
    virtual const char* name() const noexcept = 0;
    virtual bool isAvailable() const noexcept = 0;
    
    // Batch intersects für Kandidaten-Filter
    virtual SpatialBatchResults batchIntersects(const SpatialBatchInputs& in) = 0;
    
    // Exakte Intersects-Prüfung
    virtual bool exactIntersects(const GeometryInfo& geom1, const GeometryInfo& geom2) = 0;
};
```

### SpatialBatchInputs / SpatialBatchResults

```cpp
struct SpatialBatchInputs {
    std::size_t count{0};
    // Koordinaten, MBR-Arrays, Kandidaten-IDs (SoA/AoSoA Layout)
};

struct SpatialBatchResults {
    std::vector<uint8_t> mask;  // 1 = hit, 0 = no hit
};
```

### IGeoRegistry (Plugin System)

```cpp
class IGeoRegistry {
    virtual void registerBackend(std::unique_ptr<ISpatialComputeBackend> backend) = 0;
};

// Plugin Entry Point
using RegisterGeoPluginFn = void(*)(IGeoRegistry*);
// extern "C" void RegisterGeoPlugin(IGeoRegistry* registry);
```

## Backend Factory

```cpp
// CPU Backend (exakt, Boost.Geometry)
ISpatialComputeBackend* getCpuExactBackend();

// CPU Backend (näherungsweise, MBR-basiert)
ISpatialComputeBackend* getCpuApproximateBackend();

// Precision-basierte Auswahl
ISpatialComputeBackend* getBackendForPrecision(GeoPrecisionMode mode);

// GPU Backend (CUDA/HIP mit automatischem CPU-Fallback)
ISpatialComputeBackend* getGpuBackend();
```

## Features

### Räumliche Operationen
- **Intersects** — Überschneidungsprüfung
- **Contains** — Enthält-Prüfung
- **Distance** — Distanzberechnung (Haversine)
- **Within** — Innerhalb-Prüfung
- **ST_BUFFER** — Geometrie um feste Distanz erweitern
- **ST_UNION** — Vereinigung zweier Geometrien
- **ST_DIFFERENCE** — Differenz zweier Geometrien

### Indexierung
- **R-Tree** — Räumlicher Index (Boost.Geometry rstar, lazy build)
- **S2 Cells** — Google S2 Geometrie
- **H3 Cells** — Uber H3 Hexagons
- **Geohash** — String-basierte Tiles

### GPU Acceleration
- **CUDA Backend** — NVIDIA GPUs (`gpu_backend_cuda.cu`, `THEMIS_GEO_CUDA=ON`)
- **ROCm/HIP Backend** — AMD GPUs (`gpu_backend_hip.cpp`, `THEMIS_ENABLE_HIP=ON`)
- **CPU Fallback** — automatisch via Circuit-Breaker bei CUDA/HIP-Fehler

### Erweiterte Features
- **GeoJSON RFC 7946** — alle 7 Geometrietypen (inkl. GeometryCollection, MultiPolygon)
- **Spatial JOIN** — alle Paare innerhalb einer konfigurierbaren Distanz
- **DBSCAN Clustering** — dichtebasierte Geo-Punkt-Cluster
- **k-Means Clustering** — zentroidsbasierte Geo-Punkt-Cluster (inkl. k-means++)
- **Raster-Abfragen** — Höhenprofil, Bbox-Extraktion, Gaussian KDE Heatmaps
- **Temporale Geo-Abfragen** — Standort zu Zeitpunkt T (`SystemVersionedTable`)
- **Tile Server** — Karten-Tile-Server-Integration

## AQL Geo-Funktionen

```aql
// Distance Query
FOR doc IN locations
  LET dist = ST_DISTANCE(doc.point, @userLocation)
  FILTER dist < 1000
  SORT dist ASC
  RETURN {doc, distance: dist}

// Contains Query
FOR doc IN regions
  FILTER ST_CONTAINS(doc.polygon, @point)
  RETURN doc

// Intersects Query
FOR doc IN areas
  FILTER ST_INTERSECTS(doc.geometry, @searchArea)
  RETURN doc

// Buffer Query
FOR doc IN features
  LET buffered = ST_Buffer(doc.geometry, 500)
  FILTER ST_INTERSECTS(buffered, @searchArea)
  RETURN doc
```

## Bekannte Einschränkungen

- ST_BUFFER, ST_UNION und ST_DIFFERENCE verwenden auf dem GPU-Pfad aktuell CPU-Fallback; dedizierte CUDA-Kernel sind für v2.2.0 geplant.
- DBSCAN und k-Means nutzen O(n²) Brute-Force; GPU-Beschleunigung für v2.3.0 geplant.
- Sphärische WGS-84-Ellipsoid-Geometrie (Vincenty/Karney) ist für v2.5.0 geplant.

## Verwandte Dokumentation

- [geo_integration.md](geo_integration.md) — Integration Guide
- [geo_feature_tiering.md](geo_feature_tiering.md) — Feature Tiers
- [geo_benchmarks.md](geo_benchmarks.md) — Performance-Benchmarks
- [geo_architecture.md](geo_architecture.md) — Architektur-Details
- [src/geo/ROADMAP.md](../../../src/geo/ROADMAP.md) — Modul-Roadmap
- [src/geo/FUTURE_ENHANCEMENTS.md](../../../src/geo/FUTURE_ENHANCEMENTS.md) — Geplante Erweiterungen mit wissenschaftlichen Quellen
