# ThemisDB Admin Tools

> 📚 **Complete Documentation:** See [Tools & Utilities Index](../docs/TOOLS_INDEX.md) for comprehensive documentation of all 30+ tools.

## Übersicht

Die ThemisDB Admin Tools sind eine Suite von Windows-Desktop-Anwendungen und Python-Tools zur Verwaltung, Überwachung, Analyse und Compliance-Prüfung der ThemisDB-Datenbank.

## Quick Links

- **[📖 Tools Index](../docs/TOOLS_INDEX.md)** - Complete catalog of all tools with documentation links
- **[🔧 Admin Tools](../docs/TOOLS_INDEX.md#-administration-tools)** - Desktop applications (.NET)
- **[🔄 Operations Tools](../docs/TOOLS_INDEX.md#-operations--monitoring)** - Scripts for operations and monitoring
- **[📦 Ingestion Tools](../docs/TOOLS_INDEX.md#-data-ingestion)** - Data import utilities
- **[💻 Development Tools](../docs/TOOLS_INDEX.md#-development-utilities)** - Developer utilities

## Python-Tools

### Ingestion Tool (`ingest.py`)

Ein autonomes Werkzeug zur rekursiven Durchsuchung von Verzeichnissen nach ingestierbaren Dateien und deren Aufbereitung für ThemisDB.

**Features:**
- Rekursive Verzeichnisdurchsuchung mit konfigurierbaren Filtern
- Hash-basierte Duplikaterkennung (SHA256) - bereits ingestierte Dateien werden übersprungen
- Fortschrittsanzeige mit Progress Bar (tqdm)
- Detailliertes Logging in `ingestion.log`
- SQLite-Tracking-Datenbank für verarbeitete Dateien
- Metadatenextraktion für ThemisDB-Modelle:
  - **Graph**: Entitäten, Beziehungen, Properties
  - **Vector**: Text-Content für Embeddings, semantische Suche
  - **Relational**: Schema, Datensätze, Feldtypen
- Unterstützung für JSON, YAML, CSV, Text-Dateien
- Konfiguration über YAML/JSON oder Kommandozeile

**Verwendung:**
```bash
# Grundlegende Verwendung
python3 tools/ingest.py --source /path/to/data

# Mit Konfigurationsdatei
python3 tools/ingest.py --config tools/ingest_config.example.yaml

# Mit benutzerdefinierten Optionen
python3 tools/ingest.py --source /path/to/data \
    --output results.json \
    --include-ext .json .yaml .txt \
    --max-size 50

# Nur bestimmte Modelle aktivieren
python3 tools/ingest.py --source /path/to/data --no-relational

# Verbose Modus für detailliertes Logging
python3 tools/ingest.py --source /path/to/data --verbose
```

**Ausgaben:**
- `ingestion_output.json` - Detaillierte Metadaten aller ingestierten Dateien
- `ingestion_tracker.db` - SQLite-Datenbank mit Hash-Tracking
- `ingestion.log` - Logdatei mit allen Ereignissen

**Konfigurationsdatei:**
Siehe `ingest_config.example.yaml` für ein vollständiges Beispiel.

**Integration mit ThemisDB:**
Das Tool generiert Metadaten im Format, das mit ThemisDB's BaseEntity und Importer Interface kompatibel ist. Die generierten JSON-Daten können direkt in ThemisDB importiert werden.

**Voraussetzungen:**
- Python 3.8+
- Optional: `pyyaml` für YAML-Konfiguration (`pip install pyyaml`)
- Optional: `tqdm` für Progress Bar (`pip install tqdm`)

### Ingestion Tool - C# Version (`Themis.IngestionTool`)

Eine C# .NET Console-Anwendung mit den gleichen Features wie das Python-Tool, aber mit Integration in die Themis.AdminTools.Shared Bibliothek.

**Features:**
- Alle Features des Python-Tools
- Integration mit Themis.AdminTools.Shared
- System.CommandLine für CLI
- Microsoft.Extensions.Logging für strukturiertes Logging
- SQLite mit System.Data.SQLite
- YamlDotNet für YAML-Unterstützung

**Verwendung:**
```bash
cd Themis.IngestionTool
dotnet build

# Grundlegende Verwendung
dotnet run -- --source /path/to/data

# Mit Optionen
dotnet run -- --source /path/to/data --output results.json --verbose
```

**Voraussetzungen:**
- .NET 8.0 SDK
- Windows, Linux oder macOS

**Dokumentation:** Siehe [Themis.IngestionTool/README.md](Themis.IngestionTool/README.md)



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

## Web-Tools

### TCO Calculator (`tco-calculator-wordpress`)

Ein WordPress-Plugin für den Total Cost of Ownership (TCO) Rechner für ThemisDB.

**Features:**
- Shortcode-basierte Einbindung: `[themisdb_tco_calculator]`
- Admin-Einstellungsseite für Standardwerte
- Vollständige TCO-Analyse (Infrastruktur, Personal, Lizenzen, Betrieb, AI/LLM)
- Interaktive Visualisierungen mit Chart.js
- Export-Funktionen (PDF, CSV)
- WordPress-optimiert und theme-kompatibel

**Verwendung:**
```php
// In WordPress Seite/Post
[themisdb_tco_calculator]

// Mit Parametern
[themisdb_tco_calculator show_intro="no" title="Kostenrechner"]
```

**Installation:**
```bash
# In WordPress plugins-Verzeichnis kopieren
cp -r tco-calculator-wordpress /path/to/wordpress/wp-content/plugins/themisdb-tco-calculator/

# In WordPress Admin aktivieren
# Plugins → ThemisDB TCO Calculator → Aktivieren
```

**Dokumentation:**
- [README](tco-calculator-wordpress/README.md)
- [Quickstart](tco-calculator-wordpress/QUICKSTART.md)
- [Installation](tco-calculator-wordpress/INSTALLATION.md)
- [Implementation Guide](tco-calculator-wordpress/IMPLEMENTATION.md)

---

### Wiki Integration (`themisdb-wiki-integration`)

Ein WordPress-Plugin zur **automatischen Integration der ThemisDB-Dokumentation aus GitHub**.

**Features:**
- ✅ Automatisches Abrufen von Markdown-Dateien aus GitHub
- ✅ Unterstützung für mehrere Sprachen (DE, EN, FR)
- ✅ Caching-Mechanismus für Performance
- ✅ Automatische stündliche Synchronisierung
- ✅ Inhaltsverzeichnis-Generierung
- ✅ Responsive Design mit Dark Mode Support
- ✅ Admin-Panel zur Konfiguration

**Verwendung:**
```php
// Dokumentation anzeigen
[themisdb_wiki file="README.md" lang="de" show_toc="yes"]

// Dokumentationsliste anzeigen
[themisdb_docs lang="de" layout="grid"]

// Beispiele
[themisdb_wiki file="features/FEATURES.md" lang="de" show_toc="yes"]
[themisdb_wiki file="architecture/ARCHITECTURE.md" lang="en"]
```

**Installation:**
```bash
# In WordPress plugins-Verzeichnis kopieren
cp -r themisdb-wiki-integration /path/to/wordpress/wp-content/plugins/

# In WordPress Admin aktivieren
# Plugins → ThemisDB Wiki Integration → Aktivieren
```

**Konfiguration:**
- Gehen Sie zu **Einstellungen → ThemisDB Wiki**
- GitHub Repository: `makr-code/ThemisDB`
- Branch: `main`
- Dokumentationspfad: `docs`
- Optional: GitHub Token für höhere API-Limits

**Dokumentation:**
- [README](themisdb-wiki-integration/README.md)
- [Quick Start](themisdb-wiki-integration/QUICKSTART.md)
- [Installation Guide](themisdb-wiki-integration/INSTALLATION.md)

### ThemisDB-spezifische WordPress Plugins (Konzept)

**Spezialisierte Plugins zur Visualisierung von ThemisDB-Daten** (Benchmarks, Features, Tests, Docs).

**Dokumentation:**
- [Vollständiges Konzept (DE)](../docs/de/tools/THEMISDB_WORDPRESS_PLUGINS_KONZEPT.md) - 7 Plugin-Ideen mit Roadmap
- [Concept (EN)](../docs/en/tools/THEMISDB_WORDPRESS_PLUGINS_CONCEPT.md) - Plugin concepts and prioritization

**Vorgeschlagene Plugins:**
1. **Benchmark Visualizer** (Prio: Hoch) - Performance-Vergleiche interaktiv
2. **Live Query Playground** (Prio: Hoch) - AQL-Queries im Browser testen
3. **Feature Matrix** (Prio: Mittel) - Feature-Vergleich vs. Wettbewerber
4. **Documentation Search** (Prio: Mittel) - AI-basierte Docs-Suche mit ThemisDB
5. **Architecture Diagrams** (Prio: Mittel) - Interaktive Architektur-Visualisierung

**Phase 1 Budget:** ~70-100h (~€5.250-7.500) für Benchmark Visualizer + Feature Matrix

---

### Generische WordPress Plugins Empfehlung

Analyse von Standard-WordPress-Plugins für Website-Funktionalität (SEO, Performance, Sicherheit).

**Dokumentation:**
- [Deutsche Version](../docs/de/tools/WORDPRESS_PLUGINS_EMPFEHLUNG.md) - Vollständige Analyse (22 Kategorien, 50+ Plugins)
- [English Version](../docs/en/tools/WORDPRESS_PLUGINS_RECOMMENDATION.md) - Concise recommendations
- [Quick Reference](../docs/de/tools/WORDPRESS_PLUGINS_QUICKREF.md) - Schnellübersicht und Checkliste

**Kernempfehlungen:**
- SEO: Rank Math SEO
- Performance: WP Rocket
- Sicherheit: Wordfence Security
- Code: Syntax Highlighter Evolved
- Dokumentation: Heroic KB
- Analytics: MonsterInsights
- Backup: UpdraftPlus

**Budget:** €0 (kostenlos) bis €601/Jahr (Enterprise-Setup)

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
