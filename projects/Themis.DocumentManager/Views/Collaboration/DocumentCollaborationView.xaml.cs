/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentCollaborationView.xaml.cs                  ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     458                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
    • e550fd37e  2025-12-10  Add UI components, ViewModel, and Unit Tests for Phase 2 ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using MediatR;
using Themis.DocumentManager.Application.Collaboration.Commands;
using Themis.DocumentManager.Application.Collaboration.Queries;
using Themis.DocumentManager.Domain.Collaboration;
using Themis.DocumentManager.Infrastructure.SignalR;

namespace Themis.DocumentManager.Views.Collaboration
{
    /// <summary>
    /// Collaboration View für Dokumenten-Locking, Kommentare und User Presence.
    /// Implementiert Phase 2 Sprint 5-6 UI Components.
    /// </summary>
    public partial class DocumentCollaborationView : UserControl
    {
        private readonly IMediator _mediator;
        private readonly ISignalRService _signalRService;
        
        private string? _currentDocumentId;
        private string? _currentUserId;
        private string? _currentUserName;
        private DocumentLock? _currentLock;
        
        // View Models für UI Binding
        private List<UserPresenceViewModel> _activeUsers = new();
        private List<CommentViewModel> _comments = new();

        public DocumentCollaborationView()
        {
            InitializeComponent();
            
            // Services werden über Property Injection gesetzt
            // oder können über Constructor Injection kommen wenn View im DI Container registriert ist
            _mediator = null!; // Wird von außen gesetzt
            _signalRService = null!; // Wird von außen gesetzt
        }

        /// <summary>
        /// Constructor mit Dependency Injection
        /// </summary>
        public DocumentCollaborationView(IMediator mediator, ISignalRService signalRService) : this()
        {
            _mediator = mediator;
            _signalRService = signalRService;
            
            // SignalR Events abonnieren
            SubscribeToSignalREvents();
        }

        /// <summary>
        /// Initialisiert die View für ein bestimmtes Dokument
        /// </summary>
        public async void InitializeForDocument(string documentId, string userId, string userName)
        {
            _currentDocumentId = documentId;
            _currentUserId = userId;
            _currentUserName = userName;

            // Lock Status laden
            await LoadLockStatusAsync();

            // Kommentare laden
            await LoadCommentsAsync();

            // SignalR: Dokument beitreten
            if (_signalRService != null && _signalRService.IsConnected)
            {
                await _signalRService.JoinDocumentAsync(documentId);
                await _signalRService.UpdatePresenceAsync(documentId, PresenceStatus.Viewing);
            }
        }

        private void SubscribeToSignalREvents()
        {
            if (_signalRService == null) return;

            _signalRService.DocumentLocked += OnDocumentLockedEvent;
            _signalRService.DocumentUnlocked += OnDocumentUnlockedEvent;
            _signalRService.CommentAdded += OnCommentAddedEvent;
            _signalRService.PresenceUpdated += OnPresenceUpdatedEvent;
        }

        private async void OnDocumentLockedEvent(object? sender, DocumentLockEventArgs e)
        {
            if (e.DocumentId != _currentDocumentId) return;

            // UI Thread Update
            await Dispatcher.InvokeAsync(() =>
            {
                _currentLock = e.Lock;
                UpdateLockUI();
            });
        }

        private async void OnDocumentUnlockedEvent(object? sender, DocumentLockEventArgs e)
        {
            if (e.DocumentId != _currentDocumentId) return;

            await Dispatcher.InvokeAsync(() =>
            {
                _currentLock = null;
                UpdateLockUI();
            });
        }

        private async void OnCommentAddedEvent(object? sender, CommentEventArgs e)
        {
            if (e.Comment == null || e.Comment.DocumentId != _currentDocumentId) return;

            await Dispatcher.InvokeAsync(async () =>
            {
                await LoadCommentsAsync(); // Reload comments
            });
        }

        private async void OnPresenceUpdatedEvent(object? sender, PresenceEventArgs e)
        {
            if (e.Presence == null || e.DocumentId != _currentDocumentId) return;

            await Dispatcher.InvokeAsync(() =>
            {
                UpdateActiveUsers(e.Presence);
            });
        }

        private async System.Threading.Tasks.Task LoadLockStatusAsync()
        {
            if (_mediator == null || string.IsNullOrEmpty(_currentDocumentId)) return;

            var query = new GetDocumentLockStatusQuery(_currentDocumentId);
            var result = await _mediator.Send(query);

            if (result.Success && result.Value != null)
            {
                _currentLock = result.Value;
            }
            else
            {
                _currentLock = null;
            }

            UpdateLockUI();
        }

        private void UpdateLockUI()
        {
            if (_currentLock == null || !_currentLock.IsActive())
            {
                // Dokument ist nicht gesperrt
                LockStatusText.Text = "Dokument entsperrt";
                LockDetailsText.Visibility = Visibility.Collapsed;
                LockIcon.Visibility = Visibility.Collapsed;
                LockActionButton.Content = "Auschecken";
            }
            else
            {
                // Dokument ist gesperrt
                bool isOwnLock = _currentLock.UserId == _currentUserId;
                
                LockStatusText.Text = isOwnLock 
                    ? "Von Ihnen ausgecheckt" 
                    : $"Gesperrt von {_currentLock.UserName}";
                
                LockDetailsText.Text = $"Seit {_currentLock.LockedAt:HH:mm}";
                LockDetailsText.Visibility = Visibility.Visible;
                LockIcon.Visibility = Visibility.Visible;
                
                LockActionButton.Content = isOwnLock ? "Einchecken" : "Entsperrt";
                LockActionButton.IsEnabled = isOwnLock;
            }
        }

        private async void LockActionButton_Click(object sender, RoutedEventArgs e)
        {
            if (_mediator == null || string.IsNullOrEmpty(_currentDocumentId)) return;

            try
            {
                if (_currentLock == null || !_currentLock.IsActive())
                {
                    // Check-out
                    var command = new CheckOutDocumentCommand(
                        DocumentId: _currentDocumentId,
                        UserId: _currentUserId ?? "user",
                        UserName: _currentUserName ?? "Benutzer",
                        LockType: LockType.Write,
                        TimeoutMinutes: 30
                    );

                    var result = await _mediator.Send(command);
                    
                    if (result.Success)
                    {
                        _currentLock = result.Value;
                        UpdateLockUI();

                        // SignalR: Andere benachrichtigen
                        if (_signalRService != null && result.Value != null)
                        {
                            await _signalRService.NotifyDocumentLockedAsync(_currentDocumentId, result.Value);
                            await _signalRService.UpdatePresenceAsync(_currentDocumentId, PresenceStatus.Editing);
                        }
                    }
                    else
                    {
                        MessageBox.Show(result.ErrorMessage ?? "Fehler beim Auschecken", "Fehler", 
                            MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
                else if (_currentLock.UserId == _currentUserId)
                {
                    // Check-in
                    var command = new CheckInDocumentCommand(
                        DocumentId: _currentDocumentId,
                        UserId: _currentUserId ?? "user"
                    );

                    var result = await _mediator.Send(command);
                    
                    if (result.Success)
                    {
                        _currentLock = null;
                        UpdateLockUI();

                        // SignalR: Andere benachrichtigen
                        if (_signalRService != null)
                        {
                            await _signalRService.NotifyDocumentUnlockedAsync(_currentDocumentId);
                            await _signalRService.UpdatePresenceAsync(_currentDocumentId, PresenceStatus.Viewing);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Fehler: {ex.Message}", "Fehler", 
                    MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private async System.Threading.Tasks.Task LoadCommentsAsync()
        {
            if (_mediator == null || string.IsNullOrEmpty(_currentDocumentId)) return;

            var query = new GetDocumentCommentsQuery(
                DocumentId: _currentDocumentId,
                PageNumber: 1,
                PageSize: 100
            );

            var result = await _mediator.Send(query);
            
            if (result.Success && result.Value != null)
            {
                _comments = result.Value.Comments
                    .Select(c => new CommentViewModel(c))
                    .ToList();

                CommentsControl.ItemsSource = _comments;
                CommentCountText.Text = $"({_comments.Count})";
            }
        }

        private async void AddComment_Click(object sender, RoutedEventArgs e)
        {
            if (_mediator == null || string.IsNullOrEmpty(_currentDocumentId)) return;
            
            var content = CommentInputBox.Text?.Trim();
            if (string.IsNullOrEmpty(content)) return;

            try
            {
                var command = new AddCommentCommand(
                    DocumentId: _currentDocumentId,
                    AuthorId: _currentUserId ?? "user",
                    AuthorName: _currentUserName ?? "Benutzer",
                    Content: content
                );

                var result = await _mediator.Send(command);
                
                if (result.Success && result.Value != null)
                {
                    // Clear input
                    CommentInputBox.Text = string.Empty;

                    // Reload comments
                    await LoadCommentsAsync();

                    // SignalR: Andere benachrichtigen
                    if (_signalRService != null)
                    {
                        await _signalRService.NotifyCommentAddedAsync(_currentDocumentId, result.Value);
                    }
                }
                else
                {
                    MessageBox.Show(result.ErrorMessage ?? "Fehler beim Hinzufügen des Kommentars", 
                        "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Fehler: {ex.Message}", "Fehler", 
                    MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void CancelComment_Click(object sender, RoutedEventArgs e)
        {
            CommentInputBox.Text = string.Empty;
        }

        private async void RefreshComments_Click(object sender, RoutedEventArgs e)
        {
            await LoadCommentsAsync();
        }

        private void UpdateActiveUsers(UserPresence presence)
        {
            // Update oder Add presence
            var existing = _activeUsers.FirstOrDefault(u => u.UserId == presence.UserId);
            if (existing != null)
            {
                existing.Update(presence);
            }
            else
            {
                _activeUsers.Add(new UserPresenceViewModel(presence));
            }

            // Remove inactive users
            _activeUsers.RemoveAll(u => !u.IsActive);

            // Update UI
            ActiveUsersControl.ItemsSource = null;
            ActiveUsersControl.ItemsSource = _activeUsers;
            NoActiveUsersText.Visibility = _activeUsers.Any() ? Visibility.Collapsed : Visibility.Visible;
        }
    }

    /// <summary>
    /// ViewModel für Comment UI Binding
    /// </summary>
    public class CommentViewModel
    {
        public string AuthorName { get; set; }
        public string Content { get; set; }
        public string CreatedAtText { get; set; }
        public bool IsEdited { get; set; }
        public int ReactionCount { get; set; }
        public bool HasReactions { get; set; }

        public CommentViewModel(Comment comment)
        {
            AuthorName = comment.AuthorName;
            Content = comment.Content;
            CreatedAtText = GetRelativeTime(comment.CreatedAt);
            IsEdited = comment.IsEdited;
            ReactionCount = comment.Reactions.Count;
            HasReactions = ReactionCount > 0;
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
    /// ViewModel für UserPresence UI Binding
    /// </summary>
    public class UserPresenceViewModel
    {
        public string UserId { get; set; }
        public string UserName { get; set; }
        public string Initials { get; set; }
        public string Color { get; set; }
        public string StatusText { get; set; }
        public bool IsActive { get; set; }

        public UserPresenceViewModel(UserPresence presence)
        {
            UserId = presence.UserId;
            UserName = presence.UserName;
            Initials = GetInitials(presence.UserName);
            Color = GetColorForUser(presence.UserId);
            StatusText = GetStatusText(presence.Status);
            IsActive = presence.IsActive(TimeSpan.FromMinutes(5));
        }

        public void Update(UserPresence presence)
        {
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
            // Einfacher Hash für konsistente Farbe pro Benutzer
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
}
