/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentPreviewModels.cs                           ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     314                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 60d127110  2025-12-09  feat: Add comprehensive test report for ThemisDB Document... ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
    • 031e0bfe3  2025-12-08  Add modular document preview system with multi-format sup... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

#nullable enable

/// <summary>
/// Document preview information
/// </summary>
public class DocumentPreview
{
    public string DocumentId { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public DocumentPreviewType Type { get; set; }
    public string ContentType { get; set; } = string.Empty;
    public long SizeInBytes { get; set; }
    public DateTime LastModified { get; set; }
    public string? ThumbnailUrl { get; set; }
    public List<PreviewPage> Pages { get; set; } = new();
    public DocumentMetadata Metadata { get; set; } = new();
    public bool IsPreviewAvailable { get; set; }
    public string? PreviewError { get; set; }
}

public enum DocumentPreviewType
{
    Image,
    PDF,
    Word,
    Excel,
    PowerPoint,
    Text,
    Email,
    Video,
    Audio,
    Archive,
    Unknown
}

/// <summary>
/// Single page in document preview
/// </summary>
public class PreviewPage
{
    public int PageNumber { get; set; }
    public string? ImageUrl { get; set; }
    public string? ThumbnailUrl { get; set; }
    public int Width { get; set; }
    public int Height { get; set; }
    public string? ExtractedText { get; set; }
    public List<PreviewAnnotation> Annotations { get; set; } = new();
}

/// <summary>
/// Annotation on preview page (highlights, comments, etc.)
/// </summary>
public class PreviewAnnotation
{
    public string Id { get; set; } = string.Empty;
    public AnnotationType Type { get; set; }
    public double X { get; set; }
    public double Y { get; set; }
    public double Width { get; set; }
    public double Height { get; set; }
    public string? Text { get; set; }
    public string? Color { get; set; }
    public string? CreatedBy { get; set; }
    public DateTime CreatedAt { get; set; }
}

public enum AnnotationType
{
    Highlight,
    Comment,
    Redaction,
    Signature,
    Stamp
}

/// <summary>
/// Preview module configuration
/// </summary>
public class PreviewModuleConfig
{
    public bool EnableThumbnails { get; set; } = true;
    public int ThumbnailWidth { get; set; } = 200;
    public int ThumbnailHeight { get; set; } = 280;
    public bool EnableTextExtraction { get; set; } = true;
    public bool EnableAnnotations { get; set; } = true;
    public int MaxPages { get; set; } = 100;
    public PreviewQuality Quality { get; set; } = PreviewQuality.Medium;
    public bool CachePreview { get; set; } = true;
    public TimeSpan CacheDuration { get; set; } = TimeSpan.FromDays(7);
}

public enum PreviewQuality
{
    Low,    // 72 DPI
    Medium, // 150 DPI
    High    // 300 DPI
}

/// <summary>
/// Word document preview content
/// </summary>
public class WordPreviewContent
{
    public string? Title { get; set; }
    public string? Author { get; set; }
    public int PageCount { get; set; }
    public int WordCount { get; set; }
    public List<string> Headings { get; set; } = new();
    public List<WordTable> Tables { get; set; } = new();
    public List<string> Images { get; set; } = new();
    public Dictionary<string, string> CustomProperties { get; set; } = new();
}

/// <summary>
/// Rich document metadata from ThemisDB including vector embeddings.
/// </summary>
public class DocumentMetadata
{
    public string Name { get; set; } = string.Empty;
    public string ContentType { get; set; } = string.Empty;
    public long Size { get; set; }
    public DateTime ModifiedAt { get; set; }
    public DateTime CreatedAt { get; set; }
    public string? Author { get; set; }

    public string? ExtractedText { get; set; }
    public string? Summary { get; set; }
    public List<string>? Tags { get; set; }
    public List<string>? Topics { get; set; }
    public List<string>? Categories { get; set; }
    public string? Language { get; set; }
    public int? PageCount { get; set; }
    public int? WordCount { get; set; }

    public List<DocumentEntity>? Entities { get; set; }
    public SentimentAnalysis? Sentiment { get; set; }
    public double? Confidence { get; set; }

    public float[]? VectorEmbedding { get; set; }
    public Dictionary<string, object>? VectorMetadata { get; set; }

    public List<SimilarDocument>? SimilarDocuments { get; set; }
}

public class DocumentEntity
{
    public string Text { get; set; } = string.Empty;
    public string Type { get; set; } = string.Empty;
    public double Confidence { get; set; }
    public int StartPosition { get; set; }
    public int EndPosition { get; set; }
}

public class SentimentAnalysis
{
    public string Overall { get; set; } = "Neutral";
    public double Score { get; set; }
    public Dictionary<string, double>? AspectScores { get; set; }
}

public class SimilarDocument
{
    public string DocumentId { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public double Similarity { get; set; }
    public string? Summary { get; set; }
}

public class WordTable
{
    public int Index { get; set; }
    public int Rows { get; set; }
    public int Columns { get; set; }
    public string? Caption { get; set; }
}

/// <summary>
/// Excel document preview content
/// </summary>
public class ExcelPreviewContent
{
    public List<ExcelSheet> Sheets { get; set; } = new();
    public int TotalRows { get; set; }
    public int TotalColumns { get; set; }
    public List<string> Charts { get; set; } = new();
    public List<string> PivotTables { get; set; } = new();
}

public class ExcelSheet
{
    public string Name { get; set; } = string.Empty;
    public int Index { get; set; }
    public int UsedRows { get; set; }
    public int UsedColumns { get; set; }
    public string? PreviewImageUrl { get; set; }
    public List<List<string>> PreviewData { get; set; } = new(); // First 10 rows
}

/// <summary>
/// PowerPoint document preview content
/// </summary>
public class PowerPointPreviewContent
{
    public int SlideCount { get; set; }
    public string? Theme { get; set; }
    public List<PowerPointSlide> Slides { get; set; } = new();
}

public class PowerPointSlide
{
    public int Index { get; set; }
    public string? Title { get; set; }
    public string? Layout { get; set; }
    public string? ThumbnailUrl { get; set; }
    public string? Notes { get; set; }
    public int ShapeCount { get; set; }
}

/// <summary>
/// Email preview content
/// </summary>
public class EmailPreviewContent
{
    public string? From { get; set; }
    public List<string> To { get; set; } = new();
    public List<string> Cc { get; set; } = new();
    public string? Subject { get; set; }
    public DateTime? SentDate { get; set; }
    public string? BodyPreview { get; set; }
    public bool IsHtml { get; set; }
    public List<EmailAttachment> Attachments { get; set; } = new();
    public int TotalSize { get; set; }
}

public class EmailAttachment
{
    public string Name { get; set; } = string.Empty;
    public string ContentType { get; set; } = string.Empty;
    public long Size { get; set; }
}

/// <summary>
/// PDF preview content
/// </summary>
public class PdfPreviewContent
{
    public int PageCount { get; set; }
    public string? Title { get; set; }
    public string? Author { get; set; }
    public string? Subject { get; set; }
    public string? Creator { get; set; }
    public DateTime? CreationDate { get; set; }
    public bool IsEncrypted { get; set; }
    public bool AllowPrinting { get; set; }
    public bool AllowCopying { get; set; }
    public List<PdfBookmark> Bookmarks { get; set; } = new();
    public bool HasForms { get; set; }
    public bool HasSignatures { get; set; }
}

public class PdfBookmark
{
    public string Title { get; set; } = string.Empty;
    public int PageNumber { get; set; }
    public int Level { get; set; }
    public List<PdfBookmark> Children { get; set; } = new();
}

/// <summary>
/// Preview rendering options
/// </summary>
public class PreviewRenderOptions
{
    public int? PageNumber { get; set; }
    public int? Width { get; set; }
    public int? Height { get; set; }
    public PreviewFormat Format { get; set; } = PreviewFormat.PNG;
    public bool IncludeAnnotations { get; set; } = true;
    public bool IncludeRedactions { get; set; } = false;
    public int? DPI { get; set; }
    public bool Antialias { get; set; } = true;
}

public enum PreviewFormat
{
    PNG,
    JPEG,
    WebP,
    SVG
}
