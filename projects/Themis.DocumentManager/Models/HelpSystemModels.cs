/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            HelpSystemModels.cs                                ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     602                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

// ============================================================================
// Integrierte Hilfe / Help System Models
// ============================================================================

#region Help System Models

/// <summary>
/// Hilfe-Artikel
/// </summary>
public class HelpArticle
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Urn => $"urn:themis:help-article:{Id}";
    
    public string Title { get; set; } = string.Empty;
    public string Category { get; set; } = string.Empty;
    public HelpArticleType Type { get; set; }
    
    public string Content { get; set; } = string.Empty;
    public string ContentFormat { get; set; } = "markdown"; // markdown, html, plaintext
    
    // Metadaten
    public List<string> Tags { get; set; } = new();
    public List<string> Keywords { get; set; } = new();
    public string Excerpt { get; set; } = string.Empty;
    
    // Kontext
    public string? TargetFeature { get; set; } // z.B. "inbox", "process", "timeline"
    public string? TargetView { get; set; } // z.B. "InboxView", "ProcessView"
    public HelpTargetAudience Audience { get; set; } = HelpTargetAudience.All;
    
    // Verwandte Artikel
    public List<string> RelatedArticleIds { get; set; } = new();
    public List<HelpLink> RelatedLinks { get; set; } = new();
    
    // Multimedia
    public List<HelpAttachment> Attachments { get; set; } = new();
    public List<HelpVideo> Videos { get; set; } = new();
    public List<HelpScreenshot> Screenshots { get; set; } = new();
    
    // Versionierung
    public string Version { get; set; } = "1.0";
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime UpdatedAt { get; set; } = DateTime.UtcNow;
    public string Author { get; set; } = string.Empty;
    
    // Bewertung
    public int Views { get; set; } = 0;
    public double Rating { get; set; } = 0.0;
    public int RatingCount { get; set; } = 0;
    public bool IsFavorite { get; set; } = false;
}

public enum HelpArticleType
{
    Tutorial,          // Schritt-für-Schritt-Anleitung
    HowTo,            // Wie macht man...
    Concept,          // Konzept-Erklärung
    Reference,        // Referenz-Dokumentation
    Troubleshooting,  // Problemlösung
    FAQ,              // Häufig gestellte Fragen
    QuickStart,       // Schnellstart
    BestPractice,     // Best Practices
    Video,            // Video-Tutorial
    Release           // Release Notes
}

public enum HelpTargetAudience
{
    All,              // Alle Benutzer
    EndUser,          // Endbenutzer
    PowerUser,        // Erfahrene Benutzer
    Administrator,    // Administratoren
    Developer         // Entwickler
}

/// <summary>
/// Hilfe-Link (externe Ressource)
/// </summary>
public class HelpLink
{
    public string Title { get; set; } = string.Empty;
    public string Url { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public HelpLinkType Type { get; set; }
}

public enum HelpLinkType
{
    Documentation,    // Dokumentation
    Video,           // Video
    Website,         // Webseite
    Download,        // Download
    Forum,           // Forum
    Support          // Support
}

/// <summary>
/// Hilfe-Anhang
/// </summary>
public class HelpAttachment
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string FileName { get; set; } = string.Empty;
    public string ContentType { get; set; } = string.Empty;
    public long Size { get; set; }
    public string? Url { get; set; }
    public byte[]? Data { get; set; }
}

/// <summary>
/// Hilfe-Video
/// </summary>
public class HelpVideo
{
    public string Title { get; set; } = string.Empty;
    public string Url { get; set; } = string.Empty;
    public string? ThumbnailUrl { get; set; }
    public TimeSpan Duration { get; set; }
    public string Platform { get; set; } = "YouTube"; // YouTube, Vimeo, etc.
}

/// <summary>
/// Hilfe-Screenshot
/// </summary>
public class HelpScreenshot
{
    public string Title { get; set; } = string.Empty;
    public string? Caption { get; set; }
    public string Url { get; set; } = string.Empty;
    public byte[]? ImageData { get; set; }
    public int Width { get; set; }
    public int Height { get; set; }
}

#endregion

#region Interactive Help Models

/// <summary>
/// Interaktive Tour (wie Feature-Walkthrough)
/// </summary>
public class InteractiveTour
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    public List<TourStep> Steps { get; set; } = new();
    
    public string TargetFeature { get; set; } = string.Empty;
    public bool IsRequired { get; set; } = false; // Pflicht-Tour für neue Benutzer
    public int EstimatedMinutes { get; set; }
    
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// Tour-Schritt
/// </summary>
public class TourStep
{
    public int Order { get; set; }
    public string Title { get; set; } = string.Empty;
    public string Content { get; set; } = string.Empty;
    
    // UI-Element-Targeting
    public string? TargetElementId { get; set; }
    public string? TargetSelector { get; set; }
    public TourStepPosition Position { get; set; } = TourStepPosition.Bottom;
    
    // Interaktion
    public TourStepAction? RequiredAction { get; set; }
    public string? NextButtonText { get; set; } = "Weiter";
    public string? BackButtonText { get; set; } = "Zurück";
    
    // Medien
    public string? ImageUrl { get; set; }
    public string? VideoUrl { get; set; }
}

public enum TourStepPosition
{
    Top,
    Bottom,
    Left,
    Right,
    Center
}

public class TourStepAction
{
    public string Type { get; set; } = string.Empty; // "click", "input", "navigate"
    public string Target { get; set; } = string.Empty;
    public bool IsOptional { get; set; } = false;
}

/// <summary>
/// Tour-Fortschritt (User-spezifisch)
/// </summary>
public class TourProgress
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string UserId { get; set; } = string.Empty;
    public string TourId { get; set; } = string.Empty;
    
    public int CurrentStep { get; set; } = 0;
    public bool IsCompleted { get; set; } = false;
    public DateTime? CompletedAt { get; set; }
    
    public DateTime StartedAt { get; set; } = DateTime.UtcNow;
    public DateTime LastAccessedAt { get; set; } = DateTime.UtcNow;
}

#endregion

#region Contextual Help Models

/// <summary>
/// Kontextsensitive Hilfe
/// </summary>
public class ContextualHelp
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Context { get; set; } = string.Empty; // z.B. "InboxView", "CreateProcess"
    
    public string QuickTip { get; set; } = string.Empty;
    public List<string> RelatedArticleIds { get; set; } = new();
    public List<HelpTip> Tips { get; set; } = new();
    public List<HelpShortcut> Shortcuts { get; set; } = new();
}

/// <summary>
/// Hilfe-Tipp
/// </summary>
public class HelpTip
{
    public string Title { get; set; } = string.Empty;
    public string Content { get; set; } = string.Empty;
    public string Icon { get; set; } = "💡";
    public HelpTipLevel Level { get; set; } = HelpTipLevel.Info;
}

public enum HelpTipLevel
{
    Info,
    Success,
    Warning,
    Error,
    ProTip
}

/// <summary>
/// Tastenkombination
/// </summary>
public class HelpShortcut
{
    public string Action { get; set; } = string.Empty;
    public string Keys { get; set; } = string.Empty; // z.B. "Ctrl+S"
    public string Description { get; set; } = string.Empty;
    public string Category { get; set; } = string.Empty;
}

#endregion

#region Help Search & Navigation

/// <summary>
/// Hilfe-Suche
/// </summary>
public class HelpSearchQuery
{
    public string Query { get; set; } = string.Empty;
    public List<string> Categories { get; set; } = new();
    public List<HelpArticleType> Types { get; set; } = new();
    public HelpTargetAudience? Audience { get; set; }
    public List<string> Tags { get; set; } = new();
    
    public int MaxResults { get; set; } = 20;
}

/// <summary>
/// Hilfe-Suchergebnis
/// </summary>
public class HelpSearchResult
{
    public string ArticleId { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Excerpt { get; set; } = string.Empty;
    public HelpArticleType Type { get; set; }
    public string Category { get; set; } = string.Empty;
    
    public double Relevance { get; set; }
    public List<string> MatchedKeywords { get; set; } = new();
    public string? HighlightedExcerpt { get; set; }
}

/// <summary>
/// Hilfe-Navigation
/// </summary>
public class HelpNavigationNode
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Title { get; set; } = string.Empty;
    public string? ArticleId { get; set; }
    public string Icon { get; set; } = "📄";
    
    public List<HelpNavigationNode> Children { get; set; } = new();
    public int Order { get; set; }
    public bool IsExpanded { get; set; } = false;
}

#endregion

#region Help Analytics

/// <summary>
/// Hilfe-Nutzungsstatistik
/// </summary>
public class HelpUsageStatistics
{
    public string ArticleId { get; set; } = string.Empty;
    public int TotalViews { get; set; }
    public int UniqueViews { get; set; }
    public double AverageTimeSpent { get; set; } // in Sekunden
    public double AverageRating { get; set; }
    public int HelpfulCount { get; set; }
    public int NotHelpfulCount { get; set; }
    
    public DateTime LastViewed { get; set; }
    public DateTime FirstViewed { get; set; }
}

/// <summary>
/// Hilfe-Feedback
/// </summary>
public class HelpFeedback
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string ArticleId { get; set; } = string.Empty;
    public string UserId { get; set; } = string.Empty;
    
    public bool IsHelpful { get; set; }
    public int? Rating { get; set; } // 1-5 Sterne
    public string? Comment { get; set; }
    
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// Hilfe-Anfrage (User kann Frage stellen)
/// </summary>
public class HelpRequest
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string UserId { get; set; } = string.Empty;
    public string UserName { get; set; } = string.Empty;
    
    public string Question { get; set; } = string.Empty;
    public string? Context { get; set; }
    public string? Screenshot { get; set; }
    
    public HelpRequestStatus Status { get; set; } = HelpRequestStatus.Open;
    public string? Answer { get; set; }
    public string? AnsweredBy { get; set; }
    public DateTime? AnsweredAt { get; set; }
    
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

public enum HelpRequestStatus
{
    Open,
    InProgress,
    Answered,
    Closed
}

#endregion

#region Predefined Help Content

/// <summary>
/// Standard-Hilfekategorien für ThemisDB DMS
/// </summary>
public static class ThemisDBHelpCategories
{
    public static List<string> GetStandardCategories() => new()
    {
        "Erste Schritte",
        "Posteingang & Postausgang",
        "Vorgangsbearbeitung",
        "Dokumentenverwaltung",
        "Suche & Filter",
        "Timeline & Gantt",
        "AI-Assistent",
        "Benachrichtigungen",
        "Mitzeichnung",
        "Wiedervorlage & Fristen",
        "Aktenplan",
        "Formulare",
        "E-Mail-Integration",
        "Scannen & OCR",
        "Messenger-Integration",
        "4-Augen-Prinzip",
        "Akteneinsicht",
        "Stellvertretung",
        "eGov-Schnittstellen",
        "Administration",
        "Einstellungen",
        "Tastenkombinationen",
        "Problemlösung",
        "FAQs"
    };
}

/// <summary>
/// Standard-Hilfe-Artikel für ThemisDB DMS
/// </summary>
public static class ThemisDBHelpArticles
{
    public static List<HelpArticle> GetQuickStartArticles() => new()
    {
        new HelpArticle
        {
            Title = "Willkommen bei ThemisDB Document Manager",
            Category = "Erste Schritte",
            Type = HelpArticleType.QuickStart,
            Excerpt = "Eine kurze Einführung in ThemisDB Document Manager und seine Hauptfunktionen.",
            Content = @"# Willkommen bei ThemisDB Document Manager

ThemisDB Document Manager ist ein modernes Dokumentenmanagementsystem für die öffentliche Verwaltung.

## Hauptfunktionen

- **Posteingang & Postausgang**: Verwalten Sie eingehende und ausgehende Dokumente
- **Vorgangsbearbeitung**: Erstellen und bearbeiten Sie Vorgänge
- **Timeline**: Visualisieren Sie den Verlauf Ihrer Vorgänge
- **AI-Assistent**: Nutzen Sie KI-Unterstützung für Ihre Arbeit
- **Suche**: Finden Sie schnell Dokumente und Vorgänge

## Erste Schritte

1. Melden Sie sich mit Ihren Zugangsdaten an
2. Erkunden Sie den Posteingang
3. Erstellen Sie Ihren ersten Vorgang
4. Nutzen Sie den AI-Assistenten für Fragen

Viel Erfolg!",
            Tags = new List<string> { "einführung", "quickstart", "anfänger" },
            Keywords = new List<string> { "erste schritte", "willkommen", "einführung", "start" },
            Audience = HelpTargetAudience.All
        },
        
        new HelpArticle
        {
            Title = "Posteingang verwenden",
            Category = "Posteingang & Postausgang",
            Type = HelpArticleType.HowTo,
            TargetFeature = "inbox",
            Excerpt = "Erfahren Sie, wie Sie mit dem Posteingang arbeiten.",
            Content = @"# Posteingang verwenden

Der Posteingang ist Ihre zentrale Anlaufstelle für eingehende Dokumente.

## Dokument im Posteingang öffnen

1. Navigieren Sie zum Posteingang
2. Klicken Sie auf ein Dokument
3. Das Dokument wird in der Detailansicht geöffnet

## Dokument zuweisen

1. Wählen Sie ein Dokument
2. Klicken Sie auf 'Zuweisen'
3. Wählen Sie den Empfänger
4. Bestätigen Sie

## Filtern und Suchen

- Nutzen Sie die Filter in der Seitenleiste
- Verwenden Sie die Suchleiste für Volltextsuche
- Kombinieren Sie mehrere Filter

## Tastenkombinationen

- `N`: Neues Dokument
- `F`: Filter öffnen
- `S`: Suche aktivieren
- `/`: Fokus auf Suchfeld",
            Tags = new List<string> { "posteingang", "inbox", "dokumente" },
            Keywords = new List<string> { "posteingang", "inbox", "zuweisen", "filter", "suche" },
            Audience = HelpTargetAudience.EndUser
        },
        
        new HelpArticle
        {
            Title = "AI-Assistent verwenden",
            Category = "AI-Assistent",
            Type = HelpArticleType.Tutorial,
            TargetFeature = "ai-chat",
            Excerpt = "Nutzen Sie den AI-Assistenten für Ihre tägliche Arbeit.",
            Content = @"# AI-Assistent verwenden

Der AI-Assistent hilft Ihnen bei vielen Aufgaben im DMS.

## Chat starten

1. Öffnen Sie das AI-Panel auf der linken Seite
2. Tippen Sie Ihre Frage ein
3. Drücken Sie Enter oder klicken Sie auf Senden

## Nützliche Befehle

- `/zusammenfassen`: Dokument zusammenfassen
- `/suchen`: Nach Dokumenten suchen
- `/vorgang`: Neuen Vorgang erstellen
- `/prüfen`: Dokument auf Compliance prüfen
- `/übersetzen`: Text übersetzen

## Dokumente anhängen

1. Ziehen Sie ein Dokument in den Chat
2. Oder klicken Sie auf das Büroklammer-Symbol
3. Der AI-Assistent kann nun auf das Dokument zugreifen

## Streaming-Antworten

Aktivieren Sie 'Streaming' für Live-Antworten, die Wort für Wort erscheinen.

## MCP-Tools

Der AI-Assistent kann Tools verwenden:
- Dokumente suchen
- Vorgänge erstellen
- Ähnliche Dokumente finden

Bei kritischen Aktionen werden Sie um Genehmigung gebeten.",
            Tags = new List<string> { "ai", "assistent", "chat", "llm", "ki" },
            Keywords = new List<string> { "ai", "assistent", "chat", "ki", "hilfe", "fragen" },
            Audience = HelpTargetAudience.All
        }
    };
    
    public static List<HelpShortcut> GetStandardShortcuts() => new()
    {
        // Allgemein
        new HelpShortcut { Action = "Hilfe öffnen", Keys = "F1", Category = "Allgemein" },
        new HelpShortcut { Action = "Suche", Keys = "Ctrl+F", Category = "Allgemein" },
        new HelpShortcut { Action = "Speichern", Keys = "Ctrl+S", Category = "Allgemein" },
        new HelpShortcut { Action = "Schließen", Keys = "Esc", Category = "Allgemein" },
        
        // Navigation
        new HelpShortcut { Action = "Posteingang", Keys = "Ctrl+1", Category = "Navigation" },
        new HelpShortcut { Action = "Postausgang", Keys = "Ctrl+2", Category = "Navigation" },
        new HelpShortcut { Action = "Timeline", Keys = "Ctrl+3", Category = "Navigation" },
        new HelpShortcut { Action = "AI-Assistent", Keys = "Ctrl+K", Category = "Navigation" },
        
        // Aktionen
        new HelpShortcut { Action = "Neuer Vorgang", Keys = "Ctrl+N", Category = "Aktionen" },
        new HelpShortcut { Action = "Dokument öffnen", Keys = "Ctrl+O", Category = "Aktionen" },
        new HelpShortcut { Action = "Zuweisen", Keys = "Ctrl+Shift+A", Category = "Aktionen" },
        
        // AI-Assistent
        new HelpShortcut { Action = "Neue Konversation", Keys = "Ctrl+Shift+N", Category = "AI-Assistent" },
        new HelpShortcut { Action = "Nachricht senden", Keys = "Enter", Category = "AI-Assistent" },
        new HelpShortcut { Action = "Mehrzeilige Eingabe", Keys = "Shift+Enter", Category = "AI-Assistent" }
    };
}

#endregion
