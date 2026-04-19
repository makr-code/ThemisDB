> **Historischer Stand:** 2026-04-19
> <!-- TODO: verify against current source – method signatures and endpoint URLs below may have changed -->

# Analytics-Pipeline Dokumentation

## Übersicht

Die Analytics-Pipeline ist das Herzstück des Ingestion Tools und verarbeitet Dateien in mehreren Stages mit parallelen Analyse-Services.

## Pipeline-Architektur

```
┌──────────────────────────────────────────────────────┐
│                   Pipeline-Flow                      │
└──────────────────────────────────────────────────────┘

Input: Source Folder
    │
    ├─► Stage 1: File Collection
    │   ├─ Recursive directory scan
    │   ├─ File type filtering
    │   └─ Count total files
    │
    ├─► Stage 2: LLM Initialization
    │   ├─ Check LLM availability
    │   └─ Configure services
    │
    ├─► Stage 3: File Analysis (per file)
    │   ├─ Read content
    │   ├─ Calculate hash (SHA256)
    │   ├─ Duplicate check
    │   ├─ Metadata extraction
    │   ├─ NLP Analysis ─────┐
    │   ├─ Graph Analysis ───┼─► Parallel
    │   └─ LLM Analysis ─────┘
    │
    └─► Stage 4: Completion
        ├─ Aggregate results
        ├─ Export JSON
        └─ Generate summary

Output: IngestionPipelineResult + JSON File
```

## Service-Übersicht

### 1. LlamaService (LLM-Integration)

**Zweck**: Integration mit Large Language Models für fortgeschrittene Text-Analyse

**Aktueller Status**: Simulation mit Heuristiken (TODO: Echte Integration)

**Methoden**:

#### IsAvailableAsync()
Prüft ob LLM verfügbar ist
```csharp
public async Task<bool> IsAvailableAsync()
{
    // TODO: Echter Check gegen llama.cpp endpoint
    // Beispiel: GET http://localhost:11434/api/tags
    await Task.Delay(100);
    return true; // Simulation
}
```

#### GenerateSummaryAsync(content)
Erstellt Zusammenfassung des Inhalts
```csharp
public async Task<string> GenerateSummaryAsync(string content)
{
    // Aktuell: Erste 5 Zeilen, max 200 Zeichen
    // TODO: Echter LLM-Prompt
    /*
    var prompt = $"Summarize the following code in 2-3 sentences:\n\n{content}";
    var response = await _llamaClient.GenerateAsync(prompt);
    return response.Text;
    */
    
    var lines = content.Split('\n').Take(5);
    return $"Zusammenfassung: {string.Join(" ", lines).Substring(0, Math.Min(200, content.Length))}...";
}
```

#### ExtractKeywordsAsync(content)
Extrahiert wichtige Keywords
```csharp
public async Task<List<string>> ExtractKeywordsAsync(string content)
{
    // Aktuell: Häufigkeitsanalyse mit Regex
    // TODO: LLM-basierte Extraktion
    /*
    var prompt = $"Extract 10 important keywords from:\n\n{content}";
    var response = await _llamaClient.GenerateAsync(prompt);
    return ParseKeywords(response.Text);
    */
    
    var words = Regex.Matches(content.ToLower(), @"\b\w{4,}\b")
        .Cast<Match>()
        .Select(m => m.Value)
        .GroupBy(w => w)
        .OrderByDescending(g => g.Count())
        .Take(10)
        .Select(g => g.Key)
        .ToList();
    
    return words;
}
```

#### ExtractEntitiesAsync(content)
Named Entity Recognition
```csharp
public async Task<List<string>> ExtractEntitiesAsync(string content)
{
    // Aktuell: Großgeschriebene Wörter
    // TODO: Echte NER via LLM
    /*
    var prompt = $"Extract named entities (people, places, organizations) from:\n\n{content}";
    var response = await _llamaClient.GenerateAsync(prompt);
    return ParseEntities(response.Text);
    */
    
    var entities = Regex.Matches(content, @"\b[A-Z][a-z]+(?:\s+[A-Z][a-z]+)*\b")
        .Cast<Match>()
        .Select(m => m.Value)
        .Distinct()
        .Take(15)
        .ToList();
    
    return entities;
}
```

#### CalculateRelevanceScoreAsync(content)
Berechnet Relevanz-Score
```csharp
public async Task<double> CalculateRelevanceScoreAsync(string content)
{
    // Aktuell: Regel-basiert
    // TODO: LLM-basiertes Scoring
    /*
    var prompt = $"Rate the relevance/importance of this code on a scale of 0-1:\n\n{content}";
    var response = await _llamaClient.GenerateAsync(prompt);
    return ParseScore(response.Text);
    */
    
    var score = 0.5;
    if (content.Length > 1000) score += 0.2;
    if (content.Contains("class") || content.Contains("function")) score += 0.15;
    if (Regex.Matches(content, @"\b\w+\b").Count > 100) score += 0.15;
    
    return Math.Min(1.0, score);
}
```

**LLM-Integration Roadmap**:
```csharp
// Phase 1: llama.cpp HTTP API
public class LlamaService : ILlamaService
{
    private readonly HttpClient _client;
    private const string LlamaEndpoint = "http://localhost:11434";
    
    public async Task<string> GenerateSummaryAsync(string content)
    {
        var request = new {
            model = "llama2",
            prompt = $"Summarize concisely:\n\n{content}",
            stream = false
        };
        
        var response = await _client.PostAsJsonAsync($"{LlamaEndpoint}/api/generate", request);
        var result = await response.Content.ReadFromJsonAsync<LlamaResponse>();
        return result.Response;
    }
}

// Phase 2: Themis LLM Integration
// Wenn ThemisDB eigenen LLM-Endpoint hat:
public async Task<string> GenerateSummaryAsync(string content)
{
    var response = await _themisClient.PostAsync("/llm/interaction", new {
        prompt = $"Summarize:\n\n{content}",
        max_tokens = 200
    });
    
    var interaction = await response.Content.ReadFromJsonAsync<LLMInteraction>();
    return interaction.Response;
}
```

### 2. NlpAnalysisService

**Zweck**: Natural Language Processing ohne LLM

**Implementierung**: Regel-basiert, heuristisch

#### ExtractTopicsAsync(content)
Topic Modeling via Keyword-Matching
```csharp
public async Task<List<string>> ExtractTopicsAsync(string content)
{
    var topics = new List<string>();
    
    // Rule-based categorization
    if (content.Contains("database") || content.Contains("sql")) 
        topics.Add("Database");
    if (content.Contains("api") || content.Contains("http")) 
        topics.Add("API");
    if (content.Contains("class") || content.Contains("interface")) 
        topics.Add("Architecture");
    if (content.Contains("test") || content.Contains("assert")) 
        topics.Add("Testing");
    if (content.Contains("security") || content.Contains("auth")) 
        topics.Add("Security");
    
    return topics.Any() ? topics : new List<string> { "General" };
}
```

**Erweiterung mit ML**:
```csharp
// TODO: Nutze ML.NET für echtes Topic Modeling
public async Task<List<string>> ExtractTopicsAsync(string content)
{
    var pipeline = _mlContext.Transforms.Text
        .FeaturizeText("Features", nameof(TextData.Text))
        .Append(_mlContext.Clustering.Trainers.KMeans("Features", numberOfClusters: 5));
    
    var model = pipeline.Fit(_trainingData);
    var prediction = model.Transform(new[] { new TextData { Text = content } });
    
    return GetTopicsFromClusters(prediction);
}
```

#### DetectLanguageAsync(content)
Sprach-Erkennung
```csharp
public async Task<string> DetectLanguageAsync(string content)
{
    // Heuristic: Common stopwords
    if (Regex.IsMatch(content, @"\b(der|die|das|und|oder|nicht)\b", RegexOptions.IgnoreCase))
        return "de";
    if (Regex.IsMatch(content, @"\b(the|and|or|not|is|are)\b", RegexOptions.IgnoreCase))
        return "en";
    
    return "unknown";
}
```

#### CalculateQualityScoreAsync(content)
Code-Quality-Metriken
```csharp
public async Task<double> CalculateQualityScoreAsync(string content)
{
    var score = 0.6; // Base score
    
    var lineCount = content.Split('\n').Length;
    var avgLineLength = content.Length / Math.Max(1, lineCount);
    
    // Readable line length
    if (avgLineLength > 20 && avgLineLength < 120) score += 0.15;
    
    // Has comments
    if (content.Contains("//") || content.Contains("/*")) score += 0.1;
    
    // Sufficient length
    if (lineCount > 50) score += 0.15;
    
    return Math.Min(1.0, score);
}
```

#### ExtractMetadata(filePath)
Datei-System-Metadaten
```csharp
public Dictionary<string, string> ExtractMetadata(string filePath)
{
    var metadata = new Dictionary<string, string>();
    
    var fileInfo = new FileInfo(filePath);
    metadata["Size"] = fileInfo.Length.ToString();
    metadata["Extension"] = fileInfo.Extension;
    metadata["CreatedDate"] = fileInfo.CreationTime.ToString("yyyy-MM-dd HH:mm:ss");
    metadata["ModifiedDate"] = fileInfo.LastWriteTime.ToString("yyyy-MM-dd HH:mm:ss");
    metadata["Directory"] = Path.GetDirectoryName(filePath) ?? "";
    
    return metadata;
}
```

### 3. GraphAnalysisService

**Zweck**: Code-Struktur und Beziehungs-Analyse

#### EstimateGraphNodesAsync(content)
Zählt strukturelle Elemente
```csharp
public async Task<int> EstimateGraphNodesAsync(string content)
{
    var nodes = 1; // Base node (file itself)
    
    // Classes
    nodes += Regex.Matches(content, @"\bclass\s+\w+").Count;
    
    // Functions (JavaScript, Python)
    nodes += Regex.Matches(content, @"\bfunction\s+\w+").Count;
    nodes += Regex.Matches(content, @"\bdef\s+\w+").Count;
    
    // Interfaces
    nodes += Regex.Matches(content, @"\binterface\s+\w+").Count;
    
    return nodes;
}
```

**Erweiterte Graph-Analyse**:
```csharp
public async Task<GraphStructure> AnalyzeGraphStructure(string content)
{
    var structure = new GraphStructure();
    
    // Parse Classes
    var classMatches = Regex.Matches(content, @"class\s+(\w+)\s*(?::\s*(\w+))?");
    foreach (Match match in classMatches)
    {
        var className = match.Groups[1].Value;
        var baseClass = match.Groups[2].Value;
        
        structure.Nodes.Add(new GraphNode {
            Type = "Class",
            Name = className,
            LineNumber = GetLineNumber(content, match.Index)
        });
        
        if (!string.IsNullOrEmpty(baseClass))
        {
            structure.Relationships.Add(new GraphRelationship {
                From = className,
                To = baseClass,
                Type = "EXTENDS"
            });
        }
    }
    
    // Parse Functions/Methods
    var functionMatches = Regex.Matches(content, @"(public|private|protected)?\s*(static)?\s*\w+\s+(\w+)\s*\(");
    foreach (Match match in functionMatches)
    {
        var functionName = match.Groups[3].Value;
        structure.Nodes.Add(new GraphNode {
            Type = "Function",
            Name = functionName,
            LineNumber = GetLineNumber(content, match.Index)
        });
    }
    
    return structure;
}
```

#### EstimateRelationshipsAsync(content)
Zählt Beziehungen
```csharp
public async Task<int> EstimateRelationshipsAsync(string content)
{
    var relationships = 0;
    
    // Imports
    relationships += Regex.Matches(content, @"\bimport\s+").Count;
    relationships += Regex.Matches(content, @"\busing\s+").Count;
    relationships += Regex.Matches(content, @"\brequire\s*\(").Count;
    relationships += Regex.Matches(content, @"\bfrom\s+\w+\s+import").Count;
    
    return relationships;
}
```

#### CalculateImpactScoreAsync(content)
Impact-Berechnung
```csharp
public async Task<double> CalculateImpactScoreAsync(string content)
{
    var score = 0.4; // Base
    
    var nodes = await EstimateGraphNodesAsync(content);
    var relationships = await EstimateRelationshipsAsync(content);
    
    // High complexity = high impact
    if (nodes > 5) score += 0.2;
    if (relationships > 3) score += 0.2;
    if (content.Length > 5000) score += 0.2;
    
    return Math.Min(1.0, score);
}
```

### 4. IngestionPipelineService

**Zweck**: Orchestrierung aller Services

#### ExecutePipelineAsync
Haupt-Pipeline-Logik
```csharp
public async Task<IngestionPipelineResult> ExecutePipelineAsync(
    string sourceFolder,
    bool isDryRun,
    IProgress<PipelineStage>? progress = null,
    IProgress<FileAnalysisResult>? fileProgress = null)
{
    _cancellationTokenSource = new CancellationTokenSource();
    var result = new IngestionPipelineResult { IsDryRun = isDryRun };
    var stopwatch = Stopwatch.StartNew();

    try
    {
        // Stage 1: File Collection
        progress?.Report(new PipelineStage {
            Name = "Datei-Sammlung",
            Description = "Sammle Dateien...",
            ProcessedCount = 0,
            TotalCount = 0
        });

        var files = Directory.GetFiles(sourceFolder, "*.*", SearchOption.AllDirectories)
            .Where(f => IsAnalyzableFile(f))
            .ToArray();

        result.TotalFiles = files.Length;

        // Stage 2: LLM Initialization
        progress?.Report(new PipelineStage {
            Name = "LLM-Initialisierung",
            Description = "Prüfe LLM...",
            ProcessedCount = 0,
            TotalCount = 1
        });

        var llamaAvailable = await _llamaService.IsAvailableAsync();

        // Stage 3: File Analysis
        for (int i = 0; i < files.Length; i++)
        {
            if (_cancellationTokenSource.Token.IsCancellationRequested)
                break;

            var filePath = files[i];
            var fileStopwatch = Stopwatch.StartNew();

            progress?.Report(new PipelineStage {
                Name = "Analyse",
                Description = $"Analysiere: {Path.GetFileName(filePath)}",
                ProcessedCount = i,
                TotalCount = files.Length
            });

            try
            {
                var analysisResult = await AnalyzeFileAsync(filePath, llamaAvailable);
                analysisResult.ProcessingTime = fileStopwatch.Elapsed;

                // Classify result
                if (analysisResult.IsDuplicate)
                    result.DuplicateFiles++;
                else if (!isDryRun)
                    result.ProcessedFiles++;
                else
                    result.ProcessedFiles++;

                result.Results.Add(analysisResult);
                
                // Live update
                if (analysisResult.IsProcessed && !analysisResult.IsDuplicate)
                {
                    fileProgress?.Report(analysisResult);
                }
            }
            catch (Exception ex)
            {
                result.ErrorFiles++;
                result.Results.Add(new FileAnalysisResult {
                    FilePath = filePath,
                    FileName = Path.GetFileName(filePath),
                    ErrorMessage = ex.Message,
                    IsProcessed = false
                });
            }
        }

        // Stage 4: Completion
        progress?.Report(new PipelineStage {
            Name = "Abschluss",
            Description = isDryRun ? "DryRun abgeschlossen" : "Ingestion abgeschlossen",
            ProcessedCount = result.ProcessedFiles,
            TotalCount = result.TotalFiles,
            IsComplete = true
        });

        result.TotalTime = stopwatch.Elapsed;
    }
    finally
    {
        _cancellationTokenSource?.Dispose();
    }

    return result;
}
```

#### AnalyzeFileAsync
Einzelne Datei-Analyse
```csharp
private async Task<FileAnalysisResult> AnalyzeFileAsync(string filePath, bool llamaAvailable)
{
    var result = new FileAnalysisResult {
        FilePath = filePath,
        FileName = Path.GetFileName(filePath),
        AnalysisTimestamp = DateTime.Now
    };

    var fileInfo = new FileInfo(filePath);
    result.FileSize = fileInfo.Length;
    result.FileType = fileInfo.Extension;

    // Read content
    var content = await File.ReadAllTextAsync(filePath);
    
    // Calculate hash for duplicate detection
    result.ContentHash = CalculateHash(content);
    result.IsDuplicate = !_processedHashes.Add(result.ContentHash);

    if (!result.IsDuplicate)
    {
        // Metadata
        result.Metadata = _nlpService.ExtractMetadata(filePath);

        // NLP Analysis
        result.Language = await _nlpService.DetectLanguageAsync(content);
        result.Topics = await _nlpService.ExtractTopicsAsync(content);
        result.QualityScore = await _nlpService.CalculateQualityScoreAsync(content);

        // Graph Analysis
        result.GraphNodeCount = await _graphService.EstimateGraphNodesAsync(content);
        result.RelationshipCount = await _graphService.EstimateRelationshipsAsync(content);
        result.ImpactScore = await _graphService.CalculateImpactScoreAsync(content);

        // LLM Analysis (optional)
        if (llamaAvailable && content.Length < 100000)
        {
            result.Summary = await _llamaService.GenerateSummaryAsync(content);
            result.Keywords = await _llamaService.ExtractKeywordsAsync(content);
            result.ExtractedEntities = await _llamaService.ExtractEntitiesAsync(content);
            result.RelevanceScore = await _llamaService.CalculateRelevanceScoreAsync(content);
        }
        else
        {
            result.RelevanceScore = 0.5;
        }

        result.IsProcessed = true;
    }

    return result;
}
```

## Performance-Optimierung

### Parallel Processing (TODO)
```csharp
// Aktuell: Sequenziell
// TODO: Parallel mit SemaphoreSlim

var semaphore = new SemaphoreSlim(4); // Max 4 parallel
var tasks = files.Select(async file => {
    await semaphore.WaitAsync();
    try {
        return await AnalyzeFileAsync(file, llamaAvailable);
    } finally {
        semaphore.Release();
    }
});

var results = await Task.WhenAll(tasks);
```

### Caching
```csharp
// Cache für häufige Keywords
private static readonly Dictionary<string, List<string>> _keywordCache = new();

public async Task<List<string>> ExtractKeywordsAsync(string content)
{
    var hash = CalculateHash(content);
    if (_keywordCache.TryGetValue(hash, out var cached))
        return cached;
    
    var keywords = await ExtractKeywordsInternalAsync(content);
    _keywordCache[hash] = keywords;
    return keywords;
}
```

### Batch-Processing
```csharp
// Gruppe Files nach Type für optimierte Verarbeitung
var groupedFiles = files.GroupBy(f => Path.GetExtension(f));

foreach (var group in groupedFiles)
{
    // Optimiere Analyse-Strategy per Type
    var strategy = GetAnalysisStrategy(group.Key);
    await strategy.AnalyzeBatchAsync(group);
}
```

## Best Practices

1. **Error-Resilience**: Einzelne Fehler stoppen Pipeline nicht
2. **Progress-Reporting**: Updates pro Datei und pro Stage
3. **Cancellation**: Graceful shutdown mit CancellationToken
4. **Resource-Management**: Dispose von Streams und Connections
5. **Logging**: Detaillierte Logs für Debugging
6. **Metrics**: Time-Series für Performance-Tracking
