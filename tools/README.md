# ThemisDB Admin Tools

## Übersicht

Die ThemisDB Admin Tools sind eine Suite von Windows-Desktop-Anwendungen und Python-Tools zur Verwaltung, Überwachung, Analyse und Compliance-Prüfung der ThemisDB-Datenbank.

## Python-Tools

### Namespace Analyzer (`namespace_analyzer.py`)

Ein Python-Tool zur umfassenden Analyse der ThemisDB-Codebasis. Extrahiert und dokumentiert:
- Namespaces und ihre Hierarchien
- Klassen, Structs und Enums innerhalb jeder Namespace
- Funktionen und ihre Signaturen
- Variablen und Konstanten
- Zeitliche Informationen (wann jede Entität eingeführt/geändert wurde) via Git-Metadaten

**Verwendung:**
```bash
# Grundlegende Analyse (alle Formate)
python3 tools/namespace_analyzer.py

# Mit Git-Metadaten (langsamer)
python3 tools/namespace_analyzer.py --include-git

# Nur Markdown-Bericht
python3 tools/namespace_analyzer.py --format markdown
```

**Ausgabeformate:**
- JSON (`namespace_analysis.json`) - Strukturierte Daten für maschinelle Verarbeitung
- Markdown (`namespace_analysis.md`) - Menschenlesbarer Bericht
- CSV (`namespaces.csv`, `classes.csv`, `functions.csv`) - Tabellarische Daten

**Dokumentation:** Siehe [NAMESPACE_ANALYZER_README.md](NAMESPACE_ANALYZER_README.md)

## .NET Desktop-Anwendungen

### Themis.AdminTools.Shared
Gemeinsam genutzte Bibliothek mit:
- **ThemisApiClient**: HTTP-Client für themis_server REST API
- **Modelle**: DTOs für Audit-Logs, Konfiguration, API-Antworten
- **Utilities**: Wiederverwendbare Hilfsfunktionen

### Themis.AqlQueryBuilder
Visueller Query Builder/Editor für AQL (Advanced Query Language).

**Features:**
- Visuelle Konstruktion von AQL-Queries ohne Code-Schreiben
- FOR, LET, FILTER, SORT, LIMIT, RETURN Klauseln
- Echtzeit-Query-Vorschau
- Query-Ausführung gegen Themis Server
- Beispiel-Queries zum Lernen
- MVVM-Architektur mit OOP Best Practices

### Themis.AuditLogViewer
WPF-Anwendung zur Anzeige und Analyse von Audit-Logs.

**Features:**
- Zeitbereichsfilter (Von/Bis-Datum)
- Benutzerfilter
- Aktionsfilter
- Entitätstypfilter
- Nur erfolgreiche Aktionen anzeigen
- Seitenweise Navigation (100 Einträge pro Seite)
- CSV-Export
- Moderne WPF-UI mit DataGrid

## Voraussetzungen

- .NET 8 SDK
- Visual Studio 2022 oder VS Code mit C# Dev Kit
- Zugriff auf laufenden themis_server (Standard: http://localhost:8080)

## Installation

```powershell
cd tools
dotnet restore
dotnet build
```

## Konfiguration

Bearbeiten Sie `Themis.AuditLogViewer/appsettings.json`:

```json
{
  "ThemisServer": {
    "BaseUrl": "http://localhost:8080",
    "ApiKey": "",
    "Timeout": 30
  }
}
```

## Ausführen

```powershell
cd Themis.AuditLogViewer
dotnet run
```

## API-Anforderungen

Der themis_server muss folgende Endpunkte bereitstellen:

### GET /api/audit
Query-Parameter:
- `start` (ISO 8601 DateTime)
- `end` (ISO 8601 DateTime)
- `user` (string)
- `action` (string)
- `entity_type` (string)
- `entity_id` (string)
- `success` (boolean)
- `page` (int)
- `page_size` (int)

Antwort:
```json
{
  "entries": [...],
  "totalCount": 1234,
  "page": 1,
  "pageSize": 100,
  "hasMore": true
}
```

### GET /api/audit/export/csv
Gleiche Query-Parameter, gibt CSV-Datei zurück.

## Entwicklung

**Architektur:**
- MVVM-Pattern mit CommunityToolkit.Mvvm
- Dependency Injection (Microsoft.Extensions.DependencyInjection)
- Async/Await für API-Calls
- INotifyPropertyChanged für Data Binding

**Nächste Schritte:**
1. themis_server API-Endpunkte implementieren (C++)
2. Authentifizierung hinzufügen (JWT/API-Key)
3. Weitere Tools entwickeln (siehe tool-todo.md)
4. Deployment-Pipeline einrichten

## Lizenz

Siehe Hauptprojekt-Lizenz.
