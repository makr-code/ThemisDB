# Railway Monitor - WPF Desktop-Anwendung

Professionelle Desktop-Anwendung für Echtzeit-Überwachung des Zugverkehrs mit Energie-Management und Kraftwerkssteuerung.

## Features

### 🚂 Zugverkehr-Monitoring
- **Live-Karte** mit OpenStreetMap (Mapsui)
- **Echtzeit-Positionen** aller Züge (1 Hz Update)
- **Verspätungs-Analyse** mit KI (Ollama LLM)
- **Zugdetails** (Geschwindigkeit, Auslastung, Route)
- **CEP Alerts** (Anomalien, Störungen)

### ⚡ Energie-Management
- **Echtzeit-Stromverbrauch** aller Züge
- **Unterwerks-Auslastung** (~800 Unterwerke)
- **Kraftwerks-Steuerung** (Dispatch-Optimierung)
- **Lastprognose** (24h Vorhersage)
- **Grünstrom-Maximierung**
- **Kosten-Optimierung**

### 🏭 Kraftwerks-Integration
- **Wasserkraft**: 200 MW
- **Wind**: 150 MW
- **Solar**: 100 MW
- **Batterie**: 50 MW
- **Gas (Backup)**: 300 MW

### 📊 Visualisierung
- **Material Design UI** (modern & responsive)
- **Live-Charts** (LiveCharts2)
- **Energie-Dashboard** (Lastprofile, Power Mix)
- **KPI-Übersicht** (Pünktlichkeit, Auslastung, CO₂)

## Technologie

### .NET Stack
- **.NET 8.0** (Windows)
- **WPF** (Windows Presentation Foundation)
- **C# 12** (Latest)

### UI Libraries
- **MaterialDesignThemes** - Modern Material Design
- **MahApps.Metro** - Metro-Style Controls
- **Mapsui** - OpenStreetMap Integration
- **LiveChartsCore** - Interactive Charts

### MVVM & DI
- **CommunityToolkit.Mvvm** - MVVM Framework
- **Microsoft.Extensions.DependencyInjection**

### Backend Communication
- **Refit** - Type-safe REST Client
- **Websocket.Client** - WebSocket für Live-Updates
- **System.Text.Json** - JSON Serialization

### Logging
- **Serilog** - Structured Logging

## Installation

### Voraussetzungen

```
Windows 10/11 (x64)
.NET 8.0 SDK
Visual Studio 2022 (optional)
```

### Build

```powershell
# Clone Repository
git clone https://github.com/makr-code/ThemisDB
cd ThemisDB/clients/RailwayMonitor.WPF

# Restore NuGet Packages
dotnet restore

# Build
dotnet build -c Release

# Run
dotnet run -c Release
```

### Visual Studio

```
1. Öffne RailwayMonitor.WPF.sln
2. Build > Build Solution (Ctrl+Shift+B)
3. Debug > Start Without Debugging (Ctrl+F5)
```

## Konfiguration

### appsettings.json

```json
{
  "ThemisDB": {
    "Url": "http://localhost:8765",
    "Timeout": 5000
  },
  "Ollama": {
    "Url": "http://localhost:11434",
    "Model": "llama3.2:latest"
  },
  "Simulator": {
    "AutoStart": false,
    "TrainCount": 50,
    "UpdateInterval": 1000
  },
  "Energy": {
    "GridCapacityMw": 1000,
    "SubstationCount": 800,
    "RenewableTarget": 0.80
  },
  "Map": {
    "DefaultZoom": 6,
    "CenterLat": 51.1657,
    "CenterLon": 10.4515
  }
}
```

## Verwendung

### 1. Starten

```powershell
# ThemisDB muss laufen
docker run -p 8765:8765 themisdb/themisdb

# Simulator starten (optional)
cd scripts/railway
python train_simulator.py --network data/db_real_data.json --trains 50

# WPF App starten
cd clients/RailwayMonitor.WPF
dotnet run
```

### 2. Hauptfenster

```
┌─────────────────────────────────────────────────────────────┐
│ 🚂 Railway Monitoring System                               │
│                                                             │
│ [687]      [42]        [4.2 min]    [🟢 Verbunden]        │
│ Züge    Verspät.   Ø Verspät.     Status                  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────┐  ┌────────────────────┐  │
│  │                             │  │  Züge              │  │
│  │                             │  ├────────────────────┤  │
│  │      OpenStreetMap          │  │ 🔴 ICE 508  +3min │  │
│  │                             │  │ 🟠 IC 2314  +1min │  │
│  │      Live Train             │  │ 🔵 RE 4523   0min │  │
│  │      Positions              │  │ 🟢 RB 6745   0min │  │
│  │                             │  │                    │  │
│  │                             │  │  KI-Analyse        │  │
│  │                             │  ├────────────────────┤  │
│  └─────────────────────────────┘  │ "Warum hat ICE    │  │
│                                    │  508 Verspätung?" │  │
│  ┌──────────────┬──────────────┐  │                    │  │
│  │ Verspätungen │ Kategorien   │  │ [Analysieren]      │  │
│  │   Chart      │   Pie Chart  │  │                    │  │
│  └──────────────┴──────────────┘  └────────────────────┘  │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│ Status: System bereit  |  Last Update: 15:42:18  |  687 Updates │
└─────────────────────────────────────────────────────────────┘
```

### 3. Energie-Dashboard

```
Tabs: [Züge] [KI-Analyse] [Netzstatus] >> [⚡ Energie] <<

┌─────────────────────────────────────────────────────────────┐
│  Echtzeit Energieverbrauch                                  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Aktuelle Last: 687 MW         Grünstrom: 78.5%           │
│  Spitzenlast:   842 MW         CO₂: 137 kg/MWh            │
│  Auslastung:    68.7%          Kosten: 82.500 EUR/h       │
│                                                             │
│  ┌────────────────────────────────────────────────────┐   │
│  │         Lastprofil (24h)                           │   │
│  │  [LiveChart: Line Chart with Forecast]            │   │
│  └────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌────────────────────────────────────────────────────┐   │
│  │         Kraftwerks-Mix                             │   │
│  │  Wasserkraft:  ████████████████░░░░ 200 MW (100%) │   │
│  │  Wind:         ████████████░░░░░░░░ 150 MW (100%) │   │
│  │  Solar:        ████████░░░░░░░░░░░  80 MW  (80%)  │   │
│  │  Batterie:     ████░░░░░░░░░░░░░░░  50 MW (100%)  │   │
│  │  Gas:          █████████████░░░░░░ 207 MW  (69%)  │   │
│  └────────────────────────────────────────────────────┘   │
│                                                             │
│  Unterwerke (Top 5 Auslastung):                            │
│  1. UW Frankfurt Süd    ██████████████░░ 42.1 MW (93.5%)  │
│  2. UW München Ost      ███████████████░ 38.2 MW (84.8%)  │
│  3. UW Hamburg Nord     ███████████░░░░░ 34.5 MW (76.6%)  │
│                                                             │
│  [Für Kosten optimieren]  [Für CO₂ optimieren]            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Energie-Features

### Automatische Berechnung

```csharp
// Für jeden Zug wird automatisch berechnet:
var energyData = await _energyService.CalculateTrainEnergyAsync(train);

// Output:
{
    "InstantaneousPowerKw": 4200,    // Aktuelle Leistung
    "CumulativeEnergyKwh": 1523.5,   // Gesamtverbrauch
    "TractionPowerKw": 3800,         // Antrieb
    "AuxiliaryPowerKw": 400,         // Nebenverbraucher
    "RecuperationPowerKw": -300,     // Bremsenergierückgewinnung
    "EfficiencyPercent": 87.3        // Wirkungsgrad
}
```

### Kraftwerks-Optimierung

```csharp
// Merit-Order Prinzip (günstigste zuerst)
var dispatch = await _energyService.OptimizeDispatchAsync(
    demandMw: 687,
    optimizeFor: "cost"  // or "co2", "reliability"
);

// Ergebnis:
{
    "Allocations": {
        "hydro": 200,   // Vollast (günstigste)
        "wind": 150,    // Vollast
        "solar": 80,    // 80% (Tageszeit)
        "battery": 50,  // Vollast (Peak-Shaving)
        "gas": 207      // Rest (teuerste)
    },
    "TotalCostEur": 82500,
    "Co2KgPerMwh": 137,
    "RenewablePercent": 78.5
}
```

### Lastverschiebung

```csharp
// Güterverkehr in günstige Zeiten verschieben
var optimizedSchedule = await _energyService.OptimizeScheduleForEnergyAsync(
    freightTrains,
    targetRenewablePercent: 0.90
);

// Einsparung: ~16 Mio. EUR/Jahr
```

## Architektur

```
┌─────────────────────────────────────────────────────────┐
│                    WPF Application                      │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌───────────────┐  ┌──────────────┐  ┌─────────────┐ │
│  │   Views       │  │  ViewModels  │  │   Models    │ │
│  │  (XAML)       │→│   (MVVM)     │→│             │ │
│  └───────────────┘  └──────────────┘  └─────────────┘ │
│                             ↓                           │
│  ┌───────────────────────────────────────────────────┐ │
│  │              Services                             │ │
│  ├───────────────────────────────────────────────────┤ │
│  │  ThemisDbService  │  Energy  │  LLM  │  Map      │ │
│  │  (REST Client)    │  Mgmt    │       │           │ │
│  └───────────────────────────────────────────────────┘ │
│                             ↓                           │
└─────────────────────────────────────────────────────────┘
                              ↓
                    ┌─────────────────┐
                    │   ThemisDB      │
                    │   (Backend)     │
                    ├─────────────────┤
                    │  • Graph DB     │
                    │  • Time-Series  │
                    │  • Geo-Spatial  │
                    │  • CEP Engine   │
                    └─────────────────┘
```

## Testing

```powershell
# Unit Tests
dotnet test

# UI Tests (optional: mit TestStack.White)
# Manual Testing empfohlen
```

## Deployment

### ClickOnce

```powershell
# Publish als ClickOnce
dotnet publish -c Release -r win-x64 --self-contained

# Output: bin/Release/net8.0-windows/win-x64/publish/
```

### MSIX Package

```powershell
# Windows Store Package
msbuild /t:Publish /p:Configuration=Release /p:Platform=x64
```

## Troubleshooting

### Problem: Map lädt nicht

```
Lösung: Stelle sicher, dass Internet-Verbindung besteht
        OSM Tiles benötigen Internet-Zugriff
```

### Problem: ThemisDB Connection Failed

```
Lösung: 
1. Prüfe ob ThemisDB läuft: curl http://localhost:8765
2. Prüfe Firewall-Einstellungen
3. Prüfe appsettings.json URL
```

### Problem: Energie-Daten nicht sichtbar

```
Lösung:
1. Simulator muss laufen (generiert Energie-Daten)
2. Prüfe ob train_simulator.py mit --energy Flag gestartet
```

## Performance

```
Ressourcen (50 Züge):
- Memory: ~200 MB
- CPU: <10% (Intel i5)
- Network: ~100 KB/s (Updates)

Skalierung:
- Getestet: 500 Züge = ~400 MB RAM
- Max empfohlen: 1000 Züge
```

## Roadmap

- [ ] Offline-Modus (gecachte Daten)
- [ ] Export nach Excel/PDF
- [ ] Multi-Monitor Support
- [ ] Touch-Optimierung
- [ ] Dark/Light Theme Toggle
- [ ] Alarm-Konfiguration
- [ ] Benutzerverwaltung
- [ ] Historie-Replay

## Lizenz

MIT License

## Support

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: `docs/projects/`
