/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CollaborationCommands.cs                           ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     105                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

