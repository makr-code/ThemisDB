> ⚠️ **Historischer Vergleich** – Beschreibt den Stand zum Zeitpunkt der Erstellung.

> **Historischer Stand:** 2026-04-19

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
| **Graph Metadata Extraktion** | ✓ | ✓ |
| **Vector Metadata Extraktion** | ✓ | ✓ |
| **Relational Metadata Extraktion** | ✓ | ✓ |
| **Geo/Spatial Metadata Extraktion** | ✓ | ✓ |
| **Process-Aware Metadata Extraktion** | ✓ | ✓ |
| JSON Datei Support | ✓ | ✓ |
| YAML Datei Support | ✓ | ✓ |
| CSV Datei Support | ✓ | ✓ |
| Text/Markdown Support | ✓ | ✓ |
| Themis.AdminTools.Shared Integration | - | ✓ |

## Neue Features: Geo/Spatial & Process-Aware

### Geo/Spatial Metadaten
- **Koordinaten**: Automatische Erkennung von latitude/longitude Feldern
- **Adressen**: Extraktion von Straße, Stadt, PLZ, Land
- **Geometrien**: WKT-Format Generierung (z.B. POINT(13.405 52.52))
- **Spatial Indexing**: R-Tree Index Hints, SRID 4326 (WGS84)
- **Vollständige Adressen**: Zusammengesetzte Adress-Strings

### Process-Aware Metadaten
- **State/Status**: Prozesszustand Tracking
- **Activities/Tasks**: Aktivitäten und Aufgaben
- **Case/Instance IDs**: Prozessinstanz-Identifikation
- **Process Variables**: Kontextvariablen und Daten
- **Tokens**: Ausführungspositionen (Petri-Netze)
- **BPMN-Erkennung**: Automatische BPMN-Formatserkennung
- **Process Mining**: Bereitschaft für Process Mining Analysen
- **Collection Hints**: Vorgeschlagene Zielcollections

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

## Ausgabeformat (erweitert mit Geo & Process)

Beide Tools generieren erweiterte JSON-Ausgaben mit allen ThemisDB Multi-Model Metadaten:

```json
{
  "metadata": {
    "ingestionTime": "2026-01-01T10:00:00Z",
    "sourceDirectory": "/path/to/data",
    "config": { ... }
  },
  "statistics": {
    "totalFilesScanned": 3,
    "filesProcessed": 3,
    "filesSkipped": 0,
    "filesFailed": 0,
    "totalSizeBytes": 1500,
    "elapsedSeconds": 0.01
  },
  "ingestedFiles": [
    {
      "filePath": "/path/to/file.json",
      "fileHash": "abc123...",
      "fileSize": 500,
      "mimeType": "application/json",
      "themisMetadata": {
        "graph": {
          "entityType": "Document",
          "entityId": "file.json",
          "properties": { "source_file": "...", "file_type": ".json" },
          "relationships": []
        },
        "vector": {
          "objectName": "documents",
          "documentId": "file.json",
          "embeddingRequired": true,
          "textContent": "...",
          "contentLength": 500
        },
        "relational": {
          "tableName": "ingested_documents",
          "schema": { "id": "TEXT PRIMARY KEY", ... },
          "record": { "id": "file.json", ... }
        },
        "geo": {
          "hasGeometry": true,
          "coordinateFields": {
            "latitude": 52.52,
            "longitude": 13.405,
            "srid": 4326
          },
          "addressFields": {
            "address": "Hauptstraße 123",
            "city": "Berlin",
            "postal_code": "10115",
            "country": "Germany"
          },
          "geometryWkt": "POINT(13.405 52.52)",
          "fullAddress": "Hauptstraße 123, 10115, Berlin, Germany",
          "spatialIndexRequired": true,
          "indexType": "R-Tree"
        },
        "process": {
          "isProcessAware": true,
          "processFields": {
            "state": "pending",
            "activity": "Review Order",
            "case_id": "ORDER-12345",
            "timestamp": "2026-01-01T10:00:00Z",
            "resource": "user@example.com",
            "variables": { ... }
          },
          "hasState": true,
          "hasVariables": true,
          "isProcessInstance": true,
          "processMiningReady": true,
          "suggestedCollection": "_process_instances"
        }
      }
    }
  ]
}
```

## Beispiel-Dateien für Geo/Process Metadaten

### Location-Daten (Geo)
```json
{
  "name": "Restaurant Example",
  "address": "Hauptstraße 123",
  "city": "Berlin",
  "postal_code": "10115",
  "country": "Germany",
  "latitude": 52.5200,
  "longitude": 13.4050
}
```

### Process-Instanz (BPMN/Workflow)
```json
{
  "case_id": "ORDER-12345",
  "activity": "Approve Order",
  "timestamp": "2026-01-01T10:00:00Z",
  "state": "pending_approval",
  "resource": "john.doe@example.com",
  "variables": {
    "customer_id": "CUST-789",
    "order_total": 150.00
  }
}
```

### Kombiniert: Geo + Process
```json
{
  "case_id": "DELIVERY-456",
  "activity": "Deliver Package",
  "state": "in_transit",
  "latitude": 48.8566,
  "longitude": 2.3522,
  "address": "15 Avenue des Champs-Élysées",
  "city": "Paris"
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

Beide Tools generieren Metadaten, die mit ThemisDB's Multi-Model Architektur kompatibel sind:

- **Graph Model**: Entitäten, Beziehungen, Properties
- **Vector Model**: Text-Content für Embeddings, semantische Suche
- **Relational Model**: Schema und Records
- **Geo/Spatial Model**: Koordinaten (WGS84), Adressen, WKT Geometrien, Spatial Index Hints
- **Process Model**: BPMN, Workflows, State Machines, Process Mining

Die generierten JSON-Daten können direkt in ThemisDB importiert werden und unterstützen alle reservierten Felder:
- `_geometry` für Geo-Daten
- `_state`, `_tokens`, `_variables` für Process-Daten
- `_from`, `_to` für Graph-Beziehungen
- `_embedding` für Vector-Daten

## Zusammenfassung

Beide Tools bieten die gleiche Funktionalität und Ausgabeformate. Die Wahl hängt von der Umgebung und den Anforderungen ab:

- **Python**: Flexibel, einfach, plattformübergreifend
- **C# .NET**: Enterprise-ready, integriert, performant

Beide erfüllen die Anforderungen aus dem Problem Statement vollständig:
✅ Rekursive Verzeichnisdurchsuchung
✅ JSON/YAML Metadaten-Unterstützung
✅ ThemisDB-relevante Metadaten (Graph, Vector, Relational, **Geo, Process**)
✅ Autonome Arbeitsweise
✅ Fortschrittsanzeige (Progressbar)
✅ Logging
✅ Hash-basierte Duplikaterkennung

### Neu hinzugefügt:
✅ **Geo/Spatial Metadaten** - Koordinaten, Adressen, WKT Geometrien
✅ **Process-Aware Metadaten** - BPMN, Workflows, State Machines, Process Mining Readiness
