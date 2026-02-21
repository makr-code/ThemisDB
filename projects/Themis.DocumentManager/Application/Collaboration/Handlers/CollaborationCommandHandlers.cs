/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CollaborationCommandHandlers.cs                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     214                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Application.Collaboration.Commands;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Domain.Collaboration;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Application.Collaboration.Handlers;

/// <summary>
/// Handler für CheckOutDocumentCommand.
/// Implementiert die Logik zum Sperren eines Dokuments.
/// </summary>
public class CheckOutDocumentHandler : IRequestHandler<CheckOutDocumentCommand, Result<DocumentLock>>
{
    private readonly IDocumentLockingService _lockingService;
    private readonly ILogger<CheckOutDocumentHandler> _logger;

    public CheckOutDocumentHandler(
        IDocumentLockingService lockingService,
        ILogger<CheckOutDocumentHandler> logger)
    {
        _lockingService = lockingService;
        _logger = logger;
    }

    public async Task<Result<DocumentLock>> Handle(CheckOutDocumentCommand request, CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("User {UserId} attempting to check out document {DocumentId}", 
                request.UserId, request.DocumentId);

            // Prüfen, ob bereits eine Sperre existiert
            var existingLock = await _lockingService.GetDocumentLockAsync(request.DocumentId);
            if (existingLock != null && existingLock.IsActive())
            {
                if (existingLock.UserId != request.UserId)
                {
                    _logger.LogWarning("Document {DocumentId} is already locked by {UserId}", 
                        request.DocumentId, existingLock.UserId);
                    return Result<DocumentLock>.Fail($"Dokument ist bereits von {existingLock.UserName} gesperrt");
                }
                
                // Benutzer hat bereits eine Sperre - verlängern
                return Result<DocumentLock>.Ok(existingLock);
            }

            // Neue Sperre erstellen
            var documentLock = new DocumentLock
            {
                DocumentId = request.DocumentId,
                UserId = request.UserId,
                UserName = request.UserName,
                Type = request.LockType,
                Reason = request.Reason,
                MachineName = Environment.MachineName,
                LockedAt = DateTime.UtcNow,
                ExpiresAt = request.TimeoutMinutes.HasValue 
                    ? DateTime.UtcNow.AddMinutes(request.TimeoutMinutes.Value) 
                    : null
            };

            await _lockingService.AcquireLockAsync(documentLock, cancellationToken);

            _logger.LogInformation("Document {DocumentId} locked successfully by {UserId}", 
                request.DocumentId, request.UserId);

            return Result<DocumentLock>.Ok(documentLock);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error checking out document {DocumentId}", request.DocumentId);
            return Result<DocumentLock>.Fail($"Fehler beim Auschecken: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für CheckInDocumentCommand.
/// Implementiert die Logik zum Entsperren eines Dokuments.
/// </summary>
public class CheckInDocumentHandler : IRequestHandler<CheckInDocumentCommand, Result<bool>>
{
    private readonly IDocumentLockingService _lockingService;
    private readonly ILogger<CheckInDocumentHandler> _logger;

    public CheckInDocumentHandler(
        IDocumentLockingService lockingService,
        ILogger<CheckInDocumentHandler> logger)
    {
        _lockingService = lockingService;
        _logger = logger;
    }

    public async Task<Result<bool>> Handle(CheckInDocumentCommand request, CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("User {UserId} attempting to check in document {DocumentId}", 
                request.UserId, request.DocumentId);

            var existingLock = await _lockingService.GetDocumentLockAsync(request.DocumentId);
            if (existingLock == null)
            {
                _logger.LogWarning("No lock found for document {DocumentId}", request.DocumentId);
                return Result<bool>.Fail("Keine aktive Sperre gefunden");
            }

            if (existingLock.UserId != request.UserId)
            {
                _logger.LogWarning("User {UserId} cannot release lock owned by {LockUserId}", 
                    request.UserId, existingLock.UserId);
                return Result<bool>.Fail("Sie können nur Ihre eigenen Sperren freigeben");
            }

            await _lockingService.ReleaseLockAsync(request.DocumentId, cancellationToken);

            _logger.LogInformation("Document {DocumentId} checked in successfully by {UserId}", 
                request.DocumentId, request.UserId);

            return Result<bool>.Ok(true);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error checking in document {DocumentId}", request.DocumentId);
            return Result<bool>.Fail($"Fehler beim Einchecken: {ex.Message}");
        }
    }
}

/// <summary>
/// Handler für AddCommentCommand.
/// Implementiert die Logik zum Hinzufügen von Kommentaren.
/// </summary>
public class AddCommentHandler : IRequestHandler<AddCommentCommand, Result<Comment>>
{
    private readonly ICommentService _commentService;
    private readonly ILogger<AddCommentHandler> _logger;

    public AddCommentHandler(
        ICommentService commentService,
        ILogger<AddCommentHandler> logger)
    {
        _commentService = commentService;
        _logger = logger;
    }

    public async Task<Result<Comment>> Handle(AddCommentCommand request, CancellationToken cancellationToken)
    {
        try
        {
            _logger.LogInformation("User {AuthorId} adding comment to document {DocumentId}", 
                request.AuthorId, request.DocumentId);

            var comment = new Comment
            {
                DocumentId = request.DocumentId,
                AuthorId = request.AuthorId,
                AuthorName = request.AuthorName,
                Content = request.Content,
                ParentCommentId = request.ParentCommentId,
                MentionedUserIds = request.MentionedUserIds ?? new List<string>(),
                Position = request.Position,
                CreatedAt = DateTime.UtcNow
            };

            // Wenn Parent-Kommentar, Thread-ID setzen
            if (!string.IsNullOrEmpty(request.ParentCommentId))
            {
                var parentComment = await _commentService.GetCommentAsync(request.ParentCommentId);
                comment.ThreadId = parentComment?.ThreadId ?? request.ParentCommentId;
            }

            await _commentService.AddCommentAsync(comment, cancellationToken);

            _logger.LogInformation("Comment {CommentId} added successfully to document {DocumentId}", 
                comment.Id, request.DocumentId);

            return Result<Comment>.Ok(comment);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error adding comment to document {DocumentId}", request.DocumentId);
            return Result<Comment>.Fail($"Fehler beim Hinzufügen des Kommentars: {ex.Message}");
        }
    }
}
