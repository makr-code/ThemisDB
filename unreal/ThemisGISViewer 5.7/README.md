# Themis GIS Viewer - Unreal Engine 5 Project

## Übersicht

Dieses Unreal Engine 5 Projekt ist Teil des Themis GIS Viewers - ein hochperformantes GIS-Tool mit Echtzeit-Analysen und photorealistischer Visualisierung.

## Systemanforderungen

### Minimum
- **Unreal Engine**: 5.4 oder höher
- **OS**: Windows 10/11 (64-bit)
- **CPU**: Intel i7 / AMD Ryzen 7 (8+ Cores)
- **RAM**: 16 GB
- **GPU**: NVIDIA GTX 1660 / AMD RX 5600 (6+ GB VRAM)
- **Speicher**: 50 GB SSD

### Empfohlen
- **CPU**: Intel i9 / AMD Ryzen 9 (12+ Cores)
- **RAM**: 32 GB
- **GPU**: NVIDIA RTX 3070 / AMD RX 6800 (8+ GB VRAM) mit Nanite/Lumen Support
- **Speicher**: 100 GB NVMe SSD

## Features

- ✅ **Nanite**: Virtualisierte Geometrie für Millionen von Polygonen
- ✅ **Lumen**: Dynamische Global Illumination
- ✅ **World Partition**: Streaming Open-World (100+ km²)
- ✅ **Niagara**: VFX für Wind/Regen/Partikel-Simulationen
- ✅ **Chaos Physics**: Destruktion und Physik-Simulationen
- ✅ **MetaSounds**: 3D Audio für Schallausbreitung

## Projekt-Struktur

```
ThemisGISViewer/
├── Config/
│   ├── DefaultEngine.ini    # Engine-Konfiguration (Nanite, Lumen, World Partition)
│   └── DefaultGame.ini       # Projekt-Einstellungen
├── Content/
│   ├── Maps/                 # Level (MainWorld mit World Partition)
│   ├── Materials/            # PBR Materials (Gebäude, Straßen, Terrain)
│   ├── Blueprints/          # Gameplay-Logik
│   ├── VFX/                 # Niagara Systems (Wind, Regen)
│   └── Audio/               # MetaSounds (Stadt-Ambiente, Verkehr)
├── Plugins/
│   ├── ThemisDBPlugin/      # ThemisDB REST API Integration (C++)
│   ├── OSMImporterPlugin/   # OSM Import & Procedural Generation (C++)
│   └── GISAnalysisFramework/ # Plugin-System für Analysen (C++)
├── Source/
│   └── ThemisGISViewer/     # C++ Game Module
└── ThemisGISViewer.uproject # Projekt-Datei
```

## Plugins

### ThemisDBPlugin (C++)

**Zweck**: Integration mit ThemisDB Server für Geodaten-Queries

**Features**:
- HTTP Client für ThemisDB REST API
- Async Queries (Buildings, Terrain, AQL)
- Blueprint-Bindings für einfache Nutzung
- Geo-Koordinaten ↔ Unreal World Transformation

**Beispiel (Blueprint)**:
```cpp
// Query buildings in bounding box
ThemisDBClient->QueryBuildingsAsync(
    SouthWest: (Lat: 52.5, Lon: 13.4),
    NorthEast: (Lat: 52.55, Lon: 13.45),
    OnComplete: HandleBuildingsReceived
);
```

### OSMImporterPlugin (TODO)

**Zweck**: Import und Visualisierung von OpenStreetMap-Daten

**Features** (geplant):
- OSM Parser (LibOSM Integration)
- Procedural Building Generation (Extrusion)
- Road Network (Spline-based)
- Terrain Generation (DEM)

### GISAnalysisFramework (TODO)

**Zweck**: Plugin-System für Analyse-Module

**Features** (geplant):
- IAnalysisModule Interface (C++)
- DLL Plugin Loader
- Parameter-System
- Visualisierungs-Layer

## Installation

### 1. Unreal Engine installieren

```bash
# Epic Games Launcher installieren
# Unreal Engine 5.4+ über Launcher installieren
```

### 2. Projekt öffnen

```bash
# Im Epic Games Launcher:
# "Durchsuchen" → ThemisGISViewer.uproject auswählen
```

### 3. C++ Module kompilieren

```bash
# Beim ersten Öffnen kompiliert Unreal automatisch
# Oder manuell in Visual Studio:
# 1. Rechtsklick auf .uproject → "Generate Visual Studio project files"
# 2. ThemisGISViewer.sln öffnen
# 3. Build → Build Solution (Development Editor)
```

### 4. ThemisDB Server starten

```powershell
# In separatem Terminal
cd /path/to/ThemisDB
.\build\Release\themis_server.exe --config config.yaml
```

### 5. WPF Control Panel starten (optional)

```powershell
cd tools/Themis.GISViewer.ControlPanel
dotnet run
```

## Konfiguration

### DefaultEngine.ini

Wichtige Einstellungen für GIS-Anwendungen:

```ini
[ThemisDB]
ServerURL=http://localhost:8765
ConnectionTimeout=30
EnableCaching=True

[/Script/Engine.WorldPartitionSettings]
bEnableWorldPartition=True
WorldPartitionGridSize=25600  # 256m Tiles

[/Script/Engine.RendererSettings]
r.Nanite=True                  # Nanite aktivieren
r.Lumen.DiffuseIndirect=True  # Lumen GI aktivieren
```

### Named Pipes (IPC mit WPF)

```ini
[UnrealIPC]
PipeName=ThemisGISViewer_IPC
BufferSize=65536
```

## Erste Schritte

### 1. Test-Level öffnen

```
Content/Maps/MainWorld.umap
```

### 2. ThemisDB Client testen (Blueprint)

```cpp
// Im Level Blueprint:
Event BeginPlay
  → Create ThemisDBClient
  → Initialize (ServerURL: "http://localhost:8765")
  → Query Buildings (Berlin Mitte)
  → Für jedes Gebäude: Spawn ProceduralBuilding Actor
```

### 3. Kamera steuern

- **WASD**: Bewegung
- **Maus**: Umsehen
- **Q/E**: Hoch/Runter
- **Shift**: Schneller

## Development Workflow

### C++ Plugin entwickeln

```bash
# 1. Neue C++ Klasse in Plugin erstellen
#    Unreal Editor → Tools → New C++ Class

# 2. Code bearbeiten
#    Visual Studio → Edit .h/.cpp

# 3. Hot Reload
#    Unreal Editor → Compile Button (oder Ctrl+Alt+F11)
```

### Blueprint Logic

```
Content/Blueprints/
  BP_GISWorldManager    # World Streaming & Tile Loading
  BP_CameraController   # Kamera-Steuerung
  BP_AnalysisVisualizer # Analyse-Visualisierung (Heatmaps, etc.)
```

## Bekannte Probleme

1. **Erste Kompilierung dauert lange** (10-30 Min)
   - Normal für Unreal Engine 5
   - Danach sind Incremental Builds schneller (1-2 Min)

2. **Nanite erfordert RTX/RDNA2 GPU**
   - Fallback auf Standard-Rendering wenn nicht verfügbar
   - Performance kann sinken

3. **World Partition benötigt viel RAM**
   - Minimum 16 GB für kleine Städte
   - 32 GB empfohlen für große Gebiete

## Debugging

### ThemisDB Plugin

```cpp
// In ThemisDBClient.cpp:
UE_LOG(LogTemp, Log, TEXT("Query sent to: %s"), *BaseURL);

// Output Window in Unreal Editor:
// Window → Developer Tools → Output Log
```

### Performance Profiling

```
# Unreal Insights
# Tools → Session Frontend → Profiler
```

## Roadmap

Siehe: [docs/tools/GIS_VIEWER_ROADMAP.md](../../../docs/tools/GIS_VIEWER_ROADMAP.md)

- [x] Phase 1: Setup & Grundlagen (ThemisDB Plugin)
- [ ] Phase 2: OSM Integration
- [ ] Phase 3: Plugin Framework
- [ ] Phase 4: Analysis Modules (Wind, Water, Sound, Disaster)
- [ ] Phase 5: Polish & Optimization

## Ressourcen

- **Unreal Engine Documentation**: https://docs.unrealengine.com/5.4/
- **Nanite**: https://docs.unrealengine.com/5.4/nanite/
- **Lumen**: https://docs.unrealengine.com/5.4/lumen/
- **World Partition**: https://docs.unrealengine.com/5.4/world-partition/
- **ThemisDB Docs**: [../../docs/](../../docs/)

## Support

Für Fragen und Issues:
- GitHub: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: [docs/tools/GIS_VIEWER_CONCEPT.md](../../../docs/tools/GIS_VIEWER_CONCEPT.md)

---

**Version**: 0.1.0  
**Status**: 🚧 In Development  
**Last Updated**: Dezember 2024
