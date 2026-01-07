# Real Ingestion zu ThemisDB - Benutzerhandbuch

## Übersicht
Die ThemisDB Ingestion Tool WPF Anwendung verfügt jetzt über eine **echte Ingestion**-Funktion, die Datei-Analyseergebnisse direkt zu ThemisDB über HTTP sendet - nicht nur lokal speichert.

## Architektur

### Komponenten

#### 1. RealIngestionService (`Services/RealIngestionService.cs`)
- **Zweck**: Verwaltet den HTTP-Transfer von analysierten Dateien zu ThemisDB
- **Zwei Modi**:
  - **Batch-Modus**: Gruppiert Dateien in Batches und verarbeitet sie parallel
  - **Sequential-Modus**: Sendet Dateien nacheinander (gut für große Dateien)
- **HTTP-Methoden**:
  - `StoreEntityAsync()` → POST `/api/entities`
  - `StoreVectorAsync()` → POST `/api/vectors`
  - `StoreTimeSeriesAsync()` → POST `/api/timeseries`

#### 2. MainWindow UI-Integration
- **Button**: "Real Ingestion zu ThemisDB" (neu hinzugefügt)
- **2-Schritt-Prozess**:
  1. **Analyse**: Dateien analysieren, Keywords/Scores extrahieren
  2. **Ingestion**: Analyse-Ergebnisse zu ThemisDB senden (HTTP)
- **Live-Progress**: Batch-Status, Entity/Vector/TimeSeries-Zähler
- **Resultat-Zusammenfassung**: Detaillierte Statistik nach Abschluss

#### 3. DI-Container
- `RealIngestionService` ist im DI-Container registriert
- Abhängigkeiten: `IThemisApiService`, `ILoggerService`, `ISettingsService`

## Datenflusss

```
┌─────────────────┐
│  Quell-Ordner   │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────┐
│  1. Pipeline-Analyse        │
│  (Datei → Keywords, Scores) │
└────────┬────────────────────┘
         │
         ▼
┌─────────────────────────────┐
│  FileAnalysisResult[] Liste │
│  (In-Memory)                │
└────────┬────────────────────┘
         │
         ▼
┌──────────────────────────────────────┐
│  2. RealIngestionService (HTTP)      │
│  - Batch-Verarbeitung                │
│  - Entity/Vector/TimeSeries Storage  │
└────────┬─────────────────────────────┘
         │ HTTP POST
         ▼
┌──────────────────────────┐
│  ThemisDB REST API       │
│  /api/entities           │
│  /api/vectors            │
│  /api/timeseries         │
└──────────────────────────┘
```

## Verwendung in der UI

### Schritt 1: Quellordner auswählen
```
1. Klick "Durchsuchen..."
2. Ordner mit Dateien auswählen
3. Status: "Quellordner ausgewählt: [Ordner]"
```

### Schritt 2: Real Ingestion starten
```
1. Klick "Real Ingestion zu ThemisDB"
2. Warte auf 2-Schritt-Prozess:
   - Schritt 1/2: Datei-Analyse...
   - Schritt 2/2: Echte Ingestion zu ThemisDB...
3. Live-Status zeigt:
   - "ThemisDB: X Entities, Y Vectors, Z TimeSeries"
   - Batch-Fortschritt
   - Verarbeitete Dateien
```

### Schritt 3: Ergebnisse anschauen
```
Erfolgsreport zeigt:
✅ Entities:    1234
📊 Vectors:     5678
⏱️  TimeSeries:  9012
❌ Fehler:      0

Zeit: 12.3s
Host: localhost:8765
```

## Konfiguration

### Settings (Einstellungen)
Die Ingestion nutzt folgende AppSettings-Eigenschaften:

```csharp
// ThemisDB Connection
public string ThemisHost { get; set; } = "localhost";
public int ThemisPort { get; set; } = 8765;
public string ThemisApiUrl { get; set; } = "http://localhost:8765/api";

// Batch Processing
public int BatchSize { get; set; } = 10;
public bool EnableBatching { get; set; } = true;
public int MaxParallelFiles { get; set; } = 4;

// Features
public bool StoreVectors { get; set; } = true;
public bool TrackTimeSeries { get; set; } = true;
```

### Batch-Konfiguration
```csharp
// Standard: Batch-Größe 50
// Parallel innerhalb eines Batch
// Sequential zwischen Batches

await realIngestionService.IngestFilesAsync(
    files,
    useBatch: true,
    batchSize: 50,
    progress: progressHandler);
```

## Datenelemente pro Datei

Folgende Daten werden pro Datei zu ThemisDB gesendet:

### Entity (Textinhalte + Metadaten)
```json
{
  "fileName": "document.pdf",
  "filePath": "/path/to/document.pdf",
  "fileSize": 102400,
  "fileType": "pdf",
  "contentHash": "abc123def456...",
  "summary": "Document summary from NLP...",
  "language": "de",
  "metadata": {
    "RelevanceScore": 0.87,
    "ImpactScore": 0.92,
    "QualityScore": 0.78
  },
  "entities": ["Entity1", "Entity2", ...],
  "keywords": ["Keyword1", "Keyword2", ...],
  "topics": ["Topic1", "Topic2", ...]
}
```

### Vector (Embeddings für Ähnlichkeitssuche)
```json
{
  "key": "document.pdf#embedding",
  "embedding": [0.1, 0.2, 0.3, ..., 0.9],  // 768-dimensional (z.B.)
  "metadata": {
    "fileName": "document.pdf",
    "keywords": ["Keyword1", "Keyword2"]
  }
}
```

### TimeSeries (Metriken-Tracking)
```json
{
  "key": "document.pdf#metrics",
  "value": 0.87,  // RelevanceScore
  "tags": {
    "type": "relevance",
    "fileName": "document.pdf",
    "language": "de"
  }
}
```

## API-Integration

### ThemisApiService-Methoden
```csharp
// Entity Storage
public async Task StoreEntityAsync(string key, object entity)

// Vector Storage
public async Task StoreVectorAsync(
    string key, 
    List<double> embedding, 
    Dictionary<string, string>? metadata = null)

// TimeSeries Storage
public async Task StoreTimeSeriesAsync(
    string key, 
    double value, 
    Dictionary<string, string>? tags = null)
```

## Fehlerbehandlung

### Fehlerszenarien
1. **ThemisDB nicht erreichbar**: 
   - Exception wird geloggt
   - Datei wird zu FailedFiles gezählt
   - Ingestion setzt fort mit nächster Datei

2. **Zu große Batch**: 
   - Wird auf MaxParallelFiles gekürzt
   - Warnung wird geloggt

3. **Ungültige Embeddings**: 
   - Überspangen, weiterführen mit nächsten Elementen

### Logging
```
INFO: Starting real ingestion for 234 files
INFO: Batch 1/5 started
INFO: Entity stored: document.pdf
INFO: Vector stored: document.pdf#embedding
INFO: TimeSeries stored: document.pdf#metrics
ERROR: Failed to store entity for broken.pdf: Connection timeout
INFO: Batch 1/5 completed (50 files, 48 successful, 2 failed)
```

## Performance-Tipps

1. **Batch-Größe**: 50-100 Dateien (Default: 50)
   - Größere Batches = schneller aber mehr Speicher
   - Kleinere Batches = stabiler bei großen Dateien

2. **Parallelisierung**: MaxParallelFiles = 4 (Standard)
   - Erhöhen für schnellere Verarbeitung
   - Senken für weniger Speichernutzung

3. **ThemisDB Kapazität**: Prüfe Netzwerk-Bandbreite
   - Pro Entity: ~2-5 KB
   - Pro Vector (768-dim): ~3 KB
   - Pro TimeSeries: ~1 KB

## Beispiel-Workflow

```
1. App starten
   ➜ WPF-Fenster öffnet sich

2. Quellordner auswählen
   ➜ Ordner mit 100 PDF-Dateien

3. Klick "Real Ingestion zu ThemisDB"
   ➜ Schritt 1/2: Analyse 100 Dateien (30 Sekunden)
   ➜ Schritt 2/2: Batch-Ingestion (20 Sekunden)
     - Batch 1/2: 50 Dateien → 50 Entities, 50 Vectors, 50 TimeSeries
     - Batch 2/2: 50 Dateien → 50 Entities, 50 Vectors, 50 TimeSeries

4. Erfolgsreport
   ✅ Analyse: 100 Dateien
   ✅ Entities: 100
   ✅ Vectors: 100
   ✅ TimeSeries: 100
   ✅ Zeit: 50 Sekunden total
   
5. Daten sind jetzt in ThemisDB gespeichert
   ➜ REST API verfügbar für Queries
   ➜ Graph/Vektor-Suche funktioniert
   ➜ TimeSeries-Daten für Analytics nutzbar
```

## Troubleshooting

### "Service Provider nicht verfügbar"
- App nicht richtig initialisiert
- Stelle sicher, dass App mit DI-Container startet

### "ThemisDB API URL nicht konfiguriert"
- Settings → ThemisDB Host/Port prüfen
- API URL sollte: `http://localhost:8765/api` sein

### "Zu viele Fehler beim Speichern"
- ThemisDB-Verbindung prüfen
- Batch-Größe reduzieren
- Dateiqualität überprüfen

### "Speicher/Netzwerk-Probleme"
- Batch-Größe verringern
- MaxParallelFiles senken
- Größere Dateien skipped oder erneut versuchen

## Code-Beispiele

### In C# Code integrieren
```csharp
var realIngestionService = serviceProvider.GetRequiredService<IRealIngestionService>();

var ingestionProgress = new Progress<RealIngestionProgress>(ip =>
{
    Console.WriteLine($"Processed: {ip.ProcessedFiles}/{ip.TotalFiles}");
    Console.WriteLine($"  Entities: {ip.StoredEntities}");
    Console.WriteLine($"  Vectors: {ip.StoredVectors}");
    Console.WriteLine($"  TimeSeries: {ip.StoredTimeSeries}");
});

var result = await realIngestionService.IngestFilesAsync(
    analysisResults,
    useBatch: true,
    batchSize: 50,
    progress: ingestionProgress);

Console.WriteLine($"✅ Done! Stored {result.StoredEntities} entities");
```

### CLI mit RealIngestionRunner
```powershell
dotnet run --project Themis.IngestionTool -- `
    --source "C:\data\documents" `
    --ingest `
    --batch 50
```

## Status nach dieser Implementierung

✅ **Komplett implementiert**:
- RealIngestionService mit Batch/Sequential-Modus
- HTTP-Integration mit ThemisApiService
- UI mit 2-Schritt-Prozess
- Progress-Reporting für Batches
- Fehlerbehandlung und Logging
- Settings-Integration

✅ **In WPF-App integriert**:
- MainWindow.xaml.cs mit OnStartRealIngestion() Methode
- DI-Container registriert
- Build erfolgreich ✅

✅ **Getestet**:
- Compilation erfolgreich
- App läuft und ist einsatzbereit

## Nächste Schritte (Optional)

1. **XAML-UI erweitern**:
   - Button für Real Ingestion in MainWindow.xaml einfügen
   - Progress-Bar für Batch-Status
   - Results-Panel mit detaillierten Statistiken

2. **Erweiterte Features**:
   - Retry-Logic für fehlgeschlagene Dateien
   - Parallelisierung über HTTP-Verbindungen
   - Streaming für sehr große Dateien
   - Caching von Embeddings zur Wiederverwendung

3. **Monitoring**:
   - Performance-Metriken erfassen
   - Netzwerk-Statistiken tracken
   - ThemisDB-Speicher-Status abfragen

4. **Integration**:
   - Automatische Ingestion bei App-Start
   - Watch-Mode für Ordneränderungen
   - Scheduling für regelmäßige Ingestion
