# ArcGIS Data Provider Integration (Enterprise Feature)

## Übersicht

Das ThemisDB ArcGIS Data Provider Plugin ermöglicht es ArcGIS-Anwendungen, auf geospatiale Daten in ThemisDB zuzugreifen. ThemisDB fungiert als **Datenquelle** für ArcGIS, nicht umgekehrt.

**Enterprise Feature**: Der ArcGIS Data Provider ist als Enterprise-Plugin verfügbar.

## Kernfunktionalität

### 3D Geospatial Support (Core Feature)

Das Data Provider nutzt die Core-3D-Geometrie-Unterstützung (Point(x, y, z)) mit z=0 als Fallback für 2D-Daten.

### Anwendungsfälle

- **Umweltrisikobewertung** (Hochwasser, Dürre) - Enterprise Feature
- **Störfall-Kaskadeneffekt** (12. BImSchV) - Enterprise Feature
- **3D Terrain Analysis** - Core Feature
- **Multi-Model Integration** (Graph + Geo + Time-Series) - Enterprise Feature

## Weitere Informationen

Siehe vollständige Spezifikation in `include/enterprise/arcgis_data_provider.h` und `plugins/enterprise/arcgis_data_provider/`.
