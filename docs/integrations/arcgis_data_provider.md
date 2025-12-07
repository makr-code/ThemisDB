# ArcGIS Data Provider Integration

## Übersicht

Das ThemisDB ArcGIS Data Provider Plugin ermöglicht es ArcGIS-Anwendungen, auf geospatiale Daten in ThemisDB zuzugreifen. ThemisDB fungiert als **Datenquelle** für ArcGIS, nicht umgekehrt.

## Kernfunktionalität

### 3D Geospatial Support

Das Data Provider unterstützt vollständige 3D-Geometrien (Point(x, y, z)) mit z=0 als Fallback für 2D-Daten.

### Anwendungsfälle

- **Umweltrisikobewertung** (Hochwasser, Dürre)
- **Störfall-Kaskadeneffekt** (12. BImSchV)
- **3D Terrain Analysis**
- **Multi-Model Integration** (Graph + Geo + Time-Series)

## Weitere Informationen

Siehe vollständige Spezifikation in `include/geo/arcgis_data_provider.h`.
