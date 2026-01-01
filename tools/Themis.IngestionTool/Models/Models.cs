using System.Text.Json;

namespace Themis.IngestionTool.Models;

/// <summary>
/// Metadata für eine ingestierte Datei
/// </summary>
public record FileMetadata
{
    public string FilePath { get; init; } = string.Empty;
    public string FileHash { get; init; } = string.Empty;
    public long FileSize { get; init; }
    public string MimeType { get; init; } = string.Empty;
    public DateTime IngestionTime { get; init; }
    public Dictionary<string, object> Metadata { get; init; } = new();
    public ThemisMetadata ThemisMetadata { get; init; } = new();
}

/// <summary>
/// ThemisDB-spezifische Metadaten (Graph, Vector, Relational, Geo, Process)
/// </summary>
public record ThemisMetadata
{
    public GraphMetadata? Graph { get; init; }
    public VectorMetadata? Vector { get; init; }
    public RelationalMetadata? Relational { get; init; }
    public GeoMetadata? Geo { get; init; }
    public ProcessMetadata? Process { get; init; }
}

/// <summary>
/// Graph-Model Metadaten
/// </summary>
public record GraphMetadata
{
    public string EntityType { get; init; } = "Document";
    public string EntityId { get; init; } = string.Empty;
    public Dictionary<string, object> Properties { get; init; } = new();
    public List<Relationship> Relationships { get; init; } = new();
}

public record Relationship
{
    public string Type { get; init; } = string.Empty;
    public List<string> Targets { get; init; } = new();
}

/// <summary>
/// Vector-Model Metadaten (für Embeddings und semantische Suche)
/// </summary>
public record VectorMetadata
{
    public string ObjectName { get; init; } = "documents";
    public string DocumentId { get; init; } = string.Empty;
    public string ContentType { get; init; } = string.Empty;
    public bool EmbeddingRequired { get; init; } = true;
    public string? TextContent { get; init; }
    public int ContentLength { get; init; }
}

/// <summary>
/// Relational-Model Metadaten
/// </summary>
public record RelationalMetadata
{
    public string TableName { get; init; } = "ingested_documents";
    public Dictionary<string, string> Schema { get; init; } = new();
    public Dictionary<string, object> Record { get; init; } = new();
}

/// <summary>
/// Geo/Spatial Model Metadaten (Koordinaten, Adressen, Geometrien)
/// </summary>
public record GeoMetadata
{
    public bool HasGeometry { get; init; }
    public Dictionary<string, object> CoordinateFields { get; init; } = new();
    public Dictionary<string, object> AddressFields { get; init; } = new();
    public string? GeometryWkt { get; init; }
    public string? GeometryFormat { get; init; }
    public object? GeometryRaw { get; init; }
    public string? FullAddress { get; init; }
    public bool SpatialIndexRequired { get; init; }
    public string? IndexType { get; init; }
}

/// <summary>
/// Process-Aware Metadaten (BPMN, Workflows, State Machines)
/// </summary>
public record ProcessMetadata
{
    public bool IsProcessAware { get; init; }
    public Dictionary<string, object> ProcessFields { get; init; } = new();
    public bool HasState { get; init; }
    public bool HasVariables { get; init; }
    public bool HasTokens { get; init; }
    public bool IsProcessInstance { get; init; }
    public bool IsBpmn { get; init; }
    public bool IsStateMachine { get; init; }
    public string? ProcessType { get; init; }
    public string? Format { get; init; }
    public bool ProcessMiningReady { get; init; }
    public string? SuggestedCollection { get; init; }
    public object? Transitions { get; init; }
}

/// <summary>
/// Konfiguration für den Ingestion-Prozess
/// </summary>
public record IngestionConfig
{
    public string SourceDir { get; init; } = string.Empty;
    public string OutputFile { get; init; } = "ingestion_output.json";
    public string DbPath { get; init; } = "ingestion_tracker.db";
    
    public List<string> IncludeExtensions { get; init; } = new();
    public List<string> ExcludeExtensions { get; init; } = new() 
    { 
        ".exe", ".dll", ".so", ".dylib", ".bin", ".pdb", ".obj", ".o" 
    };
    public List<string> ExcludePatterns { get; init; } = new() 
    { 
        ".git", "__pycache__", "node_modules", ".venv", "bin", "obj" 
    };
    
    public double MaxFileSizeMb { get; init; } = 100.0;
    public bool ExtractTextPreview { get; init; } = true;
    public int PreviewLength { get; init; } = 500;
    
    public bool GenerateVectorMetadata { get; init; } = true;
    public bool GenerateGraphMetadata { get; init; } = true;
    public bool GenerateRelationalMetadata { get; init; } = true;
}

/// <summary>
/// Statistik für den Ingestion-Prozess
/// </summary>
public record IngestionStats
{
    public int TotalFilesScanned { get; set; }
    public int FilesProcessed { get; set; }
    public int FilesSkipped { get; set; }
    public int FilesFailed { get; set; }
    public long TotalSizeBytes { get; set; }
    public double ElapsedSeconds { get; set; }
}

/// <summary>
/// Ergebnis des Ingestion-Prozesses
/// </summary>
public record IngestionResult
{
    public IngestionMetadataInfo Metadata { get; init; } = new();
    public IngestionStats Statistics { get; init; } = new();
    public List<FileMetadata> IngestedFiles { get; init; } = new();
}

public record IngestionMetadataInfo
{
    public DateTime IngestionTime { get; init; }
    public string SourceDirectory { get; init; } = string.Empty;
    public IngestionConfig Config { get; init; } = new();
}
