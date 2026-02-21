/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentCollaborationViewModel.cs                  ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     497                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows.Input;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MediatR;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Application.Collaboration.Commands;
using Themis.DocumentManager.Application.Collaboration.Queries;
using Themis.DocumentManager.Domain.Collaboration;
using Themis.DocumentManager.Infrastructure.SignalR;

namespace Themis.DocumentManager.ViewModels;

/// <summary>
/// ViewModel für Document Collaboration Features.
/// Verwaltet Lock Status, Comments und User Presence.
/// </summary>
public partial class DocumentCollaborationViewModel : ObservableObject
{
    private readonly IMediator _mediator;
    private readonly ISignalRService _signalRService;
    private readonly ILogger<DocumentCollaborationViewModel> _logger;

    [ObservableProperty]
    private string? _currentDocumentId;

    [ObservableProperty]
    private string? _currentUserId;

    [ObservableProperty]
    private string? _currentUserName;

    [ObservableProperty]
    private bool _isDocumentLocked;

    [ObservableProperty]
    private string _lockStatusText = "Dokument entsperrt";

    [ObservableProperty]
    private string? _lockDetailsText;

    [ObservableProperty]
    private bool _canCheckOut = true;

    [ObservableProperty]
    private bool _canCheckIn;

    [ObservableProperty]
    private string _commentText = string.Empty;

    [ObservableProperty]
    private int _commentCount;

    [ObservableProperty]
    private bool _isLoadingComments;

    [ObservableProperty]
    private bool _isLoadingLockStatus;

    public ObservableCollection<CommentItemViewModel> Comments { get; } = new();
    public ObservableCollection<UserPresenceItemViewModel> ActiveUsers { get; } = new();

    private DocumentLock? _currentLock;

    public DocumentCollaborationViewModel(
        IMediator mediator,
        ISignalRService signalRService,
        ILogger<DocumentCollaborationViewModel> logger)
    {
        _mediator = mediator;
        _signalRService = signalRService;
        _logger = logger;

        // SignalR Events abonnieren
        SubscribeToSignalREvents();
    }

    public async Task InitializeAsync(string documentId, string userId, string userName)
    {
        CurrentDocumentId = documentId;
        CurrentUserId = userId;
        CurrentUserName = userName;

        await LoadLockStatusAsync();
        await LoadCommentsAsync();

        // SignalR: Dokument beitreten
        if (_signalRService.IsConnected)
        {
            await _signalRService.JoinDocumentAsync(documentId);
            await _signalRService.UpdatePresenceAsync(documentId, PresenceStatus.Viewing);
        }
    }

    private void SubscribeToSignalREvents()
    {
        _signalRService.DocumentLocked += OnDocumentLocked;
        _signalRService.DocumentUnlocked += OnDocumentUnlocked;
        _signalRService.CommentAdded += OnCommentAdded;
        _signalRService.PresenceUpdated += OnPresenceUpdated;
    }

    private async void OnDocumentLocked(object? sender, DocumentLockEventArgs e)
    {
        if (e.DocumentId != CurrentDocumentId) return;
        
        _currentLock = e.Lock;
        UpdateLockUI();
    }

    private async void OnDocumentUnlocked(object? sender, DocumentLockEventArgs e)
    {
        if (e.DocumentId != CurrentDocumentId) return;
        
        _currentLock = null;
        UpdateLockUI();
    }

    private async void OnCommentAdded(object? sender, CommentEventArgs e)
    {
        if (e.Comment == null) return;
        if (e.Comment.DocumentId != CurrentDocumentId) return;
        
        await LoadCommentsAsync();
    }

    private void OnPresenceUpdated(object? sender, PresenceEventArgs e)
    {
        if (e.Presence == null) return;
        if (e.DocumentId != CurrentDocumentId) return;
        
        UpdateActiveUser(e.Presence);
    }

    [RelayCommand]
    private async Task CheckOutDocumentAsync()
    {
        if (string.IsNullOrEmpty(CurrentDocumentId) || string.IsNullOrEmpty(CurrentUserId)) return;

        try
        {
            IsLoadingLockStatus = true;

            var command = new CheckOutDocumentCommand(
                DocumentId: CurrentDocumentId,
                UserId: CurrentUserId,
                UserName: CurrentUserName ?? "Benutzer",
                LockType: LockType.Write,
                TimeoutMinutes: 30
            );

            var result = await _mediator.Send(command);

            if (result.Success && result.Value != null)
            {
                _currentLock = result.Value;
                UpdateLockUI();

                // SignalR: Andere benachrichtigen
                await _signalRService.NotifyDocumentLockedAsync(CurrentDocumentId, result.Value);
                await _signalRService.UpdatePresenceAsync(CurrentDocumentId, PresenceStatus.Editing);

                _logger.LogInformation("Document {DocumentId} checked out successfully", CurrentDocumentId);
            }
            else
            {
                _logger.LogWarning("Failed to check out document: {Error}", result.ErrorMessage);
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error checking out document");
        }
        finally
        {
            IsLoadingLockStatus = false;
        }
    }

    [RelayCommand]
    private async Task CheckInDocumentAsync()
    {
        if (string.IsNullOrEmpty(CurrentDocumentId) || string.IsNullOrEmpty(CurrentUserId)) return;

        try
        {
            IsLoadingLockStatus = true;

            var command = new CheckInDocumentCommand(
                DocumentId: CurrentDocumentId,
                UserId: CurrentUserId
            );

            var result = await _mediator.Send(command);

            if (result.Success)
            {
                _currentLock = null;
                UpdateLockUI();

                // SignalR: Andere benachrichtigen
                await _signalRService.NotifyDocumentUnlockedAsync(CurrentDocumentId);
                await _signalRService.UpdatePresenceAsync(CurrentDocumentId, PresenceStatus.Viewing);

                _logger.LogInformation("Document {DocumentId} checked in successfully", CurrentDocumentId);
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error checking in document");
        }
        finally
        {
            IsLoadingLockStatus = false;
        }
    }

    [RelayCommand]
    private async Task AddCommentAsync()
    {
        if (string.IsNullOrEmpty(CurrentDocumentId) || string.IsNullOrEmpty(CommentText)) return;

        try
        {
            var command = new AddCommentCommand(
                DocumentId: CurrentDocumentId,
                AuthorId: CurrentUserId ?? "user",
                AuthorName: CurrentUserName ?? "Benutzer",
                Content: CommentText
            );

            var result = await _mediator.Send(command);

            if (result.Success && result.Value != null)
            {
                CommentText = string.Empty;
                await LoadCommentsAsync();

                // SignalR: Andere benachrichtigen
                await _signalRService.NotifyCommentAddedAsync(CurrentDocumentId, result.Value);

                _logger.LogInformation("Comment added to document {DocumentId}", CurrentDocumentId);
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error adding comment");
        }
    }

    [RelayCommand]
    private async Task LoadCommentsAsync()
    {
        if (string.IsNullOrEmpty(CurrentDocumentId)) return;

        try
        {
            IsLoadingComments = true;

            var query = new GetDocumentCommentsQuery(
                DocumentId: CurrentDocumentId,
                PageNumber: 1,
                PageSize: 100
            );

            var result = await _mediator.Send(query);

            if (result.Success && result.Value != null)
            {
                Comments.Clear();
                foreach (var comment in result.Value.Comments)
                {
                    Comments.Add(new CommentItemViewModel(comment));
                }

                CommentCount = Comments.Count;
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error loading comments");
        }
        finally
        {
            IsLoadingComments = false;
        }
    }

    private async Task LoadLockStatusAsync()
    {
        if (string.IsNullOrEmpty(CurrentDocumentId)) return;

        try
        {
            IsLoadingLockStatus = true;

            var query = new GetDocumentLockStatusQuery(CurrentDocumentId);
            var result = await _mediator.Send(query);

            if (result.Success && result.Value != null)
            {
                _currentLock = result.Value;
            }

            UpdateLockUI();
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error loading lock status");
        }
        finally
        {
            IsLoadingLockStatus = false;
        }
    }

    private void UpdateLockUI()
    {
        if (_currentLock == null || !_currentLock.IsActive())
        {
            IsDocumentLocked = false;
            LockStatusText = "Dokument entsperrt";
            LockDetailsText = null;
            CanCheckOut = true;
            CanCheckIn = false;
        }
        else
        {
            IsDocumentLocked = true;
            bool isOwnLock = _currentLock.UserId == CurrentUserId;

            LockStatusText = isOwnLock
                ? "Von Ihnen ausgecheckt"
                : $"Gesperrt von {_currentLock.UserName}";

            LockDetailsText = $"Seit {_currentLock.LockedAt:HH:mm}";
            CanCheckOut = !isOwnLock;
            CanCheckIn = isOwnLock;
        }
    }

    private void UpdateActiveUser(UserPresence presence)
    {
        var existing = ActiveUsers.FirstOrDefault(u => u.UserId == presence.UserId);
        
        if (existing != null)
        {
            existing.UpdateFromPresence(presence);
        }
        else
        {
            ActiveUsers.Add(new UserPresenceItemViewModel(presence));
        }

        // Remove inactive users
        var inactive = ActiveUsers.Where(u => !u.IsActive).ToList();
        foreach (var user in inactive)
        {
            ActiveUsers.Remove(user);
        }
    }
}

/// <summary>
/// ViewModel für einzelnen Kommentar
/// </summary>
public partial class CommentItemViewModel : ObservableObject
{
    [ObservableProperty]
    private string _authorName;

    [ObservableProperty]
    private string _content;

    [ObservableProperty]
    private string _createdAtText;

    [ObservableProperty]
    private bool _isEdited;

    [ObservableProperty]
    private int _reactionCount;

    [ObservableProperty]
    private bool _hasReactions;

    public CommentItemViewModel(Comment comment)
    {
        _authorName = comment.AuthorName;
        _content = comment.Content;
        _createdAtText = GetRelativeTime(comment.CreatedAt);
        _isEdited = comment.IsEdited;
        _reactionCount = comment.Reactions.Count;
        _hasReactions = _reactionCount > 0;
    }

    private string GetRelativeTime(DateTime dateTime)
    {
        var diff = DateTime.UtcNow - dateTime;

        if (diff.TotalMinutes < 1) return "Gerade eben";
        if (diff.TotalMinutes < 60) return $"vor {(int)diff.TotalMinutes} Min.";
        if (diff.TotalHours < 24) return $"vor {(int)diff.TotalHours} Std.";
        if (diff.TotalDays < 7) return $"vor {(int)diff.TotalDays} Tag(en)";

        return dateTime.ToString("dd.MM.yyyy HH:mm");
    }
}

/// <summary>
/// ViewModel für User Presence
/// </summary>
public partial class UserPresenceItemViewModel : ObservableObject
{
    [ObservableProperty]
    private string _userId = string.Empty;

    [ObservableProperty]
    private string _userName = string.Empty;

    [ObservableProperty]
    private string _initials = string.Empty;

    [ObservableProperty]
    private string _color = string.Empty;

    [ObservableProperty]
    private string _statusText = string.Empty;

    [ObservableProperty]
    private bool _isActive;

    public UserPresenceItemViewModel(UserPresence presence)
    {
        UpdateFromPresence(presence);
    }

    public void UpdateFromPresence(UserPresence presence)
    {
        UserId = presence.UserId;
        UserName = presence.UserName;
        Initials = GetInitials(presence.UserName);
        Color = GetColorForUser(presence.UserId);
        StatusText = GetStatusText(presence.Status);
        IsActive = presence.IsActive(TimeSpan.FromMinutes(5));
    }

    private string GetInitials(string name)
    {
        var parts = name.Split(' ');
        if (parts.Length >= 2)
            return $"{parts[0][0]}{parts[1][0]}".ToUpper();
        if (parts.Length == 1 && parts[0].Length >= 2)
            return parts[0].Substring(0, 2).ToUpper();
        return "??";
    }

    private string GetColorForUser(string userId)
    {
        var hash = userId.GetHashCode();
        var colors = new[] { "#2196F3", "#4CAF50", "#FF9800", "#9C27B0", "#F44336", "#00BCD4" };
        return colors[Math.Abs(hash) % colors.Length];
    }

    private string GetStatusText(PresenceStatus status)
    {
        return status switch
        {
            PresenceStatus.Viewing => "Betrachtet",
            PresenceStatus.Editing => "Bearbeitet",
            PresenceStatus.Away => "Abwesend",
            PresenceStatus.Left => "Verlassen",
            _ => "Aktiv"
        };
    }
}
