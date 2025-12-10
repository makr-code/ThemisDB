using MediatR;
using Themis.DocumentManager.Domain.Collaboration;

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

/// <summary>
/// Ergebnis-Wrapper für Commands mit Erfolgs-/Fehler-Handling.
/// </summary>
/// <typeparam name="T">Typ des Ergebnisses</typeparam>
public class Result<T>
{
    public bool Success { get; init; }
    public T? Value { get; init; }
    public string? ErrorMessage { get; init; }
    public List<string> Errors { get; init; } = new();

    public static Result<T> Ok(T value) => new() { Success = true, Value = value };
    public static Result<T> Fail(string error) => new() { Success = false, ErrorMessage = error };
    public static Result<T> Fail(List<string> errors) => new() { Success = false, Errors = errors };
}
