# Themis GIS Viewer - Entwicklungs-Roadmap

## Übersicht

Dieses Dokument beschreibt die Entwicklungs-Roadmap für das Themis GIS Viewer Projekt.

## Phase 1: Setup & Grundlagen (Wochen 1-3)

### Unreal Engine 5 Projekt
- [ ] Unreal Engine 5.4+ Projekt erstellen
- [ ] World Partition aktivieren
- [ ] Nanite/Lumen aktivieren
- [ ] Basic Map erstellen
- [ ] Camera Controller implementieren

### WPF Control Panel
- [x] Projekt-Struktur erstellen
- [x] MVVM Architektur aufsetzen
- [x] Services implementieren (IPC, ThemisDB)
- [ ] UI-Tests durchführen
- [ ] IPC-Kommunikation testen (Mock)

### ThemisDB Plugin (C++)
- [ ] Plugin-Gerüst erstellen
- [ ] HTTP Client implementieren (libcurl/cpp-httplib)
- [ ] Basic AQL Query Support
- [ ] Async Request Handling
- [ ] Blueprint Bindings

## Phase 2: OSM Integration (Wochen 4-7)

### OSM Importer Plugin
- [ ] LibOSM Integration
- [ ] OSM Parser implementieren
- [ ] Gebäude-Generator (Procedural Meshes)
  - [ ] Footprint Extrusion
  - [ ] Facade Material System
  - [ ] Roof Generation (flat, gabled, hipped)
  - [ ] Nanite Conversion
- [ ] Straßen-Generator (Spline-based)
- [ ] Terrain-Generator (DEM Support)

### ThemisDB Integration
- [ ] OSM → ThemisDB Import Pipeline
- [ ] Spatial Query Implementation
- [ ] Tile-based Streaming
- [ ] LOD Management

## Phase 3: Plugin Framework (Wochen 8-11)

### GIS Analysis Framework
- [ ] IAnalysisModule Interface (C++)
- [ ] Plugin Loader (DLL Loading)
- [ ] Module Registry
- [ ] Blueprint Integration
- [ ] Parameter System

### WPF ↔ Unreal Communication
- [ ] Named Pipes Server (Unreal)
- [ ] Command Protocol definieren
- [ ] Real-time Data Streaming
- [ ] Error Handling

## Phase 4: Analysis Modules (Wochen 12-16)

### Wind Simulation Module
- [ ] Niagara Particle System Setup
- [ ] CFD Grid implementieren
- [ ] Compute Shader für Wind-Simulation
- [ ] Obstacle Detection (Buildings)
- [ ] Visualization Modes (Particles, Vectors, Heatmap)
- [ ] WPF Parameter Controls

### Water Flow Module
- [ ] Shallow Water Equations (GPU)
- [ ] Terrain Heightmap Integration
- [ ] Rainfall Simulation
- [ ] Unreal Water Plugin Integration
- [ ] Flood Zone Visualization
- [ ] WPF Controls

### Sound Propagation Module
- [ ] MetaSounds Integration
- [ ] Acoustic Ray-Tracing
- [ ] Building Occlusion
- [ ] Noise Map Generation
- [ ] Decibel Visualization
- [ ] WPF Controls

### Disaster Simulation Module
- [ ] Chaos Destruction Setup
- [ ] Earthquake Simulation (Ground Motion)
- [ ] Building Vulnerability Assessment
- [ ] Fire Propagation (Niagara)
- [ ] Flood Simulation (Water Module)
- [ ] Damage Analysis
- [ ] WPF Controls & Statistics

## Phase 5: Polish & Optimization (Wochen 17-20)

### Performance
- [ ] Profiling (Unreal Insights)
- [ ] Nanite Optimization
- [ ] World Partition Tuning
- [ ] Async Loading Optimierung
- [ ] GPU Culling Verification
- [ ] Memory Profiling

### UI/UX
- [ ] WPF UI Polishing
- [ ] Theme Consistency
- [ ] Error Messages & Tooltips
- [ ] Loading Indicators
- [ ] Progress Bars
- [ ] Export Functions (Screenshots, Data)

### Dokumentation
- [ ] User Manual (Deutsch)
- [ ] Developer Documentation (C++/Blueprint)
- [ ] API Reference
- [ ] Tutorial Videos
- [ ] Sample Datasets (Berlin, Hamburg)
- [ ] Plugin Development Guide

### Testing
- [ ] Unit Tests (C++ Plugins)
- [ ] Integration Tests (Unreal ↔ WPF)
- [ ] Performance Benchmarks
- [ ] End-to-End Scenarios
- [ ] Bug Fixes

### Release
- [ ] Packaging (Windows)
- [ ] Installer erstellen
- [ ] Demo Video (YouTube)
- [ ] GitHub Release v1.0
- [ ] Documentation Website

## Technische Entscheidungen

### Bereits getroffen
- ✅ Unreal Engine 5 als Rendering Engine
- ✅ WPF für Control Panel
- ✅ Named Pipes für IPC
- ✅ ThemisDB für Geodaten-Storage
- ✅ C++ Plugins für Analysen

### Offen
- [ ] Exaktes OSM Parser Library (LibOSM vs. eigene Impl.)
- [ ] Terrain Provider (DEM Format: GeoTIFF, HGT?)
- [ ] 3D-Model Format (glTF, FBX, beide?)
- [ ] Compute Shader Language (HLSL, GLSL?)
- [ ] Packaging Tool (UE5 Standard vs. Custom)

## Meilensteine

| Phase | Dauer | Deliverable |
|-------|-------|-------------|
| Phase 1 | 3 Wochen | Funktionsfähiges Setup (UE5 + WPF + IPC) |
| Phase 2 | 4 Wochen | OSM Import & Visualisierung (Berlin Demo) |
| Phase 3 | 4 Wochen | Plugin Framework & erste Module |
| Phase 4 | 5 Wochen | Alle 4 Analysis Modules funktionsfähig |
| Phase 5 | 4 Wochen | Release v1.0 (poliert, dokumentiert) |
| **Gesamt** | **20 Wochen** | **Production-Ready GIS Viewer** |

## Risiken & Mitigation

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|------------|
| Unreal Engine Performance-Probleme | Mittel | Hoch | Frühes Profiling, Nanite/Lumen optimieren |
| IPC-Latenz zu hoch | Niedrig | Mittel | Benchmarking, ggf. UDP statt Named Pipes |
| OSM-Parsing zu langsam | Mittel | Mittel | Background-Threading, Caching |
| ThemisDB Skalierung | Niedrig | Hoch | Spatial Index testen, Batch-Requests |
| Compute Shader Komplexität | Hoch | Mittel | Simplified CFD, ggf. Pre-Computed Data |

## Nächste Schritte

1. ✅ Konzept erstellen (GIS_VIEWER_CONCEPT.md)
2. ✅ WPF Control Panel Grundgerüst
3. **TODO**: Unreal Engine 5 Projekt anlegen
4. **TODO**: ThemisDB Plugin (C++) Grundgerüst
5. **TODO**: IPC Test (Mock Communication)

---

**Stand**:  6. April 2026
**Version**: 0.1 (Konzeptphase)  
**Verantwortlich**: ThemisDB Team
