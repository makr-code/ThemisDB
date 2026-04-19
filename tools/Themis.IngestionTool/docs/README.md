> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Ingestion Tool

## Übersicht

Das **ThemisDB Ingestion Tool** ist eine moderne WPF-Anwendung zur effizienten Datenaufnahme und Analyse von Dateien für die ThemisDB Multi-Model-Datenbank. Es kombiniert fortschrittliche Analyse-Technologien (NLP, LLM, Graph-Analyse) mit einer benutzerfreundlichen grafischen Oberfläche.

## Hauptfunktionen

### 🔍 Erweiterte Datei-Analyse
- **NLP-basierte Metadaten-Extraktion**
  - Keyword-Extraktion (Häufigkeitsanalyse)
  - Named Entity Recognition
  - Topic Modeling
  - Sprach-Erkennung (Deutsch/Englisch)
  
- **Graph-Analyse**
  - Automatische Erkennung von Code-Strukturen (Klassen, Funktionen)
  - Beziehungs-Analyse (Imports, Referenzen)
  - Impact-Score-Berechnung
  
- **Quality Metrics**
  - Relevanz-Score (0-1)
  - Impact-Score (0-1)
  - Quality-Score (0-1)
  - Graph-Knoten-Zählung
  - Beziehungs-Zählung

### 📊 Live-Metriken-Dashboard
- Echtzeit-Anzeige aller Analyse-Ergebnisse
- DataGrid mit Spalten: Datei, Relevanz, Impact, Qualität, Knoten, Beziehungen, Sprache
- Fortschrittsanzeige mit Prozentsatz
- Stage-basiertes Pipeline-Tracking

### 🌐 ThemisDB-Integration
- Automatische Verbindungsprüfung (Heartbeat alle 5 Sekunden)
- Online/Offline-Statusanzeige
- Health-Check gegen `/health` Endpoint
- Port 8765 (konfigurierbar)

### 🔄 DryRun-Modus
- Vollständige Analyse ohne DB-Schreibzugriff
- Ideal für Testing und Evaluation
- Exportiert JSON-Ergebnisse

### 💾 Persistente Einstellungen
- Automatisches Speichern der letzten Konfiguration
- Themis-Host und Port
- Quellordner und Ausgabedatei
- Metadaten-Optionen

## Schnellstart

### Voraussetzungen
- .NET 8.0 Runtime
- Windows 10/11
- ThemisDB-Server (optional für DryRun)

### Installation
1. Öffnen Sie das Projekt in Visual Studio oder VS Code
2. Führen Sie aus:
   ```powershell
   dotnet build -c Release
   ```
3. Starten Sie:
   ```powershell
   .\bin\Release\net8.0-windows\Themis.IngestionTool.exe
   ```

### Erste Schritte
1. **Themis-Verbindung prüfen**: Statusbar zeigt Online/Offline
2. **Quellordner auswählen**: Klicken Sie auf "..." neben dem Textfeld
3. **DryRun aktivieren** (optional): Checkbox aktivieren für Test-Modus
4. **Start**: Pipeline beginnt mit Datei-Sammlung und Analyse
5. **Live-Ergebnisse**: Rechte Seite zeigt Metriken in Echtzeit

## Unterstützte Dateitypen
- `.cs`, `.java`, `.py`, `.js`, `.ts`, `.cpp`, `.h`
- `.txt`, `.md`
- `.json`, `.xml`, `.yaml`, `.yml`
- `.sql`

## Architektur

Das Tool folgt dem **MVVM-Pattern** mit Dependency Injection:

```
┌─────────────────────────────────────────────────┐
│              WPF Application                    │
├─────────────────────────────────────────────────┤
│  Views         │ ViewModels      │ Models       │
│  - MainWindow  │ - MainWindowVM  │ - AppSettings│
│  - Settings    │ - SettingsVM    │ - FileResult │
├─────────────────────────────────────────────────┤
│                  Services                       │
│  - IngestionPipelineService                     │
│  - LlamaService (LLM-Integration)               │
│  - NlpAnalysisService                           │
│  - GraphAnalysisService                         │
│  - ThemisConnectionService                      │
│  - SettingsService                              │
├─────────────────────────────────────────────────┤
│              ThemisDB API                       │
│  - POST /entities                               │
│  - POST /graph/traverse                         │
│  - POST /vector/search                          │
└─────────────────────────────────────────────────┘
```

## Dokumentation

- [Architektur-Details](ARCHITECTURE.md)
- [Benutzerhandbuch](USER_GUIDE.md)
- [API-Integration](API_INTEGRATION.md)
- [Analytics-Pipeline](ANALYTICS_PIPELINE.md)
- [Konfiguration](CONFIGURATION.md)

## Technologie-Stack

- **Framework**: .NET 8.0 / WPF
- **Pattern**: MVVM
- **DI**: Microsoft.Extensions.DependencyInjection
- **UI-Framework**: Windows Presentation Foundation
- **Serialisierung**: System.Text.Json, YamlDotNet
- **Datenbank**: System.Data.SQLite (Tracking)

## Best Practices

### Für Entwickler
- Folgen Sie dem MVVM-Pattern strikt
- Nutzen Sie async/await für alle I/O-Operationen
- Implementieren Sie IProgress für lange Operationen
- Verwenden Sie CancellationToken für abbrechbare Tasks

### Für Benutzer
- Starten Sie mit DryRun für große Ordner
- Prüfen Sie die Themis-Verbindung vor der Ingestion
- Exportieren Sie JSON-Ergebnisse für spätere Analyse
- Überwachen Sie die Live-Metriken

## Lizenz

Copyright © 2026 ThemisDB Project

## Support

Bei Problemen oder Fragen:
- GitHub Issues: [themisdb/themis](https://github.com/themisdb/themis)
- Dokumentation: [themisdb.org/docs](https://themisdb.org/docs)
