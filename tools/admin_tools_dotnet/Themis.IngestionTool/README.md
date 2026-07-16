# Themis.IngestionTool

Ein C# .NET Console-Tool zur rekursiven Durchsuchung von Verzeichnissen nach ingestierbaren Dateien mit Hash-basierter Duplikaterkennung und Metadatenextraktion für ThemisDB.

## Features

- **Rekursive Verzeichnisdurchsuchung** mit konfigurierbaren Filtern
- **Hash-basierte Duplikaterkennung** (SHA256) - bereits ingestierte Dateien werden übersprungen
- **SQLite-Tracking-Datenbank** für verarbeitete Dateien
- **Fortschrittsanzeige** während der Verarbeitung
- **Detailliertes Logging** mit Microsoft.Extensions.Logging
- **Metadatenextraktion** für ThemisDB-Modelle:
  - **Graph**: Entitäten, Beziehungen, Properties
  - **Vector**: Text-Content für Embeddings, semantische Suche
  - **Relational**: Schema, Datensätze, Feldtypen
- **Unterstützung** für JSON, YAML, CSV, Text-Dateien
- **Integration** mit Themis.AdminTools.Shared Bibliothek
- **Konfiguration** über YAML/JSON oder Kommandozeile

## Voraussetzungen

- .NET 8.0 SDK oder höher
- Windows, Linux oder macOS

## Installation

```bash
cd tools/Themis.IngestionTool
dotnet restore
dotnet build
```

## Verwendung

### Grundlegende Verwendung

```bash
dotnet run --project Themis.IngestionTool -- --source /path/to/data
```

### Mit Konfigurationsdatei

```bash
dotnet run --project Themis.IngestionTool -- --config config.yaml
```

### Mit benutzerdefinierten Optionen

```bash
dotnet run --project Themis.IngestionTool -- \
    --source /path/to/data \
    --output results.json \
    --db tracking.db \
    --include-ext .json .yaml .txt \
    --max-size 50 \
    --verbose
```

### Nur bestimmte Modelle aktivieren

```bash
# Nur Vector und Graph, ohne Relational
dotnet run --project Themis.IngestionTool -- \
    --source /path/to/data \
    --no-relational
```

## Kommandozeilenoptionen

| Option | Beschreibung | Standard |
|--------|--------------|----------|
| `--source, -s` | Quellverzeichnis zum Scannen | - |
| `--output, -o` | Ausgabedatei für Ergebnisse | ingestion_output.json |
| `--config, -c` | Konfigurationsdatei (YAML/JSON) | - |
| `--db, -d` | SQLite-Datenbank für Tracking | ingestion_tracker.db |
| `--include-ext` | Nur diese Dateierweiterungen | alle |
| `--exclude-ext` | Diese Dateierweiterungen ausschließen | .exe, .dll, .so, etc. |
| `--max-size` | Maximale Dateigröße in MB | 100.0 |
| `--no-vector` | Vektor-Metadaten deaktivieren | false |
| `--no-graph` | Graph-Metadaten deaktivieren | false |
| `--no-relational` | Relationale Metadaten deaktivieren | false |
| `--verbose, -v` | Ausführliches Logging | false |

## Konfigurationsdatei

Beispiel `config.yaml`:

```yaml
sourceDir: "/path/to/your/data"
outputFile: "ingestion_output.json"
dbPath: "ingestion_tracker.db"

includeExtensions:
  - ".json"
  - ".yaml"
  - ".txt"

excludeExtensions:
  - ".exe"
  - ".dll"

excludePatterns:
  - ".git"
  - "node_modules"
  - "bin"
  - "obj"

maxFileSizeMb: 100.0
extractTextPreview: true
previewLength: 500

generateVectorMetadata: true
generateGraphMetadata: true
generateRelationalMetadata: true
```

## Ausgaben

### ingestion_output.json

Enthält detaillierte Metadaten aller ingestierten Dateien im JSON-Format:

```json
{
  "metadata": {
    "ingestionTime": "2024-01-01T10:00:00Z",
    "sourceDirectory": "/path/to/data",
    "config": { ... }
  },
  "statistics": {
    "totalFilesScanned": 100,
    "filesProcessed": 95,
    "filesSkipped": 3,
    "filesFailed": 2,
    "totalSizeBytes": 10485760,
    "elapsedSeconds": 2.5
  },
  "ingestedFiles": [
    {
      "filePath": "/path/to/data/document.json",
      "fileHash": "abc123...",
      "fileSize": 1024,
      "mimeType": "application/json",
      "themisMetadata": {
        "graph": { ... },
        "vector": { ... },
        "relational": { ... }
      }
    }
  ]
}
```

### ingestion_tracker.db

SQLite-Datenbank mit allen verarbeiteten Dateien und deren Hashes. Verhindert das erneute Verarbeiten bereits ingestierter Dateien.

## Architektur

Das Tool verwendet folgende Komponenten:

- **Models/Models.cs**: Datenmodelle für Metadaten und Konfiguration
- **Services/IngestionTracker.cs**: SQLite-basiertes Tracking verarbeiteter Dateien
- **Services/FileProcessor.cs**: Dateiverarbeitung und Metadatenextraktion
- **Services/IngestionEngine.cs**: Hauptlogik für den Ingestion-Prozess
- **Program.cs**: CLI-Interface mit System.CommandLine

## Integration mit ThemisDB

Das Tool nutzt die **Themis.AdminTools.Shared** Bibliothek und generiert Metadaten, die mit ThemisDB's BaseEntity und Importer Interface kompatibel sind.

Die generierten JSON-Daten können direkt in ThemisDB importiert werden.

## Performance

- **Hash-Berechnung**: SHA256 für zuverlässige Duplikaterkennung
- **Streaming-Verarbeitung**: Große Dateien werden effizient verarbeitet
- **Parallele Verarbeitung**: Kann bei Bedarf erweitert werden
- **SQLite-Indexierung**: Schnelle Duplikatprüfung

## Entwicklung

### Build

```bash
dotnet build
```

### Test

```bash
# Erstelle Testdaten
mkdir -p /tmp/test_data
echo '{"test": "data"}' > /tmp/test_data/test.json

# Führe Tool aus
dotnet run -- --source /tmp/test_data --verbose
```

### Release Build

```bash
dotnet publish -c Release -o ./publish
```

## Lizenz

Siehe Hauptprojekt-Lizenz (MIT).
