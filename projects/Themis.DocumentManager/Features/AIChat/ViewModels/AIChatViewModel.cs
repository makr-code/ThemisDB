/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AIChatViewModel.cs                                 ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     457                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Features.AIChat.Services;

namespace Themis.DocumentManager.Features.AIChat.ViewModels;

/// <summary>
/// ViewModel für AI Assistant Chat (VSCode-Style)
/// </summary>
public partial class AIChatViewModel : ObservableObject
{
    private readonly IAIChatService _chatService;
    private readonly IMCPToolService _mcpService;
    private readonly string _userId;
    
    [ObservableProperty]
    private AIChatSession? _currentSession;
    
    [ObservableProperty]
    private string _inputText = string.Empty;
    
    [ObservableProperty]
    private bool _isStreaming = false;
    
    [ObservableProperty]
    private bool _isBusy = false;
    
    [ObservableProperty]
    private string _streamingText = string.Empty;
    
    public ObservableCollection<AIChatMessage> Messages { get; } = new();
    public ObservableCollection<AIChatSession> Sessions { get; } = new();
    public ObservableCollection<ChatSuggestion> Suggestions { get; } = new();
    public ObservableCollection<MCPTool> AvailableTools { get; } = new();
    public ObservableCollection<MCPToolCall> PendingApprovals { get; } = new();
    
    // Lazy-initialized Commands (MVVM Toolkit Style)
    private AsyncRelayCommand? _sendMessageCommand;
    public ICommand SendMessageCommand => _sendMessageCommand ??= new AsyncRelayCommand(SendMessageAsync, () => !IsBusy && !string.IsNullOrWhiteSpace(InputText));
    
    private AsyncRelayCommand? _newSessionCommand;
    public ICommand NewSessionCommand => _newSessionCommand ??= new AsyncRelayCommand(CreateNewSessionAsync);
    
    private AsyncRelayCommand? _clearContextCommand;
    public ICommand ClearContextCommand => _clearContextCommand ??= new AsyncRelayCommand(ClearContextAsync);
    
    public AIChatViewModel(IAIChatService chatService, IMCPToolService mcpService, string userId)
    {
        ArgumentNullException.ThrowIfNull(chatService);
        ArgumentNullException.ThrowIfNull(mcpService);
        ArgumentException.ThrowIfNullOrEmpty(userId);
        
        _chatService = chatService;
        _mcpService = mcpService;
        _userId = userId;
        
        _ = InitializeAsync();
    }

    partial void OnInputTextChanged(string value)
    {
        _sendMessageCommand?.NotifyCanExecuteChanged();
    }

    partial void OnIsBusyChanged(bool value)
    {
        _sendMessageCommand?.NotifyCanExecuteChanged();
    }
    
    private async Task InitializeAsync()
    {
        try
        {
            IsBusy = true;
            
            // Load user sessions
            await LoadSessionsAsync();
            
            // Load suggestions
            await LoadSuggestionsAsync();
            
            // Load available MCP tools
            await LoadAvailableToolsAsync();
            
            // Load pending approvals
            await LoadPendingApprovalsAsync();
            
            // Create or load default session
            if (!Sessions.Any())
            {
                await CreateNewSessionAsync();
            }
            else
            {
                await SwitchToSessionAsync(Sessions.First().Id);
            }
        }
        finally
        {
            IsBusy = false;
        }
    }
    
    private async Task LoadSessionsAsync()
    {
        var sessions = await _chatService.GetUserSessionsAsync(_userId);
        
        Sessions.Clear();
        foreach (var session in sessions)
        {
            Sessions.Add(session);
        }
    }
    
    private async Task LoadSuggestionsAsync()
    {
        var suggestions = await _chatService.GetSuggestionsAsync();
        
        Suggestions.Clear();
        foreach (var suggestion in suggestions)
        {
            Suggestions.Add(suggestion);
        }
    }
    
    private async Task LoadAvailableToolsAsync()
    {
        var tools = await _mcpService.GetAvailableToolsAsync();
        
        AvailableTools.Clear();
        foreach (var tool in tools)
        {
            AvailableTools.Add(tool);
        }
    }
    
    private async Task LoadPendingApprovalsAsync()
    {
        var approvals = await _mcpService.GetPendingApprovalsAsync(_userId);
        
        PendingApprovals.Clear();
        foreach (var approval in approvals)
        {
            PendingApprovals.Add(approval);
        }
    }
    
    private async Task CreateNewSessionAsync()
    {
        var session = await _chatService.CreateSessionAsync(_userId);
        Sessions.Insert(0, session);
        await SwitchToSessionAsync(session.Id);
    }
    
    private async Task DeleteSessionAsync(string? sessionId)
    {
        if (string.IsNullOrEmpty(sessionId)) return;
        
        await _chatService.DeleteSessionAsync(sessionId);
        
        var session = Sessions.FirstOrDefault(s => s.Id == sessionId);
        if (session != null)
        {
            Sessions.Remove(session);
        }
        
        if (CurrentSession?.Id == sessionId)
        {
            CurrentSession = Sessions.FirstOrDefault();
            if (CurrentSession != null)
            {
                await LoadMessagesAsync(CurrentSession.Id);
            }
        }
    }
    
    private async Task SwitchToSessionAsync(string? sessionId)
    {
        if (string.IsNullOrEmpty(sessionId)) return;
        
        var session = await _chatService.GetSessionAsync(sessionId);
        if (session == null) return;
        
        CurrentSession = session;
        await LoadMessagesAsync(sessionId);
    }
    
    private async Task LoadMessagesAsync(string sessionId)
    {
        var messages = await _chatService.GetMessagesAsync(sessionId);
        
        Messages.Clear();
        foreach (var message in messages)
        {
            Messages.Add(message);
        }
    }
    
    private async Task SendMessageAsync()
    {
        if (CurrentSession == null || string.IsNullOrWhiteSpace(InputText)) return;
        
        try
        {
            IsBusy = true;
            
            var userInput = InputText;
            InputText = string.Empty; // Clear input immediately
            
            // Add user message to UI
            var userMessage = new AIChatMessage
            {
                SessionId = CurrentSession.Id,
                Role = ChatMessageRole.User,
                Content = userInput,
                Timestamp = DateTime.UtcNow
            };
            Messages.Add(userMessage);
            
            // Send to AI and get response
            var response = await _chatService.SendMessageAsync(CurrentSession.Id, userInput);
            
            // Add response to UI
            Messages.Add(response);
            
            // Update session title if first message
            if (Messages.Count == 2) // User + Assistant
            {
                var title = GenerateSessionTitle(userInput);
                await _chatService.UpdateSessionTitleAsync(CurrentSession.Id, title);
                CurrentSession.Title = title;
            }
        }
        catch (Exception ex)
        {
            // Error handling
            var errorMessage = new AIChatMessage
            {
                SessionId = CurrentSession.Id,
                Role = ChatMessageRole.System,
                Content = $"❌ Fehler: {ex.Message}",
                Timestamp = DateTime.UtcNow
            };
            Messages.Add(errorMessage);
        }
        finally
        {
            IsBusy = false;
        }
    }
    
    private async Task SendMessageStreamingAsync()
    {
        if (CurrentSession == null || string.IsNullOrWhiteSpace(InputText)) return;
        
        try
        {
            IsStreaming = true;
            IsBusy = true;
            
            var userInput = InputText;
            InputText = string.Empty;
            
            // Add user message
            var userMessage = new AIChatMessage
            {
                SessionId = CurrentSession.Id,
                Role = ChatMessageRole.User,
                Content = userInput,
                Timestamp = DateTime.UtcNow
            };
            Messages.Add(userMessage);
            
            // Create streaming message placeholder
            var streamingMessage = new AIChatMessage
            {
                SessionId = CurrentSession.Id,
                Role = ChatMessageRole.Assistant,
                Content = "",
                Timestamp = DateTime.UtcNow,
                IsStreaming = true,
                IsComplete = false
            };
            Messages.Add(streamingMessage);
            
            // Stream response
            StreamingText = "";
            var stream = await _chatService.SendMessageStreamingAsync(CurrentSession.Id, userInput);
            
            await foreach (var chunk in stream)
            {
                StreamingText += chunk.Delta;
                streamingMessage.Content = StreamingText;
                
                if (chunk.IsComplete)
                {
                    streamingMessage.IsStreaming = false;
                    streamingMessage.IsComplete = true;
                    break;
                }
            }
            
            // Update session title if first message
            if (Messages.Count(m => m.Role != ChatMessageRole.System) == 2)
            {
                var title = GenerateSessionTitle(userInput);
                await _chatService.UpdateSessionTitleAsync(CurrentSession.Id, title);
                CurrentSession.Title = title;
            }
        }
        catch (Exception ex)
        {
            var errorMessage = new AIChatMessage
            {
                SessionId = CurrentSession.Id,
                Role = ChatMessageRole.System,
                Content = $"❌ Fehler beim Streaming: {ex.Message}",
                Timestamp = DateTime.UtcNow
            };
            Messages.Add(errorMessage);
        }
        finally
        {
            IsStreaming = false;
            IsBusy = false;
            StreamingText = "";
        }
    }
    
    private async Task UseSuggestionAsync(ChatSuggestion? suggestion)
    {
        if (suggestion == null || CurrentSession == null) return;
        
        // Set input text to suggestion prompt
        InputText = suggestion.PromptTemplate;
        
        // If suggestion doesn't require context, send immediately
        if (!suggestion.RequiresContext)
        {
            await SendMessageStreamingAsync();
        }
    }
    
    private async Task ApproveToolCallAsync(MCPToolCall? toolCall)
    {
        if (toolCall == null) return;
        
        try
        {
            IsBusy = true;
            
            await _mcpService.ApproveToolCallAsync(toolCall.Id, _userId);
            
            PendingApprovals.Remove(toolCall);
            
            // Add notification to chat
            if (CurrentSession != null)
            {
                var notification = new AIChatMessage
                {
                    SessionId = CurrentSession.Id,
                    Role = ChatMessageRole.System,
                    Content = $"✓ Tool '{toolCall.ToolName}' wurde genehmigt und ausgeführt.",
                    Timestamp = DateTime.UtcNow
                };
                Messages.Add(notification);
            }
        }
        catch
        {
            // Error handling
        }
        finally
        {
            IsBusy = false;
        }
    }
    
    private async Task AttachDocumentAsync(string? documentId)
    {
        if (string.IsNullOrEmpty(documentId) || CurrentSession == null) return;
        
        await _chatService.AttachDocumentAsync(CurrentSession.Id, documentId);
        
        // Visual feedback
        var notification = new AIChatMessage
        {
            SessionId = CurrentSession.Id,
            Role = ChatMessageRole.System,
            Content = $"📎 Dokument {documentId} wurde angehängt.",
            Timestamp = DateTime.UtcNow
        };
        Messages.Add(notification);
    }
    
    private async Task ClearContextAsync()
    {
        if (CurrentSession == null) return;
        
        // Clear all attached documents
        foreach (var docId in CurrentSession.AttachedDocumentIds.ToList())
        {
            await _chatService.DetachContextAsync(CurrentSession.Id, docId);
        }
        
        CurrentSession.AttachedDocumentIds.Clear();
        CurrentSession.ProcessId = null;
        CurrentSession.DocumentId = null;
    }
    
    private static string GenerateSessionTitle(string firstMessage)
    {
        // Generate a short title from first message
        var title = firstMessage.Length > 50
            ? firstMessage.Substring(0, 47) + "..."
            : firstMessage;
            
        return title;
    }
}


