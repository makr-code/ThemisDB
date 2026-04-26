# Architektur-Dokumentation

## Übersicht

Das Themis Ingestion Tool ist als modulare WPF-Anwendung mit klarer Trennung von Verantwortlichkeiten konzipiert.

## Design-Pattern

### MVVM (Model-View-ViewModel)

```
View (XAML)
    ↕ DataBinding
ViewModel (Logic)
    ↕ Services
Model (Data)
    ↕ API
ThemisDB
```

### Dependency Injection

Alle Services werden über den DI-Container registriert (`App.xaml.cs`):

```csharp
services.AddSingleton<IIngestionService, IngestionService>();
services.AddSingleton<ILlamaService, LlamaService>();
services.AddSingleton<INlpAnalysisService, NlpAnalysisService>();
services.AddSingleton<IGraphAnalysisService, GraphAnalysisService>();
services.AddSingleton<IIngestionPipelineService, IngestionPipelineService>();
services.AddSingleton<IThemisConnectionService, ThemisConnectionService>();
services.AddSingleton<ISettingsService, SettingsService>();
```

## Komponenten-Übersicht

### 1. Views (Presentation Layer)

#### MainWindow.xaml
- **Zweck**: Hauptfenster mit 2-Spalten-Layout
- **Features**:
  - Linke Spalte: Steuerung (Ordner-Auswahl, DryRun, Start/Stop)
  - Rechte Spalte: Live-Metriken (DataGrid)
  - Statusbar: Status + Themis-Verbindung
  - Menüleiste: Datei, Einstellungen, Hilfe

#### SettingsDialog.xaml
- **Zweck**: Konfigurationsdialog
- **Features**:
  - Themis-Host/Port
  - Datenbank-Pfad
  - Max. Dateigröße
  - Metadaten-Optionen (Vector, Graph, Relational)

### 2. ViewModels (Business Logic Layer)

#### MainWindowViewModel
- **Eigenschaften**:
  - `SourceFolder`, `OutputFile`, `IsDryRun`
  - `IsRunning`, `IsConnected`
  - `FileCount`, `ProcessedCount`
  - `CurrentStage`, `Status`
  - `LiveResults` (ObservableCollection)

- **Commands**:
  - `BrowseSourceCommand`
  - `StartIngestionCommand`
  - `CancelIngestionCommand`
  - `OpenSettingsCommand`

- **Lifecycle**:
  1. Constructor: DI-Injection, Command-Initialisierung
  2. `InitializeAsync()`: Themis-Connection-Check, Event-Registrierung
  3. `LoadSettings()`: Letzte Konfiguration laden
  4. Event-Handler: Connection-Status-Updates

#### SettingsDialogViewModel
- **Eigenschaften**: Alle AppSettings-Properties mit Binding
- **Commands**: `SaveCommand`, `CancelCommand`
- **Validierung**: Port-Range, Pfad-Existenz

### 3. Models (Data Layer)

#### FileAnalysisResult
```csharp
public class FileAnalysisResult
{
    // Basic Info
    public string FilePath { get; set; }
    public string FileName { get; set; }
    public long FileSize { get; set; }
    public string FileType { get; set; }
    public string ContentHash { get; set; }
    
    // Scores
    public double RelevanceScore { get; set; }
    public double ImpactScore { get; set; }
    public double QualityScore { get; set; }
    
    // Graph Metrics
    public int GraphNodeCount { get; set; }
    public int RelationshipCount { get; set; }
    
    // NLP Data
    public List<string> ExtractedEntities { get; set; }
    public List<string> Keywords { get; set; }
    public List<string> Topics { get; set; }
    public string Summary { get; set; }
    public string Language { get; set; }
    
    // Metadata
    public Dictionary<string, string> Metadata { get; set; }
    public DateTime AnalysisTimestamp { get; set; }
    public TimeSpan ProcessingTime { get; set; }
    
    // Status
    public bool IsProcessed { get; set; }
    public bool IsDuplicate { get; set; }
    public string ErrorMessage { get; set; }
}
```

#### IngestionPipelineResult
```csharp
public class IngestionPipelineResult
{
    public int TotalFiles { get; set; }
    public int ProcessedFiles { get; set; }
    public int SkippedFiles { get; set; }
    public int ErrorFiles { get; set; }
    public int DuplicateFiles { get; set; }
    public List<FileAnalysisResult> Results { get; set; }
    public TimeSpan TotalTime { get; set; }
    public bool IsDryRun { get; set; }
}
```

#### PipelineStage
```csharp
public class PipelineStage
{
    public string Name { get; set; }
    public string Description { get; set; }
    public int ProcessedCount { get; set; }
    public int TotalCount { get; set; }
    public bool IsComplete { get; set; }
    public TimeSpan ElapsedTime { get; set; }
}
```

### 4. Services (Service Layer)

#### IIngestionPipelineService
**Verantwortlichkeit**: Orchestrierung der gesamten Analyse-Pipeline

**Methoden**:
- `ExecutePipelineAsync(sourceFolder, isDryRun, progress, fileProgress)`
- `CancelPipeline()`

**Pipeline-Stages**:
1. **Datei-Sammlung**: Rekursives Scannen des Quellordners
2. **LLM-Initialisierung**: Verfügbarkeits-Check
3. **Analyse**: Pro Datei:
   - Hash-Berechnung (SHA256)
   - Duplikat-Erkennung
   - Metadaten-Extraktion
   - NLP-Analyse
   - Graph-Analyse
   - LLM-Analyse (optional)
4. **Abschluss**: Aggregation und Export

**Progress-Reporting**:
```csharp
IProgress<PipelineStage> progress // Stage-Updates
IProgress<FileAnalysisResult> fileProgress // Per-File-Updates
```

#### ILlamaService
**Verantwortlichkeit**: LLM-Integration (llama.cpp)

**Methoden**:
- `IsAvailableAsync()`: Prüft LLM-Verfügbarkeit
- `GenerateSummaryAsync(content)`: Erstellt Zusammenfassung
- `ExtractKeywordsAsync(content)`: Extrahiert Keywords
- `ExtractEntitiesAsync(content)`: Named Entity Recognition
- `CalculateRelevanceScoreAsync(content)`: Relevanz-Bewertung

**Aktueller Status**: Simulation mit heuristischen Algorithmen
**TODO**: HTTP-Integration zu llama.cpp Endpoint

#### INlpAnalysisService
**Verantwortlichkeit**: Natural Language Processing

**Methoden**:
- `ExtractTopicsAsync(content)`: Topic Modeling
- `DetectLanguageAsync(content)`: Sprach-Erkennung
- `CalculateQualityScoreAsync(content)`: Quality Scoring
- `ExtractMetadata(filePath)`: Datei-Metadaten

**Implementierung**:
- Regex-basierte Keyword-Erkennung
- Heuristische Topic-Kategorisierung
- Qualitäts-Metriken (Zeilenlänge, Kommentare, Struktur)

#### IGraphAnalysisService
**Verantwortlichkeit**: Code-Struktur-Analyse

**Methoden**:
- `EstimateGraphNodesAsync(content)`: Zählt Klassen/Funktionen
- `EstimateRelationshipsAsync(content)`: Zählt Imports/Referenzen
- `CalculateImpactScoreAsync(content)`: Impact-Bewertung

**Graph-Knoten-Typen**:
- Klassen (`class`)
- Funktionen (`function`, `def`)
- Interfaces (`interface`)
- Module (Datei selbst)

**Beziehungs-Typen**:
- Imports (`import`, `using`, `require`, `from ... import`)
- Vererbung (implicit durch Class-Struktur)
- Calls (implicit durch Code-Referenzen)

#### IThemisConnectionService
**Verantwortlichkeit**: Verbindungs-Management

**Features**:
- **Heartbeat-Timer**: Alle 5 Sekunden automatische Prüfung
- **Event-basiert**: `ConnectionStatusChanged` Event
- **Timeout**: 3 Sekunden pro Request
- **Health-Check**: GET `/health` Endpoint

**Status-Tracking**:
```csharp
private bool _lastConnectionState = false;

// Event nur bei Änderung
if (currentState != _lastConnectionState) {
    ConnectionStatusChanged?.Invoke(...);
}
```

#### ISettingsService
**Verantwortlichkeit**: Persistente Konfiguration

**Methoden**:
- `LoadSettings()`: Liest `appsettings.json`
- `SaveSettings(settings)`: Schreibt `appsettings.json`

**Datei-Location**: `%AppData%\ThemisIngestionTool\appsettings.json`

## Datenfluss

### Startup-Flow
```
App.OnStartup()
    → Build DI Container
    → Resolve MainWindow
    → MainWindow Constructor
        → Inject MainWindowViewModel
        → DataContext = ViewModel
    → MainWindowViewModel Constructor
        → Inject Services
        → Initialize Commands
        → Call InitializeAsync()
            → LoadSettings()
            → CheckConnectionAsync()
            → Register Event Handlers
    → MainWindow.Show()
```

### Ingestion-Flow
```
User clicks "Start"
    → OnStartIngestion()
        → Validate (Folder exists, Themis connected)
        → Create Progress Handlers
        → Call ExecutePipelineAsync()
            ┌─ Stage 1: File Collection
            │   → Directory.GetFiles (recursive)
            │   → Filter by extension
            │
            ├─ Stage 2: LLM Init
            │   → LlamaService.IsAvailableAsync()
            │
            ├─ Stage 3: Analysis (per file)
            │   → Read file content
            │   → Calculate SHA256 hash
            │   → Check duplicate
            │   → NlpService.AnalyzeAsync()
            │   → GraphService.AnalyzeAsync()
            │   → LlamaService.AnalyzeAsync() (optional)
            │   → Report fileProgress
            │   → Update LiveResults (UI)
            │
            └─ Stage 4: Completion
                → Aggregate results
                → Export JSON
                → Show summary
```

## Threading-Modell

### UI-Thread
- Alle View-Updates
- Command-Execution
- Event-Handler
- ObservableCollection-Updates

### Background-Threads
- File I/O (async)
- HTTP-Requests (async)
- Analysis-Operations (async)
- Timer-Callbacks (ThreadPool)

### Synchronization
- `IProgress<T>`: Marshalling zu UI-Thread
- `ObservableCollection`: Automatisches UI-Update
- `INotifyPropertyChanged`: Data-Binding-Updates

## Error-Handling

### Service-Level
```csharp
try {
    // Operation
} catch (Exception ex) {
    _loggerService.LogError(ex.Message);
    return defaultValue;
}
```

### Pipeline-Level
```csharp
try {
    var result = await AnalyzeFileAsync(file);
    results.Add(result);
} catch (Exception ex) {
    result.ErrorFiles++;
    results.Add(new FileAnalysisResult {
        ErrorMessage = ex.Message,
        IsProcessed = false
    });
}
```

### UI-Level
```csharp
try {
    await pipeline.ExecuteAsync();
    MessageBox.Show("Erfolg");
} catch (Exception ex) {
    MessageBox.Show($"Fehler: {ex.Message}");
} finally {
    IsRunning = false;
}
```

## Performance-Optimierungen

### Datei-Analyse
- **Parallel-Verarbeitung**: TODO (aktuell sequenziell)
- **Duplikat-Check**: HashSet für O(1) Lookup
- **Content-Limit**: Max. 100KB für LLM-Analyse
- **Timeout**: 3 Sekunden pro HTTP-Request

### UI-Updates
- **Throttling**: Progress-Reports nur bei Änderung
- **Batch-Updates**: LiveResults pro Datei (nicht pro Metrik)
- **Virtual Scrolling**: DataGrid mit Virtualisierung

### Memory-Management
- **Streaming**: Keine vollständigen File-Inhalte im RAM
- **Disposal**: using-Blocks für HttpClient, StreamReader
- **GC-Hints**: Nach großen Operationen

## Erweiterbarkeit

### Neue Analyse-Services hinzufügen
1. Interface in `Services/AnalysisServiceInterfaces.cs`
2. Implementation in `Services/AnalysisServiceImplementations.cs`
3. Registration in `App.xaml.cs`
4. Integration in `IngestionPipelineService`

### Neue Metadaten-Felder
1. Erweitern Sie `FileAnalysisResult`
2. Aktualisieren Sie Service-Methoden
3. Fügen Sie DataGrid-Column hinzu
4. JSON-Serialisierung automatisch

### Themis-API-Integration
1. Erstellen Sie DTOs für API-Requests
2. Implementieren Sie in `ThemisApiService`
3. Rufen Sie auf in `IngestionPipelineService`
4. Verwenden Sie `HttpClient` mit Polly für Retry-Logic

## Sicherheit

### Credentials
- **Keine Passwörter im Code**
- Settings-Datei in `%AppData%`
- TODO: Verschlüsselung für sensitive Daten

### Input-Validierung
- Pfad-Validierung (keine Escapes)
- Port-Range-Check (1-65535)
- File-Size-Limit (konfigurierbar)

### Exception-Handling
- Keine sensiblen Daten in Logs
- User-friendly Error-Messages
- Stack-Traces nur für Entwickler
