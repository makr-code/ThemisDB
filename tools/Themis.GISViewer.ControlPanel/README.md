> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Themis.GISViewer.ControlPanel

## Übersicht

WPF Control Panel für Themis GIS Viewer (Unreal Engine 5).

## Features

- **Unreal Engine Kommunikation**: Named Pipes IPC
- **ThemisDB Integration**: REST API Client
- **Plugin Management**: Laden und Verwalten von Analysis Modules
- **Simulations-Steuerung**:
  - Wind-Simulation (Geschwindigkeit, Richtung, Visualisierung)
  - Wasser-Simulation (Niederschlag, Dauer)
  - Katastrophen-Simulation (Erdbeben, Hochwasser, Feuer)

## Verwendung

1. Unreal Engine 5 Projekt starten (ThemisGISViewer)
2. Control Panel starten
3. "Verbinden" Button klicken
4. Parameter einstellen und Simulationen starten

## Konfiguration

Siehe `appsettings.json`:

```json
{
  "ThemisDB": {
    "ApiUrl": "http://localhost:8765"
  },
  "UnrealEngine": {
    "PipeName": "ThemisGISViewer_IPC"
  }
}
```

## Architektur

- **Services**: UnrealEngineConnector, ThemisDBService, PluginService
- **ViewModels**: MVVM Pattern mit CommunityToolkit.Mvvm
- **Views**: WPF XAML mit Themis-Styling

## Dependencies

- .NET 8.0
- WPF
- CommunityToolkit.Mvvm
- Themis.AdminTools.Shared

## Status

🚧 **In Entwicklung** - Siehe [GIS_VIEWER_CONCEPT.md](../../docs/tools/GIS_VIEWER_CONCEPT.md) für Details.
