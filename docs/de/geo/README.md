# Geospatial Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Geo

---

## Übersicht

Das Geo-Modul implementiert räumliche Operationen für ThemisDB mit pluggable Backends (CPU/GPU).

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| ISpatialComputeBackend | `spatial_backend.h` | - | Backend Interface |
| IGeoRegistry | `spatial_backend.h` | - | Plugin Registry |
| IGeoOpsExtension | `geo_ops_ext.h` | - | Operations Extension |
| BoostCpuBackend | - | `cpu_backend.cpp` | CPU Implementation |
| GpuBackendStub | - | `gpu_backend_stub.cpp` | GPU Stub |

**Gesamt:** 2 Header, 3 Source-Dateien, ~300 LOC

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
// CPU Backend (Boost.Geometry)
ISpatialComputeBackend* getBoostCpuBackend();

// GPU Backend (wenn CUDA/Vulkan verfügbar)
ISpatialComputeBackend* getGpuBackend();  // Stub
```

## Features

### Räumliche Operationen
- **Intersects** - Überschneidungsprüfung
- **Contains** - Enthält-Prüfung
- **Distance** - Distanzberechnung
- **Within** - Innerhalb-Prüfung

### Indexierung
- **R-Tree** - Räumlicher Index
- **S2 Cells** - Google S2 Geometrie
- **H3 Cells** - Uber H3 Hexagons
- **Geohash** - String-basierte Tiles

### GPU Acceleration
- **CUDA Backend** - NVIDIA GPUs (geplant)
- **Vulkan Backend** - Cross-Platform (geplant)
- **CPU Fallback** - Boost.Geometry

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
```

## Verwandte Dokumentation

- [geo_integration_readme.md](geo_integration_readme.md) - Integration Guide
- [geo_feature_tiering.md](geo_feature_tiering.md) - Feature Tiers
- [Performance: Geo](../performance/performance_geo.md) - Benchmarks
- [Future Enhancements](../../geospatial_future_enhancements.md) - Planned improvements for geospatial features
