/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CollaborationServices.cs                           ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     397                                            ║
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

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Domain.Collaboration;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service Interface für Dokumenten-Locking (Check-in/Check-out).
/// </summary>
public interface IDocumentLockingService
{
    Task<DocumentLock?> GetDocumentLockAsync(string documentId);
    Task<List<DocumentLock>> GetActiveLocksAsync(string? userId = null);
    Task<bool> AcquireLockAsync(DocumentLock documentLock, CancellationToken cancellationToken = default);
    Task<bool> ReleaseLockAsync(string documentId, CancellationToken cancellationToken = default);
    Task<bool> CanUserEditDocumentAsync(string documentId, string userId);
    Task CleanupExpiredLocksAsync();
}

/// <summary>
/// Service Interface für Kommentar-Verwaltung.
/// </summary>
public interface ICommentService
{
    Task<Comment?> GetCommentAsync(string commentId);
    Task<List<Comment>> GetDocumentCommentsAsync(string documentId, string? parentCommentId = null, bool includeDeleted = false);
    Task<bool> AddCommentAsync(Comment comment, CancellationToken cancellationToken = default);
    Task<bool> UpdateCommentAsync(Comment comment, CancellationToken cancellationToken = default);
    Task<bool> DeleteCommentAsync(string commentId, CancellationToken cancellationToken = default);
    Task<int> GetCommentCountAsync(string documentId);
}

/// <summary>
/// Implementierung des DocumentLockingService.
/// Verwaltet Dokumenten-Sperren für Collaboration.
/// </summary>
public class DocumentLockingService : IDocumentLockingService
{
    private readonly IThemisApiClient _apiClient;
    private readonly ILogger<DocumentLockingService> _logger;
    private readonly ConcurrentDictionary<string, DocumentLock> _locks = new();

    public DocumentLockingService(
        IThemisApiClient apiClient,
        ILogger<DocumentLockingService> logger)
    {
        _apiClient = apiClient;
        _logger = logger;
    }

    public Task<DocumentLock?> GetDocumentLockAsync(string documentId)
    {
        _locks.TryGetValue(documentId, out var documentLock);
        
        // Prüfen ob abgelaufen
        if (documentLock != null && documentLock.IsExpired())
        {
            _locks.TryRemove(documentId, out _);
            return Task.FromResult<DocumentLock?>(null);
        }

        return Task.FromResult<DocumentLock?>(documentLock);
    }

    public Task<List<DocumentLock>> GetActiveLocksAsync(string? userId = null)
    {
        var activeLocks = _locks.Values
            .Where(l => l.IsActive())
            .Where(l => userId == null || l.UserId == userId)
            .ToList();

        return Task.FromResult(activeLocks);
    }

    public async Task<bool> AcquireLockAsync(DocumentLock documentLock, CancellationToken cancellationToken = default)
    {
        try
        {
            // In-Memory Cache für schnellen Zugriff
            _locks[documentLock.DocumentId] = documentLock;

            // Persistieren in ThemisDB
            var aql = @"
                INSERT {
                    _key: @lockId,
                    documentId: @documentId,
                    userId: @userId,
                    userName: @userName,
                    lockedAt: @lockedAt,
                    expiresAt: @expiresAt,
                    type: @type,
                    reason: @reason,
                    machineName: @machineName
                } INTO document_locks
                RETURN NEW
            ";

            // Explicit enum to string mapping für Database-Kompatibilität
            string lockTypeStr = documentLock.Type switch
            {
                LockType.Read => "read",
                LockType.Write => "write",
                LockType.Optimistic => "optimistic",
                _ => "write" // Default fallback
            };

            var bindVars = new
            {
                lockId = documentLock.Id,
                documentId = documentLock.DocumentId,
                userId = documentLock.UserId,
                userName = documentLock.UserName,
                lockedAt = documentLock.LockedAt,
                expiresAt = documentLock.ExpiresAt,
                type = lockTypeStr,
                reason = documentLock.Reason,
                machineName = documentLock.MachineName
            };

            await _apiClient.ExecuteAqlAsync<object>(aql, bindVars, cancellationToken);

            _logger.LogInformation("Lock acquired for document {DocumentId} by user {UserId}", 
                documentLock.DocumentId, documentLock.UserId);

            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error acquiring lock for document {DocumentId}", documentLock.DocumentId);
            return false;
        }
    }

    public async Task<bool> ReleaseLockAsync(string documentId, CancellationToken cancellationToken = default)
    {
        try
        {
            // Aus Cache entfernen
            _locks.TryRemove(documentId, out _);

            // Aus ThemisDB löschen
            var aql = "FOR lock IN document_locks FILTER lock.documentId == @documentId REMOVE lock IN document_locks";
            var bindVars = new { documentId };

            await _apiClient.ExecuteAqlAsync<object>(aql, bindVars, cancellationToken);

            _logger.LogInformation("Lock released for document {DocumentId}", documentId);

            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error releasing lock for document {DocumentId}", documentId);
            return false;
        }
    }

    public async Task<bool> CanUserEditDocumentAsync(string documentId, string userId)
    {
        var existingLock = await GetDocumentLockAsync(documentId);
        
        // Kein Lock = Bearbeitung erlaubt
        if (existingLock == null)
            return true;

        // Eigenes Lock = Bearbeitung erlaubt
        if (existingLock.UserId == userId)
            return true;

        // Read-Lock von anderem Benutzer = Lesen erlaubt, Schreiben nicht
        if (existingLock.Type == LockType.Read)
            return false;

        // Write-Lock von anderem Benutzer = nicht erlaubt
        return false;
    }

    public async Task CleanupExpiredLocksAsync()
    {
        try
        {
            var expiredLocks = _locks.Values
                .Where(l => l.IsExpired())
                .Select(l => l.DocumentId)
                .ToList();

            foreach (var documentId in expiredLocks)
            {
                await ReleaseLockAsync(documentId);
            }

            _logger.LogInformation("Cleaned up {Count} expired locks", expiredLocks.Count);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error cleaning up expired locks");
        }
    }
}

/// <summary>
/// Implementierung des CommentService.
/// Verwaltet Kommentare zu Dokumenten.
/// </summary>
public class CommentService : ICommentService
{
    private readonly IThemisApiClient _apiClient;
    private readonly ILogger<CommentService> _logger;
    private readonly ConcurrentDictionary<string, Comment> _commentCache = new();

    public CommentService(
        IThemisApiClient apiClient,
        ILogger<CommentService> logger)
    {
        _apiClient = apiClient;
        _logger = logger;
    }

    public Task<Comment?> GetCommentAsync(string commentId)
    {
        _commentCache.TryGetValue(commentId, out var comment);
        return Task.FromResult<Comment?>(comment);
    }

    public async Task<List<Comment>> GetDocumentCommentsAsync(
        string documentId, 
        string? parentCommentId = null, 
        bool includeDeleted = false)
    {
        try
        {
            var aql = @"
                FOR comment IN document_comments
                FILTER comment.documentId == @documentId
                FILTER comment.parentCommentId == @parentCommentId
                FILTER @includeDeleted OR comment.isDeleted == false
                SORT comment.createdAt DESC
                RETURN comment
            ";

            var bindVars = new { documentId, parentCommentId, includeDeleted };
            var comments = await _apiClient.ExecuteAqlAsync<Comment>(aql, bindVars);

            return comments ?? new List<Comment>();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting comments for document {DocumentId}", documentId);
            return new List<Comment>();
        }
    }

    public async Task<bool> AddCommentAsync(Comment comment, CancellationToken cancellationToken = default)
    {
        try
        {
            // In Cache speichern
            _commentCache[comment.Id] = comment;

            // In ThemisDB persistieren
            var aql = @"
                INSERT @comment INTO document_comments
                RETURN NEW
            ";

            await _apiClient.ExecuteAqlAsync<object>(aql, new { comment }, cancellationToken);

            _logger.LogInformation("Comment {CommentId} added to document {DocumentId}", 
                comment.Id, comment.DocumentId);

            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error adding comment {CommentId}", comment.Id);
            return false;
        }
    }

    public async Task<bool> UpdateCommentAsync(Comment comment, CancellationToken cancellationToken = default)
    {
        try
        {
            comment.UpdatedAt = DateTime.UtcNow;
            _commentCache[comment.Id] = comment;

            var aql = @"
                UPDATE @commentId WITH {
                    content: @content,
                    updatedAt: @updatedAt
                } IN document_comments
            ";

            var bindVars = new 
            { 
                commentId = comment.Id, 
                content = comment.Content, 
                updatedAt = comment.UpdatedAt 
            };

            await _apiClient.ExecuteAqlAsync<object>(aql, bindVars, cancellationToken);

            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating comment {CommentId}", comment.Id);
            return false;
        }
    }

    public async Task<bool> DeleteCommentAsync(string commentId, CancellationToken cancellationToken = default)
    {
        try
        {
            if (_commentCache.TryGetValue(commentId, out var comment))
            {
                comment.IsDeleted = true;
                comment.DeletedAt = DateTime.UtcNow;
            }

            var aql = @"
                UPDATE @commentId WITH {
                    isDeleted: true,
                    deletedAt: @deletedAt
                } IN document_comments
            ";

            var bindVars = new { commentId, deletedAt = DateTime.UtcNow };
            await _apiClient.ExecuteAqlAsync<object>(aql, bindVars, cancellationToken);

            return true;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error deleting comment {CommentId}", commentId);
            return false;
        }
    }

    public async Task<int> GetCommentCountAsync(string documentId)
    {
        try
        {
            var aql = @"
                RETURN COUNT(
                    FOR comment IN document_comments
                    FILTER comment.documentId == @documentId
                    FILTER comment.isDeleted == false
                    RETURN 1
                )
            ";

            var bindVars = new { documentId };
            var result = await _apiClient.ExecuteAqlAsync<int>(aql, bindVars);

            return result?.FirstOrDefault() ?? 0;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting comment count for document {DocumentId}", documentId);
            return 0;
        }
    }
}
