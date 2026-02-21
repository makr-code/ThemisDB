/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Document.cs                                        ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     134                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Represents a document in the ThemisDB system
/// </summary>
public class Document
{
    public string Id { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string MimeType { get; set; } = string.Empty;
    public string Filename { get; set; } = string.Empty;
    public long SizeBytes { get; set; }
    public DateTime CreatedAt { get; set; }
    public DateTime ModifiedAt { get; set; }
    public string Author { get; set; } = string.Empty;
    public Dictionary<string, object> Metadata { get; set; } = new();
    public List<string> Tags { get; set; } = new();
    
    // Geo properties
    public GeoLocation? Location { get; set; }
    
    // Vector embedding
    public float[]? Embedding { get; set; }
    
    // Content
    public string? ContentPreview { get; set; }
    public string? BlobPath { get; set; }
    
    // Classification
    public string? Classification { get; set; }
    public string? Category { get; set; }
}

/// <summary>
/// Geographic location information
/// </summary>
public class GeoLocation
{
    public double Latitude { get; set; }
    public double Longitude { get; set; }
    public string? Address { get; set; }
    public string? Country { get; set; }
    public string? City { get; set; }
}

/// <summary>
/// Document chunk for semantic search
/// </summary>
public class DocumentChunk
{
    public string Id { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty;
    public int ChunkIndex { get; set; }
    public string Text { get; set; } = string.Empty;
    public int StartOffset { get; set; }
    public int EndOffset { get; set; }
    public float[]? Embedding { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Document relation for graph visualization
/// </summary>
public class DocumentRelation
{
    public string Id { get; set; } = string.Empty;
    public string FromDocumentId { get; set; } = string.Empty;
    public string ToDocumentId { get; set; } = string.Empty;
    public string RelationType { get; set; } = string.Empty;
    public double Weight { get; set; } = 1.0;
    public Dictionary<string, object> Properties { get; set; } = new();
}

/// <summary>
/// Timeline event for temporal visualization
/// </summary>
public class TimelineEvent
{
    public string Id { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; }
    public string EventType { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Search result with relevance score
/// </summary>
public class SearchResult
{
    public Document Document { get; set; } = new();
    public double Score { get; set; }
    public string? MatchedText { get; set; }
    public SearchResultType ResultType { get; set; }
}

public enum SearchResultType
{
    FullText,
    Vector,
    Hybrid,
    Graph
}
