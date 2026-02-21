/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CollaborationCommands.cs                           ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     104                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea516e4de  2025-12-10  Address code review feedback: Move Result to Common, impr... ║
    • 7e8faa1d8  2025-12-10  Implement Phase 2 Sprint 5-6: Collaboration features (Che... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Domain.Collaboration;
using Themis.DocumentManager.Application.Common;

namespace Themis.DocumentManager.Application.Collaboration.Commands;

/// <summary>
/// Command zum Auschecken (Sperren) eines Dokuments.
/// Implementiert Check-out Mechanismus für Collaboration.
/// </summary>
public record CheckOutDocumentCommand(
    string DocumentId,
    string UserId,
    string UserName,
    LockType LockType = LockType.Write,
    int? TimeoutMinutes = null,
    string? Reason = null
) : IRequest<Result<DocumentLock>>;

/// <summary>
/// Command zum Einchecken (Entsperren) eines Dokuments.
/// Gibt die Sperre frei nach erfolgreicher Bearbeitung.
/// </summary>
public record CheckInDocumentCommand(
    string DocumentId,
    string UserId,
    string? Comment = null
) : IRequest<Result<bool>>;

/// <summary>
/// Command zum Freigeben einer Dokumentensperre (Force unlock).
/// Nur für Administratoren oder bei abgelaufenen Sperren.
/// </summary>
public record ReleaseDocumentLockCommand(
    string DocumentId,
    string AdminUserId,
    string Reason
) : IRequest<Result<bool>>;

/// <summary>
/// Command zum Hinzufügen eines Kommentars zu einem Dokument.
/// Unterstützt Thread-basierte Diskussionen und @Mentions.
/// </summary>
public record AddCommentCommand(
    string DocumentId,
    string AuthorId,
    string AuthorName,
    string Content,
    string? ParentCommentId = null,
    List<string>? MentionedUserIds = null,
    DocumentPosition? Position = null
) : IRequest<Result<Comment>>;

/// <summary>
/// Command zum Aktualisieren eines Kommentars.
/// </summary>
public record UpdateCommentCommand(
    string CommentId,
    string UserId,
    string Content
) : IRequest<Result<Comment>>;

/// <summary>
/// Command zum Löschen eines Kommentars (Soft delete).
/// </summary>
public record DeleteCommentCommand(
    string CommentId,
    string UserId
) : IRequest<Result<bool>>;

/// <summary>
/// Command zum Hinzufügen einer Reaktion zu einem Kommentar.
/// </summary>
public record AddCommentReactionCommand(
    string CommentId,
    string UserId,
    string ReactionType
) : IRequest<Result<bool>>;

