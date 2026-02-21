/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CollaborationCommands.cs                           ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     105                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

