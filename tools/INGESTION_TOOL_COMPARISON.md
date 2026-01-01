# ThemisDB Ingestion Tool - Vergleich Python vs. C# .NET

## Übersicht

Es wurden zwei Ingestion Tools mit identischer Funktionalität entwickelt:

1. **Python-Version** (`tools/ingest.py`) - Standalone-Tool mit minimalen Abhängigkeiten
2. **C# .NET-Version** (`tools/Themis.IngestionTool`) - Integration mit Themis.AdminTools.Shared

## Feature-Matrix

| Feature | Python | C# .NET |
|---------|--------|---------|
| Rekursive Verzeichnisdurchsuchung | ✓ | ✓ |
| SHA256 Hash-basierte Duplikaterkennung | ✓ | ✓ |
| SQLite Tracking-Datenbank | ✓ | ✓ |
| Progress Bar / Fortschrittsanzeige | ✓ (tqdm) | ✓ (Console) |
| Detailliertes Logging | ✓ (logging) | ✓ (Microsoft.Extensions.Logging) |
| YAML/JSON Konfiguration | ✓ (PyYAML) | ✓ (YamlDotNet) |
| Graph Metadata Extraktion | ✓ | ✓ |
| Vector Metadata Extraktion | ✓ | ✓ |
| Relational Metadata Extraktion | ✓ | ✓ |
| JSON Datei Support | ✓ | ✓ |
| YAML Datei Support | ✓ | ✓ |
| CSV Datei Support | ✓ | ✓ |
| Text/Markdown Support | ✓ | ✓ |
| Themis.AdminTools.Shared Integration | - | ✓ |

## Performance-Vergleich (4 Testdateien)

### Python
```
Files scanned:   4
Files processed: 4
Files skipped:   0
Files failed:    0
Elapsed time:    0.01 seconds
```

### C# .NET
```
Files scanned:   4
Files processed: 4
Files skipped:   0
Files failed:    0
Elapsed time:    0.57 seconds
```

*Hinweis: C# hat einen initialen Startup-Overhead, ist aber bei großen Datenmengen oft schneller.*

## Verwendungsbeispiele

### Python

```bash
# Grundlegende Verwendung
python3 tools/ingest.py --source /path/to/data

# Mit Konfigurationsdatei
python3 tools/ingest.py --config tools/ingest_config.example.yaml

# Mit Optionen
python3 tools/ingest.py --source /path/to/data \
    --output results.json \
    --include-ext .json .yaml .txt \
    --verbose
```

### C# .NET

```bash
cd tools/Themis.IngestionTool

# Grundlegende Verwendung
dotnet run -- --source /path/to/data

# Mit Konfigurationsdatei
dotnet run -- --config config.yaml

# Mit Optionen
dotnet run -- --source /path/to/data \
    --output results.json \
    --include-ext .json .yaml .txt \
    --verbose
```

## Ausgabeformat (identisch)

Beide Tools generieren identische JSON-Ausgaben:

```json
{
  "metadata": {
    "ingestionTime": "2026-01-01T10:00:00Z",
    "sourceDirectory": "/path/to/data",
    "config": { ... }
  },
  "statistics": {
    "totalFilesScanned": 4,
    "filesProcessed": 4,
    "filesSkipped": 0,
    "filesFailed": 0,
    "totalSizeBytes": 1085,
    "elapsedSeconds": 0.01
  },
  "ingestedFiles": [
    {
      "filePath": "/path/to/file.json",
      "fileHash": "abc123...",
      "fileSize": 380,
      "mimeType": "application/json",
      "themisMetadata": {
        "graph": {
          "entityType": "Document",
          "entityId": "file.json",
          "properties": { ... },
          "relationships": [ ... ]
        },
        "vector": {
          "objectName": "documents",
          "documentId": "file.json",
          "embeddingRequired": true,
          "textContent": "...",
          "contentLength": 380
        },
        "relational": {
          "tableName": "ingested_documents",
          "schema": { ... },
          "record": { ... }
        }
      }
    }
  ]
}
```

## Wann welches Tool verwenden?

### Python-Tool verwenden, wenn:
- Schnelle Prototyping erforderlich
- Keine .NET-Abhängigkeiten gewünscht
- Linux/Unix-Umgebungen ohne .NET
- Skript-Integration in bestehende Python-Pipelines
- Minimale Installation gewünscht

### C# .NET-Tool verwenden, wenn:
- Integration mit anderen Themis.AdminTools gewünscht
- Enterprise .NET-Umgebung
- Bessere Performance bei sehr großen Datenmengen
- Strukturiertes Logging mit Microsoft.Extensions.Logging
- Windows-native Anwendungen

## Architektur

### Python
```
ingest.py
├── IngestionConfig (dataclass)
├── FileMetadata (dataclass)
├── IngestionTracker (SQLite)
├── FileProcessor
└── IngestionEngine
```

### C# .NET
```
Themis.IngestionTool/
├── Models/
│   └── Models.cs (records)
├── Services/
│   ├── IngestionTracker.cs
│   ├── FileProcessor.cs
│   └── IngestionEngine.cs
└── Program.cs (CLI)
```

## Integration mit ThemisDB

Beide Tools generieren Metadaten, die mit ThemisDB's BaseEntity und Importer Interface kompatibel sind:

- **Graph Model**: Entitäten, Beziehungen, Properties
- **Vector Model**: Text-Content für Embeddings
- **Relational Model**: Schema und Records

Die generierten JSON-Daten können direkt in ThemisDB importiert werden.

## Zusammenfassung

Beide Tools bieten die gleiche Funktionalität und Ausgabeformate. Die Wahl hängt von der Umgebung und den Anforderungen ab:

- **Python**: Flexibel, einfach, plattformübergreifend
- **C# .NET**: Enterprise-ready, integriert, performant

Beide erfüllen die Anforderungen aus dem Problem Statement vollständig:
✓ Rekursive Verzeichnisdurchsuchung
✓ JSON/YAML Metadaten-Unterstützung
✓ ThemisDB-relevante Metadaten (Graph, Vector, Relational)
✓ Autonome Arbeitsweise
✓ Fortschrittsanzeige (Progressbar)
✓ Logging
✓ Hash-basierte Duplikaterkennung
