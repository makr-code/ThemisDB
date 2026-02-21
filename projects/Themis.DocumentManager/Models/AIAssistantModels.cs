/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AIAssistantModels.cs                               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     586                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace Themis.DocumentManager.Models;

// ============================================================================
// AI Assistant / LLM Chat Models (VSCode-Style with SSE & MCP)
// ============================================================================

#region Chat Session Models

/// <summary>
/// AI-Chat-Sitzung - Verwaltet Konversation mit LLM
/// </summary>
public class AIChatSession
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Urn => $"urn:themis:ai-chat-session:{Id}";
    
    public string UserId { get; set; } = string.Empty;
    public string Title { get; set; } = "Neue Konversation";
    
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime LastMessageAt { get; set; } = DateTime.UtcNow;
    
    public List<AIChatMessage> Messages { get; set; } = new();
    
    // Context
    public string? ProcessId { get; set; }
    public string? DocumentId { get; set; }
    public string? FileId { get; set; }
    public List<string> AttachedDocumentIds { get; set; } = new();
    
    // Settings
    public AIModel Model { get; set; } = AIModel.GPT4;
    public double Temperature { get; set; } = 0.7;
    public int MaxTokens { get; set; } = 2000;
    
    // Status
    public bool IsActive { get; set; } = true;
    public ChatSessionStatus Status { get; set; } = ChatSessionStatus.Active;
}

public enum ChatSessionStatus
{
    Active,
    Archived,
    Deleted
}

/// <summary>
/// AI-Chat-Nachricht
/// </summary>
public class AIChatMessage
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string SessionId { get; set; } = string.Empty;
    
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public ChatMessageRole Role { get; set; }
    public string Content { get; set; } = string.Empty;
    
    // Streaming Support (SSE)
    public bool IsStreaming { get; set; } = false;
    public bool IsComplete { get; set; } = true;
    
    // Metadata
    public int TokenCount { get; set; }
    public TimeSpan? ResponseTime { get; set; }
    
    // Attachments & References
    public List<ChatAttachment> Attachments { get; set; } = new();
    public List<ChatReference> References { get; set; } = new();
    
    // Code Blocks & Formatting
    public List<CodeBlock> CodeBlocks { get; set; } = new();
    public MessageFormat Format { get; set; } = MessageFormat.Markdown;
    
    // Actions (wie in VSCode)
    public List<ChatAction> Actions { get; set; } = new();
}

public enum ChatMessageRole
{
    System,    // System-Prompt
    User,      // User-Input
    Assistant, // AI-Antwort
    Tool       // Tool-Ergebnis (MCP)
}

public enum MessageFormat
{
    PlainText,
    Markdown,
    Html
}

/// <summary>
/// Chat-Attachment (Dokumente, Bilder, etc.)
/// </summary>
public class ChatAttachment
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string FileName { get; set; } = string.Empty;
    public string ContentType { get; set; } = string.Empty;
    public long Size { get; set; }
    public string? DocumentId { get; set; }
    public byte[]? Data { get; set; }
    public string? Url { get; set; }
}

/// <summary>
/// Chat-Referenz (zu Dokumenten, Prozessen, etc.)
/// </summary>
public class ChatReference
{
    public string Type { get; set; } = string.Empty; // "Document", "Process", "File"
    public string Id { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string? Url { get; set; }
}

/// <summary>
/// Code-Block (für Code-Snippets in Antworten)
/// </summary>
public class CodeBlock
{
    public string Language { get; set; } = string.Empty;
    public string Code { get; set; } = string.Empty;
    public bool IsExecutable { get; set; } = false;
    public List<string> SuggestedActions { get; set; } = new(); // "Copy", "Insert", "Execute"
}

/// <summary>
/// Chat-Aktion (Quick Actions wie in VSCode)
/// </summary>
public class ChatAction
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Label { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public ChatActionType Type { get; set; }
    public Dictionary<string, object> Parameters { get; set; } = new();
}

public enum ChatActionType
{
    CopyToClipboard,
    InsertAtCursor,
    OpenDocument,
    CreateProcess,
    SearchSimilar,
    Translate,
    Summarize,
    Custom
}

#endregion

#region Model Context Protocol (MCP) Models

/// <summary>
/// MCP-Tool-Definition (Model Context Protocol)
/// </summary>
public class MCPTool
{
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public MCPToolCategory Category { get; set; }
    
    public Dictionary<string, MCPParameter> Parameters { get; set; } = new();
    public MCPToolResponse? ResponseSchema { get; set; }
    
    public bool RequiresApproval { get; set; } = false;
    public List<string> RequiredPermissions { get; set; } = new();
}

public enum MCPToolCategory
{
    Search,           // Suche & Recherche
    Document,         // Dokumentenoperationen
    Process,          // Prozessverwaltung
    File,             // Dateiverwaltung
    Analysis,         // Analyse & Auswertung
    Communication,    // Kommunikation
    Administration,   // Verwaltung
    Custom            // Benutzerdefiniert
}

public class MCPParameter
{
    public string Name { get; set; } = string.Empty;
    public string Type { get; set; } = "string";
    public string Description { get; set; } = string.Empty;
    public bool Required { get; set; } = false;
    public object? DefaultValue { get; set; }
    public List<string>? Enum { get; set; }
}

public class MCPToolResponse
{
    public string Type { get; set; } = "object";
    public Dictionary<string, object> Properties { get; set; } = new();
}

/// <summary>
/// MCP-Tool-Aufruf
/// </summary>
public class MCPToolCall
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string SessionId { get; set; } = string.Empty;
    public string MessageId { get; set; } = string.Empty;
    
    public string ToolName { get; set; } = string.Empty;
    public Dictionary<string, object> Arguments { get; set; } = new();
    
    public DateTime CalledAt { get; set; } = DateTime.UtcNow;
    public DateTime? CompletedAt { get; set; }
    
    public MCPToolCallStatus Status { get; set; } = MCPToolCallStatus.Pending;
    public object? Result { get; set; }
    public string? Error { get; set; }
    
    // Approval (wenn RequiresApproval = true)
    public bool RequiresApproval { get; set; } = false;
    public bool IsApproved { get; set; } = false;
    public string? ApprovedBy { get; set; }
    public DateTime? ApprovedAt { get; set; }
}

public enum MCPToolCallStatus
{
    Pending,          // Warte auf Ausführung
    WaitingApproval,  // Warte auf Genehmigung
    Executing,        // Wird ausgeführt
    Completed,        // Erfolgreich abgeschlossen
    Failed,           // Fehlgeschlagen
    Cancelled         // Abgebrochen
}

/// <summary>
/// Vordefinierte MCP-Tools für ThemisDB DMS
/// </summary>
public static class ThemisDBMCPTools
{
    public static List<MCPTool> GetStandardTools() => new()
    {
        // Suche
        new MCPTool
        {
            Name = "search_documents",
            Description = "Durchsucht Dokumente mit Volltextsuche",
            Category = MCPToolCategory.Search,
            Parameters = new Dictionary<string, MCPParameter>
            {
                ["query"] = new() { Name = "query", Type = "string", Required = true, Description = "Suchbegriff" },
                ["limit"] = new() { Name = "limit", Type = "integer", DefaultValue = 10, Description = "Max. Anzahl Ergebnisse" }
            }
        },
        
        // Dokument öffnen
        new MCPTool
        {
            Name = "open_document",
            Description = "Öffnet ein Dokument zur Ansicht",
            Category = MCPToolCategory.Document,
            Parameters = new Dictionary<string, MCPParameter>
            {
                ["documentId"] = new() { Name = "documentId", Type = "string", Required = true, Description = "Dokument-ID" }
            }
        },
        
        // Prozess erstellen
        new MCPTool
        {
            Name = "create_process",
            Description = "Erstellt einen neuen Vorgang",
            Category = MCPToolCategory.Process,
            RequiresApproval = true,
            Parameters = new Dictionary<string, MCPParameter>
            {
                ["subject"] = new() { Name = "subject", Type = "string", Required = true, Description = "Betreff" },
                ["processType"] = new() { Name = "processType", Type = "string", Required = true, Description = "Vorgangsart" }
            }
        },
        
        // Dokument zusammenfassen
        new MCPTool
        {
            Name = "summarize_document",
            Description = "Erstellt eine Zusammenfassung eines Dokuments",
            Category = MCPToolCategory.Analysis,
            Parameters = new Dictionary<string, MCPParameter>
            {
                ["documentId"] = new() { Name = "documentId", Type = "string", Required = true, Description = "Dokument-ID" },
                ["length"] = new() { Name = "length", Type = "string", DefaultValue = "medium", Enum = new() { "short", "medium", "long" } }
            }
        },
        
        // Ähnliche Dokumente finden
        new MCPTool
        {
            Name = "find_similar",
            Description = "Findet ähnliche Dokumente (semantische Suche)",
            Category = MCPToolCategory.Search,
            Parameters = new Dictionary<string, MCPParameter>
            {
                ["documentId"] = new() { Name = "documentId", Type = "string", Required = true, Description = "Referenz-Dokument" },
                ["limit"] = new() { Name = "limit", Type = "integer", DefaultValue = 5 }
            }
        }
    };
}

#endregion

#region Server-Sent Events (SSE) Models

/// <summary>
/// SSE-Ereignis für Streaming-Antworten
/// </summary>
public class SSEEvent
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = Guid.NewGuid().ToString();
    
    [JsonPropertyName("event")]
    public string Event { get; set; } = "message";
    
    [JsonPropertyName("data")]
    public string Data { get; set; } = string.Empty;
    
    [JsonPropertyName("retry")]
    public int? Retry { get; set; }
    
    public override string ToString()
    {
        var lines = new List<string>();
        
        if (!string.IsNullOrEmpty(Id))
            lines.Add($"id: {Id}");
        
        if (!string.IsNullOrEmpty(Event))
            lines.Add($"event: {Event}");
        
        if (Retry.HasValue)
            lines.Add($"retry: {Retry.Value}");
        
        // Data kann mehrzeilig sein
        foreach (var line in Data.Split('\n'))
            lines.Add($"data: {line}");
        
        lines.Add(""); // Leere Zeile als Trenner
        
        return string.Join("\n", lines);
    }
}

/// <summary>
/// SSE-Stream-Status
/// </summary>
public class SSEStreamStatus
{
    public string SessionId { get; set; } = string.Empty;
    public string MessageId { get; set; } = string.Empty;
    
    public bool IsStreaming { get; set; } = false;
    public int ChunkCount { get; set; } = 0;
    public int TotalTokens { get; set; } = 0;
    
    public DateTime StartedAt { get; set; }
    public DateTime? CompletedAt { get; set; }
    
    public SSEStreamState State { get; set; } = SSEStreamState.Idle;
}

public enum SSEStreamState
{
    Idle,
    Connecting,
    Streaming,
    Completed,
    Error
}

/// <summary>
/// Chat-Stream-Chunk (einzelnes Stück der Streaming-Antwort)
/// </summary>
public class ChatStreamChunk
{
    public string MessageId { get; set; } = string.Empty;
    public string Delta { get; set; } = string.Empty; // Inkrementeller Text
    public int Index { get; set; }
    public bool IsComplete { get; set; } = false;
    public string? FinishReason { get; set; }
}

#endregion

#region AI Models & Configuration

public enum AIModel
{
    GPT4,
    GPT4Turbo,
    GPT35Turbo,
    Claude3Opus,
    Claude3Sonnet,
    Claude3Haiku,
    Llama2,
    Llama370B,
    Mistral,
    Custom
}

public class AIModelConfiguration
{
    public AIModel Model { get; set; }
    public string DisplayName { get; set; } = string.Empty;
    public string Provider { get; set; } = string.Empty; // "OpenAI", "Anthropic", "Ollama"
    
    public string ApiEndpoint { get; set; } = string.Empty;
    public string? ApiKey { get; set; }
    
    public int MaxTokens { get; set; } = 4096;
    public double DefaultTemperature { get; set; } = 0.7;
    
    public bool SupportsStreaming { get; set; } = true;
    public bool SupportsFunctionCalling { get; set; } = true;
    public bool SupportsVision { get; set; } = false;
    
    public List<string> SupportedLanguages { get; set; } = new();
}

/// <summary>
/// Chat-Konfiguration
/// </summary>
public class AIChatConfiguration
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string UserId { get; set; } = string.Empty;
    
    // Model
    public AIModel PreferredModel { get; set; } = AIModel.GPT4;
    public double DefaultTemperature { get; set; } = 0.7;
    public int DefaultMaxTokens { get; set; } = 2000;
    
    // Features
    public bool EnableStreaming { get; set; } = true;
    public bool EnableMCP { get; set; } = true;
    public bool EnableCodeBlocks { get; set; } = true;
    public bool EnableAutoSave { get; set; } = true;
    
    // Context
    public bool AutoAttachCurrentDocument { get; set; } = true;
    public bool AutoAttachCurrentProcess { get; set; } = true;
    public int MaxContextDocuments { get; set; } = 5;
    
    // UI
    public ChatUITheme Theme { get; set; } = ChatUITheme.VSCodeDark;
    public bool ShowTokenCount { get; set; } = true;
    public bool ShowTimestamps { get; set; } = true;
    public int MaxMessagesShown { get; set; } = 100;
}

public enum ChatUITheme
{
    VSCodeDark,
    VSCodeLight,
    Custom
}

#endregion

#region Chat Suggestions & Quick Actions

/// <summary>
/// Chat-Vorschlag (wie "/explain", "/fix" in VSCode)
/// </summary>
public class ChatSuggestion
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Label { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    public string PromptTemplate { get; set; } = string.Empty;
    public ChatSuggestionCategory Category { get; set; }
    
    public bool RequiresContext { get; set; } = false;
    public List<string> RequiredContextTypes { get; set; } = new(); // "document", "process", "selection"
}

public enum ChatSuggestionCategory
{
    Explain,       // Erklärungen
    Generate,      // Generierung
    Transform,     // Transformation
    Analyze,       // Analyse
    Search,        // Suche
    Action,        // Aktionen
    Custom         // Benutzerdefiniert
}

/// <summary>
/// Standard-Vorschläge für ThemisDB DMS
/// </summary>
public static class ThemisDBChatSuggestions
{
    public static List<ChatSuggestion> GetStandardSuggestions() => new()
    {
        new ChatSuggestion
        {
            Label = "/zusammenfassen",
            Icon = "📝",
            Description = "Erstellt eine Zusammenfassung des aktuellen Dokuments",
            PromptTemplate = "Erstelle eine prägnante Zusammenfassung des folgenden Dokuments:\n\n{document_content}",
            Category = ChatSuggestionCategory.Analyze,
            RequiresContext = true,
            RequiredContextTypes = new() { "document" }
        },
        
        new ChatSuggestion
        {
            Label = "/suchen",
            Icon = "🔍",
            Description = "Durchsucht das DMS nach ähnlichen Dokumenten",
            PromptTemplate = "Suche nach Dokumenten zu: {user_input}",
            Category = ChatSuggestionCategory.Search
        },
        
        new ChatSuggestion
        {
            Label = "/vorgang",
            Icon = "📋",
            Description = "Erstellt einen neuen Vorgang basierend auf dem Kontext",
            PromptTemplate = "Erstelle einen Vorgang mit folgenden Informationen:\n{user_input}",
            Category = ChatSuggestionCategory.Action
        },
        
        new ChatSuggestion
        {
            Label = "/prüfen",
            Icon = "✓",
            Description = "Prüft Dokument auf Compliance und Fehler",
            PromptTemplate = "Prüfe das folgende Dokument auf Compliance (DSGVO, GoBD) und formale Fehler:\n\n{document_content}",
            Category = ChatSuggestionCategory.Analyze,
            RequiresContext = true,
            RequiredContextTypes = new() { "document" }
        },
        
        new ChatSuggestion
        {
            Label = "/übersetzen",
            Icon = "🌍",
            Description = "Übersetzt den markierten Text",
            PromptTemplate = "Übersetze folgenden Text ins {target_language}:\n\n{selection}",
            Category = ChatSuggestionCategory.Transform,
            RequiresContext = true,
            RequiredContextTypes = new() { "selection" }
        }
    };
}

#endregion
