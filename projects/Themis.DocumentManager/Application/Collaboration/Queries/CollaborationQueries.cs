/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CollaborationQueries.cs                            ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     105                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea516e4de  2025-12-10  Address code review feedback: Move Result to Common, impr... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Domain.Collaboration;
using Themis.DocumentManager.Application.Common;

namespace Themis.DocumentManager.Application.Collaboration.Queries;

/// <summary>
/// Query zum Abrufen des Sperr-Status eines Dokuments.
/// </summary>
public record GetDocumentLockStatusQuery(
    string DocumentId
) : IRequest<Result<DocumentLock?>>;

/// <summary>
/// Query zum Abrufen aller aktiven Sperren.
/// Optional gefiltert nach Benutzer oder Dokument.
/// </summary>
public record GetActiveLocksQuery(
    string? UserId = null,
    string? DocumentId = null
) : IRequest<Result<List<DocumentLock>>>;

/// <summary>
/// Query zum Abrufen aller Kommentare zu einem Dokument.
/// Unterstützt Paginierung und Filterung.
/// </summary>
public record GetDocumentCommentsQuery(
    string DocumentId,
    string? ParentCommentId = null,
    bool IncludeDeleted = false,
    int PageNumber = 1,
    int PageSize = 50
) : IRequest<Result<CommentCollection>>;

/// <summary>
/// Query zum Abrufen eines einzelnen Kommentars.
/// </summary>
public record GetCommentQuery(
    string CommentId
) : IRequest<Result<Comment?>>;

/// <summary>
/// Query zum Abrufen aller Benutzer-Präsenzen für ein Dokument.
/// Zeigt, wer das Dokument gerade bearbeitet.
/// </summary>
public record GetDocumentPresencesQuery(
    string DocumentId
) : IRequest<Result<List<UserPresence>>>;

/// <summary>
/// Query zum Prüfen, ob ein Benutzer ein Dokument bearbeiten darf.
/// Berücksichtigt aktive Sperren.
/// </summary>
public record CanUserEditDocumentQuery(
    string DocumentId,
    string UserId
) : IRequest<Result<CanEditResult>>;

/// <summary>
/// Sammlung von Kommentaren mit Metadaten.
/// </summary>
public class CommentCollection
{
    public List<Comment> Comments { get; set; } = new();
    public int TotalCount { get; set; }
    public int PageNumber { get; set; }
    public int PageSize { get; set; }
    public bool HasNextPage => PageNumber * PageSize < TotalCount;
}

/// <summary>
/// Ergebnis der Bearbeitungs-Berechtigung Prüfung.
/// </summary>
public class CanEditResult
{
    public bool CanEdit { get; set; }
    public string? Reason { get; set; }
    public DocumentLock? ExistingLock { get; set; }
}
