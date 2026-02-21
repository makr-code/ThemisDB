/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Comment.cs                                         ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     209                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Domain.Collaboration;

/// <summary>
/// Repräsentiert einen Kommentar zu einem Dokument.
/// Unterstützt Thread-basierte Diskussionen und @Mentions.
/// </summary>
public class Comment
{
    /// <summary>
    /// Eindeutige ID des Kommentars
    /// </summary>
    public string Id { get; set; } = Guid.NewGuid().ToString();

    /// <summary>
    /// ID des Dokuments, zu dem der Kommentar gehört
    /// </summary>
    public string DocumentId { get; set; } = string.Empty;

    /// <summary>
    /// ID des übergeordneten Kommentars (null für Top-Level Kommentare)
    /// </summary>
    public string? ParentCommentId { get; set; }

    /// <summary>
    /// ID des Thread-Roots (für verschachtelte Kommentare)
    /// </summary>
    public string? ThreadId { get; set; }

    /// <summary>
    /// ID des Autors
    /// </summary>
    public string AuthorId { get; set; } = string.Empty;

    /// <summary>
    /// Name des Autors
    /// </summary>
    public string AuthorName { get; set; } = string.Empty;

    /// <summary>
    /// Inhalt des Kommentars
    /// </summary>
    public string Content { get; set; } = string.Empty;

    /// <summary>
    /// Zeitpunkt der Erstellung
    /// </summary>
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Zeitpunkt der letzten Änderung
    /// </summary>
    public DateTime? UpdatedAt { get; set; }

    /// <summary>
    /// Wurde der Kommentar bearbeitet?
    /// </summary>
    public bool IsEdited => UpdatedAt.HasValue;

    /// <summary>
    /// Erwähnte Benutzer (@mentions)
    /// </summary>
    public List<string> MentionedUserIds { get; set; } = new();

    /// <summary>
    /// Wurde der Kommentar gelöscht? (Soft delete)
    /// </summary>
    public bool IsDeleted { get; set; }

    /// <summary>
    /// Zeitpunkt der Löschung
    /// </summary>
    public DateTime? DeletedAt { get; set; }

    /// <summary>
    /// Anzahl der Antworten auf diesen Kommentar
    /// </summary>
    public int ReplyCount { get; set; }

    /// <summary>
    /// Reaktionen auf den Kommentar (z.B. Likes, Emoji)
    /// </summary>
    public List<CommentReaction> Reactions { get; set; } = new();

    /// <summary>
    /// Anhänge (z.B. Screenshots, Dateien)
    /// </summary>
    public List<CommentAttachment> Attachments { get; set; } = new();

    /// <summary>
    /// Seitenposition oder Bereich im Dokument (optional)
    /// </summary>
    public DocumentPosition? Position { get; set; }
}

/// <summary>
/// Repräsentiert eine Reaktion auf einen Kommentar
/// </summary>
public class CommentReaction
{
    /// <summary>
    /// ID des Benutzers
    /// </summary>
    public string UserId { get; set; } = string.Empty;

    /// <summary>
    /// Art der Reaktion (z.B. "like", "👍", "❤️")
    /// </summary>
    public string Type { get; set; } = string.Empty;

    /// <summary>
    /// Zeitpunkt der Reaktion
    /// </summary>
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// Repräsentiert einen Anhang zu einem Kommentar
/// </summary>
public class CommentAttachment
{
    /// <summary>
    /// ID des Anhangs
    /// </summary>
    public string Id { get; set; } = Guid.NewGuid().ToString();

    /// <summary>
    /// Dateiname
    /// </summary>
    public string FileName { get; set; } = string.Empty;

    /// <summary>
    /// MIME-Type
    /// </summary>
    public string ContentType { get; set; } = string.Empty;

    /// <summary>
    /// Dateigröße in Bytes
    /// </summary>
    public long Size { get; set; }

    /// <summary>
    /// URL oder Pfad zum Anhang
    /// </summary>
    public string Url { get; set; } = string.Empty;

    /// <summary>
    /// Hochladezeitpunkt
    /// </summary>
    public DateTime UploadedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// Repräsentiert eine Position oder Bereich im Dokument
/// </summary>
public class DocumentPosition
{
    /// <summary>
    /// Seitennummer (für PDF etc.)
    /// </summary>
    public int? Page { get; set; }

    /// <summary>
    /// Start-Offset im Dokument (für Text)
    /// </summary>
    public int? StartOffset { get; set; }

    /// <summary>
    /// End-Offset im Dokument (für Text)
    /// </summary>
    public int? EndOffset { get; set; }

    /// <summary>
    /// Koordinaten (für visuelle Markierungen)
    /// </summary>
    public BoundingBox? BoundingBox { get; set; }
}

/// <summary>
/// Repräsentiert ein Rechteck für visuelle Markierungen
/// </summary>
public class BoundingBox
{
    public double X { get; set; }
    public double Y { get; set; }
    public double Width { get; set; }
    public double Height { get; set; }
}
