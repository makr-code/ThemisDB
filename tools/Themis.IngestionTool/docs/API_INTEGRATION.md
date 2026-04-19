> **Hinweis:** API-Signaturen gegen aktuelle Endpunkte/Typen prüfen. Abweichungen mit `<!-- TODO: verify API -->` markieren.

> **Historischer Stand:** 2026-04-19
> <!-- TODO: verify against current source – API endpoint paths and request/response shapes below may have changed -->

# ThemisDB API-Integration

## Übersicht

Dieses Dokument beschreibt die Integration mit der ThemisDB Multi-Model-Datenbank und zeigt, wie das Ingestion Tool die verschiedenen APIs nutzt.

## ThemisDB Capabilities

ThemisDB ist eine Multi-Model-Datenbank mit folgenden Hauptfunktionen:

### 1. Entity Storage (Relational)
- Key-Value-basierte Entity-Speicherung
- ACID-Transaktionen
- MVCC (Multi-Version Concurrency Control)

### 2. Graph Database
- Property Graph Model
- Traversal-Operationen
- Beziehungen zwischen Entities

### 3. Vector Search
- High-dimensional Vector Storage
- HNSW (Hierarchical Navigable Small World) Index
- Similarity Search (COSINE, L2, IP)

### 4. Time-Series Data
- Temporal Data Storage
- Aggregation Functions
- Time-based Queries

### 5. Semantic Cache
- LLM Response Caching
- Similarity-based Lookup
- Performance Optimization

### 6. ContentFS
- Binary Content Storage
- Range-Request-Support
- ETag-basiertes Caching

## API Endpoints

### Health Check
```http
GET /health
```

**Response**:
```json
{
  "database": "themis",
  "status": "healthy",
  "uptime_seconds": 12345,
  "version": "1.3.4"
}
```

**Verwendung im Tool**:
- Automatischer Heartbeat alle 5 Sekunden
- Connection-Status-Anzeige (Online/Offline)
- Timeout: 3 Sekunden

### Entity Management

#### Create/Update Entity
```http
POST /entities
Content-Type: application/json

{
  "key": "file:abc123",
  "data": {
    "filename": "Program.cs",
    "path": "C:\\src\\Program.cs",
    "size": 5420,
    "type": ".cs",
    "relevance": 0.85,
    "impact": 0.72,
    "quality": 0.91,
    "language": "en",
    "topics": ["Architecture"],
    "keywords": ["service", "dependency", "injection"]
  }
}
```

#### Get Entity
```http
GET /entities/:key
```

#### Delete Entity
```http
DELETE /entities/:key
```

**Best Practice für Ingestion**:
```csharp
// Eindeutiger Key aus ContentHash
var key = $"file:{result.ContentHash}";

// Strukturierte Daten
var entityData = new {
    filename = result.FileName,
    path = result.FilePath,
    size = result.FileSize,
    type = result.FileType,
    relevance = result.RelevanceScore,
    impact = result.ImpactScore,
    quality = result.QualityScore,
    graph_nodes = result.GraphNodeCount,
    relationships = result.RelationshipCount,
    language = result.Language,
    topics = result.Topics,
    keywords = result.Keywords,
    entities = result.ExtractedEntities,
    summary = result.Summary,
    metadata = result.Metadata,
    analyzed_at = result.AnalysisTimestamp,
    processing_time_ms = result.ProcessingTime.TotalMilliseconds
};

await themisClient.PostAsync("/entities", new {
    key = key,
    data = entityData
});
```

### Graph Operations

#### Create Relationship
```http
POST /graph/relationship
Content-Type: application/json

{
  "from": "file:abc123",
  "to": "file:def456",
  "type": "IMPORTS",
  "properties": {
    "line": 5,
    "import_name": "System.IO"
  }
}
```

#### Graph Traversal
```http
POST /graph/traverse
Content-Type: application/json

{
  "start": "file:abc123",
  "direction": "out",
  "relationship_type": "IMPORTS",
  "max_depth": 3
}
```

**Relationship-Typen für Code-Analyse**:
- `IMPORTS`: Import/Using-Statements
- `DEPENDS_ON`: Dependency
- `CALLS`: Funktionsaufruf
- `EXTENDS`: Vererbung
- `IMPLEMENTS`: Interface-Implementation
- `CONTAINS`: Datei → Klasse/Funktion
- `REFERENCES`: Allgemeine Referenz

**Best Practice**:
```csharp
// Erstelle Graph-Beziehungen nach Analyse
foreach (var import in DetectImports(content)) {
    await CreateRelationship(new {
        from = currentFileKey,
        to = $"module:{import.ModuleName}",
        type = "IMPORTS",
        properties = new {
            line = import.LineNumber,
            import_name = import.ModuleName
        }
    });
}

// Erstelle hierarchische Struktur
foreach (var classNode in result.GraphNodes) {
    await CreateRelationship(new {
        from = currentFileKey,
        to = $"class:{classNode.Name}",
        type = "CONTAINS",
        properties = new {
            start_line = classNode.StartLine,
            end_line = classNode.EndLine
        }
    });
}
```

### Vector Search

#### Store Vector
```http
POST /vector/store
Content-Type: application/json

{
  "object_name": "documents",
  "pk": "file:abc123",
  "vector": [0.12, 0.45, -0.23, ...],
  "metadata": {
    "filename": "Program.cs",
    "relevance": 0.85
  }
}
```

#### Similarity Search
```http
POST /vector/search
Content-Type: application/json

{
  "object_name": "documents",
  "query_vector": [0.11, 0.44, -0.24, ...],
  "top_k": 10,
  "filter": {
    "relevance": { "$gte": 0.7 }
  }
}
```

**Best Practice für Embeddings**:
```csharp
// Generiere Embedding (via LLM oder Embedding-Service)
var embedding = await GenerateEmbeddingAsync(result.Summary);

// Speichere Vector mit Metadaten
await themisClient.PostAsync("/vector/store", new {
    object_name = "documents",
    pk = $"file:{result.ContentHash}",
    vector = embedding,
    metadata = new {
        filename = result.FileName,
        relevance = result.RelevanceScore,
        impact = result.ImpactScore,
        quality = result.QualityScore,
        topics = result.Topics,
        language = result.Language
    }
});
```

**Embedding-Dimensionen**:
- ThemisDB Default: 10 (konfigurierbar)
- OpenAI text-embedding-3-small: 1536
- Sentence-Transformers: 384-768
- Anpassen in ThemisDB Config: `vector_index.dimension`

### Time-Series Data

#### Store Time-Series
```http
POST /ts/put
Content-Type: application/json

{
  "metric": "file.analysis.duration",
  "timestamp": 1704124800000,
  "value": 156.5,
  "tags": {
    "file": "Program.cs",
    "stage": "nlp"
  }
}
```

#### Query Time-Series
```http
POST /ts/query
Content-Type: application/json

{
  "metric": "file.analysis.duration",
  "start": 1704124800000,
  "end": 1704211200000,
  "aggregation": "avg",
  "group_by": ["stage"]
}
```

**Best Practice für Performance-Tracking**:
```csharp
// Tracke Analyse-Performance pro Stage
await StoreTimeSeriesAsync(new {
    metric = "ingestion.file.processed",
    timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
    value = 1,
    tags = new {
        filename = result.FileName,
        file_type = result.FileType,
        size_kb = result.FileSize / 1024,
        relevance_category = GetRelevanceCategory(result.RelevanceScore)
    }
});

// Tracke Processing-Time
await StoreTimeSeriesAsync(new {
    metric = "ingestion.processing.duration",
    timestamp = result.AnalysisTimestamp,
    value = result.ProcessingTime.TotalMilliseconds,
    tags = new {
        file_type = result.FileType,
        graph_nodes = result.GraphNodeCount,
        has_llm = result.Summary != null
    }
});
```

### Transaction Support

#### Execute Transaction
```http
POST /transaction
Content-Type: application/json

{
  "operations": [
    {
      "type": "put",
      "key": "file:abc123",
      "value": { ... }
    },
    {
      "type": "graph_relationship",
      "from": "file:abc123",
      "to": "file:def456",
      "rel_type": "IMPORTS"
    },
    {
      "type": "vector_store",
      "object_name": "documents",
      "pk": "file:abc123",
      "vector": [...]
    }
  ]
}
```

**Best Practice**:
Nutzen Sie Transaktionen für atomare Multi-Model-Operationen:

```csharp
// Alle Operationen für eine Datei in einer Transaktion
var transaction = new {
    operations = new[] {
        // 1. Entity speichern
        new {
            type = "put",
            key = $"file:{result.ContentHash}",
            value = entityData
        },
        // 2. Graph-Beziehungen erstellen
        ...imports.Select(imp => new {
            type = "graph_relationship",
            from = $"file:{result.ContentHash}",
            to = $"module:{imp}",
            rel_type = "IMPORTS"
        }),
        // 3. Vector speichern
        new {
            type = "vector_store",
            object_name = "documents",
            pk = $"file:{result.ContentHash}",
            vector = embedding
        },
        // 4. Time-Series-Event
        new {
            type = "ts_put",
            metric = "ingestion.file.processed",
            timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
            value = 1
        }
    }
};

await themisClient.PostAsync("/transaction", transaction);
```

## Implementierungs-Roadmap

### Phase 1: Basic Entity Storage (Aktuell TODO)
```csharp
public interface IThemisApiService
{
    Task<bool> StoreEntityAsync(string key, object data);
    Task<T?> GetEntityAsync<T>(string key);
    Task<bool> DeleteEntityAsync(string key);
}
```

### Phase 2: Graph Relationships
```csharp
public interface IThemisGraphService
{
    Task<bool> CreateRelationshipAsync(string from, string to, string type, object? properties = null);
    Task<GraphTraversalResult> TraverseAsync(string start, string direction, string? relType = null, int maxDepth = 3);
}
```

### Phase 3: Vector Integration
```csharp
public interface IThemisVectorService
{
    Task<bool> StoreVectorAsync(string objectName, string pk, float[] vector, object? metadata = null);
    Task<VectorSearchResult[]> SearchAsync(string objectName, float[] queryVector, int topK = 10, object? filter = null);
}
```

### Phase 4: Time-Series Tracking
```csharp
public interface IThemisTimeSeriesService
{
    Task<bool> PutAsync(string metric, long timestamp, double value, object? tags = null);
    Task<TimeSeriesResult> QueryAsync(string metric, long start, long end, string? aggregation = null);
}
```

### Phase 5: Transactions
```csharp
public interface IThemisTransactionService
{
    Task<bool> ExecuteAsync(params TransactionOperation[] operations);
}
```

## Erweiterte Ingestion-Strategie

### Multi-Model Data Flow

```
┌─────────────────────────────────────────────────────────┐
│               File Analysis Result                      │
└────────────────┬────────────────────────────────────────┘
                 │
                 ├──► Entity Storage (Relational)
                 │    • File Metadata
                 │    • Analysis Results
                 │    • Scores & Metrics
                 │
                 ├──► Graph Database
                 │    • File → Module (IMPORTS)
                 │    • File → Class (CONTAINS)
                 │    • Class → Function (CONTAINS)
                 │    • Function → Function (CALLS)
                 │
                 ├──► Vector Store
                 │    • Summary Embedding
                 │    • Semantic Search
                 │    • Similarity Grouping
                 │
                 ├──► Time-Series
                 │    • Processing Duration
                 │    • File Count Metrics
                 │    • Quality Trends
                 │
                 └──► ContentFS (optional)
                      • Original File Content
                      • Binary Artifacts
```

### Konkrete Implementierung

```csharp
public async Task IngestFileToThemisAsync(FileAnalysisResult result, byte[] content)
{
    var key = $"file:{result.ContentHash}";
    
    // 1. Generate Embedding
    var embedding = await _embeddingService.GenerateAsync(result.Summary);
    
    // 2. Build Transaction
    var operations = new List<object>();
    
    // Entity
    operations.Add(new {
        type = "put",
        key = key,
        value = new {
            filename = result.FileName,
            path = result.FilePath,
            size = result.FileSize,
            type = result.FileType,
            hash = result.ContentHash,
            relevance = result.RelevanceScore,
            impact = result.ImpactScore,
            quality = result.QualityScore,
            language = result.Language,
            topics = result.Topics,
            keywords = result.Keywords,
            entities = result.ExtractedEntities,
            summary = result.Summary,
            graph_nodes = result.GraphNodeCount,
            relationships = result.RelationshipCount,
            analyzed_at = result.AnalysisTimestamp,
            processing_time_ms = result.ProcessingTime.TotalMilliseconds
        }
    });
    
    // Graph: File → Topics
    foreach (var topic in result.Topics) {
        operations.Add(new {
            type = "graph_relationship",
            from = key,
            to = $"topic:{topic.ToLower()}",
            rel_type = "HAS_TOPIC",
            properties = new { score = 1.0 }
        });
    }
    
    // Graph: File → Entities
    foreach (var entity in result.ExtractedEntities) {
        operations.Add(new {
            type = "graph_relationship",
            from = key,
            to = $"entity:{entity}",
            rel_type = "CONTAINS_ENTITY",
            properties = new { }
        });
    }
    
    // Vector
    operations.Add(new {
        type = "vector_store",
        object_name = "documents",
        pk = key,
        vector = embedding,
        metadata = new {
            filename = result.FileName,
            relevance = result.RelevanceScore,
            topics = result.Topics,
            language = result.Language
        }
    });
    
    // Time-Series
    operations.Add(new {
        type = "ts_put",
        metric = "ingestion.file.processed",
        timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds(),
        value = 1,
        tags = new {
            file_type = result.FileType,
            language = result.Language,
            relevance_category = GetCategory(result.RelevanceScore)
        }
    });
    
    // Execute Transaction
    await _themisClient.PostAsync("/transaction", new {
        operations = operations
    });
    
    // Optional: Store Content in ContentFS
    if (content.Length < 10 * 1024 * 1024) { // Max 10 MB
        await _themisClient.PutAsync($"/contentfs/{key}", content);
    }
}
```

## Query-Beispiele

### Finde ähnliche Dateien
```http
POST /vector/search
{
  "object_name": "documents",
  "query_vector": [...],
  "top_k": 10,
  "filter": {
    "language": "en",
    "relevance": { "$gte": 0.7 }
  }
}
```

### Finde alle Dependencies
```http
POST /graph/traverse
{
  "start": "file:abc123",
  "direction": "out",
  "relationship_type": "IMPORTS",
  "max_depth": 5
}
```

### Analyse-Performance über Zeit
```http
POST /ts/aggregate
{
  "metric": "ingestion.processing.duration",
  "start": 1704124800000,
  "end": 1704211200000,
  "aggregation": "avg",
  "interval": "1h",
  "group_by": ["file_type"]
}
```

## Error Handling

### Retry-Strategie mit Polly
```csharp
var retryPolicy = Policy
    .Handle<HttpRequestException>()
    .WaitAndRetryAsync(3, retryAttempt => 
        TimeSpan.FromSeconds(Math.Pow(2, retryAttempt)));

await retryPolicy.ExecuteAsync(async () => {
    await _themisClient.PostAsync("/entities", data);
});
```

### Circuit Breaker
```csharp
var circuitBreaker = Policy
    .Handle<HttpRequestException>()
    .CircuitBreakerAsync(5, TimeSpan.FromMinutes(1));
```

## Performance-Optimierung

### Batch-Operations
Gruppieren Sie Multiple Entities:
```csharp
var batch = new {
    entities = results.Select(r => new {
        key = $"file:{r.ContentHash}",
        data = MapToEntity(r)
    })
};

await _themisClient.PostAsync("/entities/batch", batch);
```

### Parallelisierung
```csharp
await Parallel.ForEachAsync(results, 
    new ParallelOptions { MaxDegreeOfParallelism = 4 },
    async (result, ct) => {
        await IngestFileToThemisAsync(result, ct);
    });
```

### Connection Pooling
```csharp
var handler = new SocketsHttpHandler {
    PooledConnectionLifetime = TimeSpan.FromMinutes(2),
    MaxConnectionsPerServer = 10
};

var client = new HttpClient(handler) {
    BaseAddress = new Uri("http://localhost:8765"),
    Timeout = TimeSpan.FromSeconds(30)
};
```
