/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SignalRService.cs                                  ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     364                                            ║
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

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.AspNetCore.SignalR.Client;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Domain.Collaboration;

namespace Themis.DocumentManager.Infrastructure.SignalR;

/// <summary>
/// Service Interface für SignalR Real-time Communication.
/// </summary>
public interface ISignalRService
{
    Task ConnectAsync(string hubUrl, string userId, string userName);
    Task DisconnectAsync();
    Task JoinDocumentAsync(string documentId);
    Task LeaveDocumentAsync(string documentId);
    Task NotifyDocumentLockedAsync(string documentId, DocumentLock documentLock);
    Task NotifyDocumentUnlockedAsync(string documentId);
    Task NotifyCommentAddedAsync(string documentId, Comment comment);
    Task UpdatePresenceAsync(string documentId, PresenceStatus status);
    
    event EventHandler<DocumentLockEventArgs>? DocumentLocked;
    event EventHandler<DocumentLockEventArgs>? DocumentUnlocked;
    event EventHandler<CommentEventArgs>? CommentAdded;
    event EventHandler<PresenceEventArgs>? PresenceUpdated;
    
    bool IsConnected { get; }
}

/// <summary>
/// Implementierung des SignalR Service für Real-time Collaboration.
/// Verwaltet die Verbindung zu einem SignalR Hub.
/// </summary>
public class SignalRService : ISignalRService, IAsyncDisposable
{
    private readonly ILogger<SignalRService> _logger;
    private readonly SignalRConfiguration _config;
    private HubConnection? _connection;
    private string _currentUserId = string.Empty;
    private string _currentUserName = string.Empty;
    private readonly ConcurrentDictionary<string, UserPresence> _activePresences = new();

    public event EventHandler<DocumentLockEventArgs>? DocumentLocked;
    public event EventHandler<DocumentLockEventArgs>? DocumentUnlocked;
    public event EventHandler<CommentEventArgs>? CommentAdded;
    public event EventHandler<PresenceEventArgs>? PresenceUpdated;

    public bool IsConnected => _connection?.State == HubConnectionState.Connected;

    public SignalRService(ILogger<SignalRService> logger, SignalRConfiguration? config = null)
    {
        _logger = logger;
        _config = config ?? new SignalRConfiguration();
    }

    public async Task ConnectAsync(string hubUrl, string userId, string userName)
    {
        if (_connection != null && IsConnected)
        {
            _logger.LogWarning("Already connected to SignalR hub");
            return;
        }

        _currentUserId = userId;
        _currentUserName = userName;

        var builder = new HubConnectionBuilder()
            .WithUrl(hubUrl);

        // Configurable reconnection
        if (_config.AutoReconnect)
        {
            builder.WithAutomaticReconnect(_config.ReconnectionIntervals);
        }

        _connection = builder.Build();

        // Event Handlers registrieren
        RegisterEventHandlers();

        try
        {
            await _connection.StartAsync();
            _logger.LogInformation("Connected to SignalR hub at {HubUrl}", hubUrl);

            // Benutzer beim Hub registrieren
            await _connection.InvokeAsync("RegisterUser", userId, userName);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error connecting to SignalR hub");
            throw;
        }
    }

    public async Task DisconnectAsync()
    {
        if (_connection != null)
        {
            try
            {
                await _connection.StopAsync();
                await _connection.DisposeAsync();
                _connection = null;
                _logger.LogInformation("Disconnected from SignalR hub");
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Error disconnecting from SignalR hub");
            }
        }
    }

    public async Task JoinDocumentAsync(string documentId)
    {
        if (!IsConnected)
        {
            _logger.LogWarning("Not connected to SignalR hub");
            return;
        }

        if (_connection is null)
        {
            _logger.LogWarning("No SignalR connection available when joining document {DocumentId}", documentId);
            return;
        }

        try
        {
            await _connection.InvokeAsync("JoinDocument", documentId);
            _logger.LogInformation("Joined document {DocumentId}", documentId);

            // Präsenz registrieren
            var presence = new UserPresence
            {
                UserId = _currentUserId,
                UserName = _currentUserName,
                DocumentId = documentId,
                Status = PresenceStatus.Viewing,
                ConnectionId = _connection.ConnectionId ?? string.Empty
            };
            _activePresences[documentId] = presence;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error joining document {DocumentId}", documentId);
        }
    }

    public async Task LeaveDocumentAsync(string documentId)
    {
        if (!IsConnected)
        {
            _logger.LogWarning("Not connected to SignalR hub");
            return;
        }

        try
        {
            await _connection!.InvokeAsync("LeaveDocument", documentId);
            _activePresences.TryRemove(documentId, out _);
            _logger.LogInformation("Left document {DocumentId}", documentId);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error leaving document {DocumentId}", documentId);
        }
    }

    public async Task NotifyDocumentLockedAsync(string documentId, DocumentLock documentLock)
    {
        if (!IsConnected) return;

        try
        {
            await _connection!.InvokeAsync("NotifyDocumentLocked", documentId, documentLock);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error notifying document locked");
        }
    }

    public async Task NotifyDocumentUnlockedAsync(string documentId)
    {
        if (!IsConnected) return;

        try
        {
            await _connection!.InvokeAsync("NotifyDocumentUnlocked", documentId);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error notifying document unlocked");
        }
    }

    public async Task NotifyCommentAddedAsync(string documentId, Comment comment)
    {
        if (!IsConnected) return;

        try
        {
            await _connection!.InvokeAsync("NotifyCommentAdded", documentId, comment);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error notifying comment added");
        }
    }

    public async Task UpdatePresenceAsync(string documentId, PresenceStatus status)
    {
        if (!IsConnected) return;

        try
        {
            if (_activePresences.TryGetValue(documentId, out var presence))
            {
                presence.Status = status;
                presence.UpdateActivity();
            }

            await _connection!.InvokeAsync("UpdatePresence", documentId, status);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating presence");
        }
    }

    private void RegisterEventHandlers()
    {
        if (_connection == null) return;

        // Empfang: Dokument wurde gesperrt
        _connection.On<string, DocumentLock>("OnDocumentLocked", (documentId, documentLock) =>
        {
            _logger.LogInformation("Document {DocumentId} locked by {UserId}", documentId, documentLock.UserId);
            DocumentLocked?.Invoke(this, new DocumentLockEventArgs(documentId, documentLock));
        });

        // Empfang: Dokument wurde entsperrt
        _connection.On<string>("OnDocumentUnlocked", (documentId) =>
        {
            _logger.LogInformation("Document {DocumentId} unlocked", documentId);
            DocumentUnlocked?.Invoke(this, new DocumentLockEventArgs(documentId, null));
        });

        // Empfang: Kommentar hinzugefügt
        _connection.On<string, Comment>("OnCommentAdded", (documentId, comment) =>
        {
            _logger.LogInformation("Comment added to document {DocumentId}", documentId);
            CommentAdded?.Invoke(this, new CommentEventArgs(documentId, comment));
        });

        // Empfang: Präsenz aktualisiert
        _connection.On<string, UserPresence>("OnPresenceUpdated", (documentId, presence) =>
        {
            _logger.LogInformation("Presence updated for user {UserId} in document {DocumentId}", 
                presence?.UserId, documentId);
            PresenceUpdated?.Invoke(this, new PresenceEventArgs(documentId, presence));
        });

        // Reconnection Events
        _connection.Reconnecting += error =>
        {
            _logger.LogWarning("SignalR reconnecting: {Error}", error?.Message);
            return Task.CompletedTask;
        };

        _connection.Reconnected += connectionId =>
        {
            _logger.LogInformation("SignalR reconnected with connection ID {ConnectionId}", connectionId);
            return Task.CompletedTask;
        };

        _connection.Closed += error =>
        {
            _logger.LogWarning("SignalR connection closed: {Error}", error?.Message);
            return Task.CompletedTask;
        };
    }

    public async ValueTask DisposeAsync()
    {
        await DisconnectAsync();
    }
}

/// <summary>
/// Event Args für Document Lock Events.
/// </summary>
public class DocumentLockEventArgs : EventArgs
{
    public string DocumentId { get; }
    public DocumentLock? Lock { get; }

    public DocumentLockEventArgs(string documentId, DocumentLock? documentLock)
    {
        DocumentId = documentId;
        Lock = documentLock;
    }
}

/// <summary>
/// Event Args für Comment Events.
/// </summary>
public class CommentEventArgs : EventArgs
{
    public string DocumentId { get; }
    public Comment? Comment { get; }

    public CommentEventArgs(string documentId, Comment comment)
    {
        DocumentId = documentId;
        Comment = comment;
    }
}

/// <summary>
/// Event Args für Presence Events.
/// </summary>
public class PresenceEventArgs : EventArgs
{
    public string DocumentId { get; }
    public UserPresence? Presence { get; }

    public PresenceEventArgs(string documentId, UserPresence? presence)
    {
        DocumentId = documentId;
        Presence = presence;
    }
}
