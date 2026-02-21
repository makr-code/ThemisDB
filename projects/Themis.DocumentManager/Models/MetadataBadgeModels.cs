/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataBadgeModels.cs                             ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     891                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#nullable enable
using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Intelligentes Metadata-Badge-System für DSM
/// Automatische Erkennung, Kategorisierung und visuelle Darstellung von Metadaten
/// </summary>

#region Badge Configuration

/// <summary>
/// Badge-Typ für verschiedene Metadaten-Kategorien
/// </summary>
public enum BadgeType
{
    Date,               // Datum (z.B. 2025-11-12)
    Department,         // Abteilung (z.B. T26, T11)
    ProcessType,        // Vorgangstyp (z.B. STN = Stellungnahme)
    FileReference,      // Aktenzeichen (z.B. GV078/22)
    Status,            // Status (z.B. Offen, Abgeschlossen)
    Priority,          // Priorität (Hoch, Normal, Niedrig)
    Person,            // Person (Name)
    Organization,      // Organisation
    Location,          // Ort/Standort
    Topic,             // Thema/Schlagwort
    Action,            // Aktion (z.B. Genehmigung, Ablehnung)
    Deadline,          // Frist
    Custom             // Benutzerdefiniert
}

/// <summary>
/// Badge-Stil-Konfiguration mit Farben und Icons
/// </summary>
public class BadgeStyle
{
    public string BackgroundColor { get; set; } = "#E0E0E0";
    public string TextColor { get; set; } = "#000000";
    public string BorderColor { get; set; } = "#CCCCCC";
    public string Icon { get; set; } = string.Empty; // Unicode icon or emoji
    public string? FontAwesomeIcon { get; set; }
    public bool ShowIcon { get; set; } = true;
    public bool ShowTooltip { get; set; } = true;
    public int FontSize { get; set; } = 12;
    public string FontWeight { get; set; } = "normal";
    
    /// <summary>
    /// Vordefinierte Stile für verschiedene Badge-Typen
    /// </summary>
    public static Dictionary<BadgeType, BadgeStyle> DefaultStyles => new()
    {
        [BadgeType.Date] = new BadgeStyle
        {
            BackgroundColor = "#E3F2FD",
            TextColor = "#1565C0",
            BorderColor = "#90CAF9",
            Icon = "📅",
            FontAwesomeIcon = "calendar"
        },
        [BadgeType.Department] = new BadgeStyle
        {
            BackgroundColor = "#FFF3E0",
            TextColor = "#E65100",
            BorderColor = "#FFB74D",
            Icon = "🏢",
            FontAwesomeIcon = "building"
        },
        [BadgeType.ProcessType] = new BadgeStyle
        {
            BackgroundColor = "#F3E5F5",
            TextColor = "#6A1B9A",
            BorderColor = "#CE93D8",
            Icon = "⚙️",
            FontAwesomeIcon = "cog"
        },
        [BadgeType.FileReference] = new BadgeStyle
        {
            BackgroundColor = "#E8F5E9",
            TextColor = "#2E7D32",
            BorderColor = "#81C784",
            Icon = "📁",
            FontAwesomeIcon = "folder"
        },
        [BadgeType.Status] = new BadgeStyle
        {
            BackgroundColor = "#FFF9C4",
            TextColor = "#F57F17",
            BorderColor = "#FFF59D",
            Icon = "⚡",
            FontAwesomeIcon = "bolt"
        },
        [BadgeType.Priority] = new BadgeStyle
        {
            BackgroundColor = "#FFEBEE",
            TextColor = "#C62828",
            BorderColor = "#EF9A9A",
            Icon = "🔥",
            FontAwesomeIcon = "fire"
        },
        [BadgeType.Person] = new BadgeStyle
        {
            BackgroundColor = "#E1F5FE",
            TextColor = "#01579B",
            BorderColor = "#81D4FA",
            Icon = "👤",
            FontAwesomeIcon = "user"
        },
        [BadgeType.Organization] = new BadgeStyle
        {
            BackgroundColor = "#FCE4EC",
            TextColor = "#880E4F",
            BorderColor = "#F48FB1",
            Icon = "🏛️",
            FontAwesomeIcon = "university"
        },
        [BadgeType.Location] = new BadgeStyle
        {
            BackgroundColor = "#E0F2F1",
            TextColor = "#00695C",
            BorderColor = "#80CBC4",
            Icon = "📍",
            FontAwesomeIcon = "map-marker"
        },
        [BadgeType.Topic] = new BadgeStyle
        {
            BackgroundColor = "#F1F8E9",
            TextColor = "#558B2F",
            BorderColor = "#C5E1A5",
            Icon = "🏷️",
            FontAwesomeIcon = "tag"
        },
        [BadgeType.Action] = new BadgeStyle
        {
            BackgroundColor = "#FBE9E7",
            TextColor = "#BF360C",
            BorderColor = "#FFAB91",
            Icon = "✅",
            FontAwesomeIcon = "check-circle"
        },
        [BadgeType.Deadline] = new BadgeStyle
        {
            BackgroundColor = "#FFCDD2",
            TextColor = "#B71C1C",
            BorderColor = "#EF5350",
            Icon = "⏰",
            FontAwesomeIcon = "clock"
        }
    };
}

#endregion

#region Badge Model

/// <summary>
/// Metadata-Badge mit automatischer Erkennung und Kategorisierung
/// </summary>
public class MetadataBadge
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    
    /// <summary>
    /// Anzeigetext des Badges
    /// </summary>
    public string DisplayText { get; set; } = string.Empty;
    
    /// <summary>
    /// Ursprünglicher extrahierter Text
    /// </summary>
    public string RawText { get; set; } = string.Empty;
    
    /// <summary>
    /// Badge-Typ (automatisch erkannt)
    /// </summary>
    public BadgeType Type { get; set; }
    
    /// <summary>
    /// Badge-Stil
    /// </summary>
    public BadgeStyle Style { get; set; } = new();
    
    /// <summary>
    /// Tooltip-Text mit Details
    /// </summary>
    public string Tooltip { get; set; } = string.Empty;
    
    /// <summary>
    /// Erkennungsgenauigkeit (0.0 - 1.0)
    /// </summary>
    public double Confidence { get; set; }
    
    /// <summary>
    /// Position im Originaltext
    /// </summary>
    public int StartPosition { get; set; }
    public int EndPosition { get; set; }
    
    /// <summary>
    /// Semantisches Feld (für Datenbank-Mapping)
    /// </summary>
    public string? SemanticField { get; set; }
    
    /// <summary>
    /// Strukturierte Daten
    /// </summary>
    public Dictionary<string, object> StructuredData { get; set; } = new();
    
    /// <summary>
    /// Verwandte Badges (für Kontext)
    /// </summary>
    public List<string> RelatedBadgeIds { get; set; } = new();
    
    /// <summary>
    /// Ist anklickbar/interaktiv
    /// </summary>
    public bool IsInteractive { get; set; } = true;
    
    /// <summary>
    /// Action beim Klick
    /// </summary>
    public BadgeAction? ClickAction { get; set; }
}

/// <summary>
/// Aktion die beim Badge-Klick ausgeführt wird
/// </summary>
public class BadgeAction
{
    public BadgeActionType Type { get; set; }
    public string Target { get; set; } = string.Empty;
    public Dictionary<string, object> Parameters { get; set; } = new();
}

public enum BadgeActionType
{
    Navigate,           // Navigation zu Dokument/Akte
    Search,            // Suche nach ähnlichen
    OpenDetails,       // Details anzeigen
    EditField,         // Feld bearbeiten
    ShowRelated,       // Verwandte anzeigen
    CopyToClipboard,   // In Zwischenablage
    Filter            // Filter anwenden
}

#endregion

#region Smart Input Field

/// <summary>
/// Intelligentes Eingabefeld mit Badge-Unterstützung und Vorschlägen
/// </summary>
public class SmartInputField
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string FieldName { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    
    /// <summary>
    /// Aktueller Wert
    /// </summary>
    public string Value { get; set; } = string.Empty;
    
    /// <summary>
    /// Erkannte Badges im Eingabefeld
    /// </summary>
    public List<MetadataBadge> Badges { get; set; } = new();
    
    /// <summary>
    /// Aktuelle Vorschläge
    /// </summary>
    public List<InputSuggestion> Suggestions { get; set; } = new();
    
    /// <summary>
    /// Konfiguration des Feldes
    /// </summary>
    public SmartInputConfig Config { get; set; } = new();
    
    /// <summary>
    /// Validierungsstatus
    /// </summary>
    public ValidationResult? Validation { get; set; }
}

/// <summary>
/// Konfiguration für Smart Input Fields
/// </summary>
public class SmartInputConfig
{
    /// <summary>
    /// Automatische Badge-Erkennung aktiviert
    /// </summary>
    public bool EnableAutoBadging { get; set; } = true;
    
    /// <summary>
    /// Semantische Vorschläge aktiviert
    /// </summary>
    public bool EnableSemanticSuggestions { get; set; } = true;
    
    /// <summary>
    /// Autocomplete aktiviert
    /// </summary>
    public bool EnableAutocomplete { get; set; } = true;
    
    /// <summary>
    /// Echtzeitvalidierung
    /// </summary>
    public bool EnableRealtimeValidation { get; set; } = true;
    
    /// <summary>
    /// Erlaubte Badge-Typen
    /// </summary>
    public List<BadgeType> AllowedBadgeTypes { get; set; } = new();
    
    /// <summary>
    /// Minimale Konfidenz für Badges (0.0 - 1.0)
    /// </summary>
    public double MinimumConfidence { get; set; } = 0.7;
    
    /// <summary>
    /// Maximale Anzahl Vorschläge
    /// </summary>
    public int MaxSuggestions { get; set; } = 10;
    
    /// <summary>
    /// Fuzzy-Matching aktiviert
    /// </summary>
    public bool EnableFuzzyMatching { get; set; } = true;
    
    /// <summary>
    /// Template-Patterns für Erkennung
    /// </summary>
    public List<BadgePattern> CustomPatterns { get; set; } = new();
}

#endregion

#region Suggestions & Recognition

/// <summary>
/// Eingabe-Vorschlag mit semantischer Ähnlichkeit
/// </summary>
public class InputSuggestion
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Text { get; set; } = string.Empty;
    public string DisplayText { get; set; } = string.Empty;
    
    /// <summary>
    /// Ähnlichkeits-Score (0.0 - 1.0)
    /// </summary>
    public double SimilarityScore { get; set; }
    
    /// <summary>
    /// Vorschlagstyp
    /// </summary>
    public SuggestionType Type { get; set; }
    
    /// <summary>
    /// Quelle des Vorschlags
    /// </summary>
    public SuggestionSource Source { get; set; }
    
    /// <summary>
    /// Zusätzliche Informationen
    /// </summary>
    public string Description { get; set; } = string.Empty;
    
    /// <summary>
    /// Häufigkeit der Verwendung
    /// </summary>
    public int UsageCount { get; set; }
    
    /// <summary>
    /// Letztes Verwendungsdatum
    /// </summary>
    public DateTime? LastUsed { get; set; }
    
    /// <summary>
    /// Kontext-relevanz
    /// </summary>
    public double ContextRelevance { get; set; }
    
    /// <summary>
    /// Badges im Vorschlag
    /// </summary>
    public List<MetadataBadge> Badges { get; set; } = new();
}

public enum SuggestionType
{
    Exact,              // Exakte Übereinstimmung
    Fuzzy,              // Fuzzy Match
    Semantic,           // Semantisch ähnlich
    Historical,         // Historisch verwendet
    Template,           // Aus Template
    Related,            // Verwandt/Ähnlich
    Autocomplete,       // Autocomplete
    Synonym,            // Synonym
    Abbreviation        // Abkürzung/Expansion
}

public enum SuggestionSource
{
    Database,           // Aus Datenbank
    LLM,               // Von LLM generiert
    UserHistory,       // Aus Benutzerhistorie
    SystemTemplate,    // System-Template
    FilingPlan,        // Aus Aktenplan
    Dictionary,        // Aus Wörterbuch
    Context,           // Aus Kontext
    Manual             // Manuell definiert
}

/// <summary>
/// Pattern für Badge-Erkennung
/// </summary>
public class BadgePattern
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Name { get; set; } = string.Empty;
    public BadgeType TargetType { get; set; }
    
    /// <summary>
    /// Regex-Pattern für Erkennung
    /// </summary>
    public string Pattern { get; set; } = string.Empty;
    
    /// <summary>
    /// Beispiele für das Pattern
    /// </summary>
    public List<string> Examples { get; set; } = new();
    
    /// <summary>
    /// Extraktions-Template
    /// </summary>
    public string? ExtractionTemplate { get; set; }
    
    /// <summary>
    /// Priorität (höher = wichtiger)
    /// </summary>
    public int Priority { get; set; } = 100;
    
    /// <summary>
    /// Ist aktiviert
    /// </summary>
    public bool IsEnabled { get; set; } = true;
    
    /// <summary>
    /// Vordefinierte Patterns für deutsche Verwaltung
    /// </summary>
    public static List<BadgePattern> GermanAdministrationPatterns => new()
    {
        // Datum-Patterns
        new BadgePattern
        {
            Name = "ISO Date",
            TargetType = BadgeType.Date,
            Pattern = @"\d{4}-\d{2}-\d{2}",
            Examples = new List<string> { "2025-11-12", "2024-01-15" },
            Priority = 100
        },
        new BadgePattern
        {
            Name = "German Date",
            TargetType = BadgeType.Date,
            Pattern = @"\d{1,2}\.\d{1,2}\.\d{4}",
            Examples = new List<string> { "12.11.2025", "15.01.2024" },
            Priority = 90
        },
        
        // Abteilungs-Patterns
        new BadgePattern
        {
            Name = "Department Code",
            TargetType = BadgeType.Department,
            Pattern = @"[A-Z]{1,3}\d{1,3}",
            Examples = new List<string> { "T26", "T11", "IT5", "HR2" },
            Priority = 95
        },
        
        // Aktenzeichen-Patterns
        new BadgePattern
        {
            Name = "File Reference Standard",
            TargetType = BadgeType.FileReference,
            Pattern = @"[A-Z]{2,4}\d{3,4}/\d{2}",
            Examples = new List<string> { "GV078/22", "BA123/24", "IT456/23" },
            Priority = 95
        },
        new BadgePattern
        {
            Name = "File Reference Extended",
            TargetType = BadgeType.FileReference,
            Pattern = @"[A-Z]{1,4}\s[A-Z]\s\d{1,2}\s-\s\d{1,4}/\d{4}",
            Examples = new List<string> { "IV C 5 - 123/2024", "II A 3 - 456/2023" },
            Priority = 100
        },
        
        // Vorgangstyp-Abkürzungen
        new BadgePattern
        {
            Name = "Process Type Abbreviation",
            TargetType = BadgeType.ProcessType,
            Pattern = @"\b(STN|GEN|ABL|GV|BE|ANF|ANT|ENTSCH)\b",
            Examples = new List<string> { "STN", "GEN", "ABL", "GV" },
            Priority = 90
        },
        
        // Frist-Patterns
        new BadgePattern
        {
            Name = "Deadline Pattern",
            TargetType = BadgeType.Deadline,
            Pattern = @"(Frist|bis|spätestens|Termin):\s*\d{1,2}\.\d{1,2}\.\d{4}",
            Examples = new List<string> { "Frist: 15.12.2024", "bis 30.11.2025" },
            Priority = 85
        }
    };
}

#endregion

#region Validation

/// <summary>
/// Validierungsergebnis für Eingabefelder
/// </summary>
public class ValidationResult
{
    public bool IsValid { get; set; }
    public List<ValidationError> Errors { get; set; } = new();
    public List<ValidationWarning> Warnings { get; set; } = new();
    public List<ValidationSuggestion> Suggestions { get; set; } = new();
}

public class ValidationError
{
    public string Field { get; set; } = string.Empty;
    public string Message { get; set; } = string.Empty;
    public string Code { get; set; } = string.Empty;
    public int Severity { get; set; } = 1; // 1=Error, 2=Warning, 3=Info
}

public class ValidationWarning
{
    public string Field { get; set; } = string.Empty;
    public string Message { get; set; } = string.Empty;
    public string Suggestion { get; set; } = string.Empty;
}

public class ValidationSuggestion
{
    public string Field { get; set; } = string.Empty;
    public string CurrentValue { get; set; } = string.Empty;
    public string SuggestedValue { get; set; } = string.Empty;
    public string Reason { get; set; } = string.Empty;
    public double Confidence { get; set; }
}

#endregion

#region Abbreviation Dictionary

/// <summary>
/// Wörterbuch für deutsche Verwaltungsabkürzungen
/// </summary>
public class AbbreviationDictionary
{
    public Dictionary<string, AbbreviationEntry> Entries { get; set; } = new();
    
    /// <summary>
    /// Standard-Abkürzungen für deutsche Verwaltung
    /// </summary>
    public static Dictionary<string, AbbreviationEntry> GermanAdministration => new()
    {
        // Vorgangstypen
        ["STN"] = new AbbreviationEntry
        {
            Abbreviation = "STN",
            FullText = "Stellungnahme",
            Category = "Vorgangstyp",
            Description = "Stellungnahme zu einem Vorgang oder Sachverhalt",
            Icon = "💬",
            Color = "#9C27B0"
        },
        ["GEN"] = new AbbreviationEntry
        {
            Abbreviation = "GEN",
            FullText = "Genehmigung",
            Category = "Vorgangstyp",
            Description = "Genehmigungsverfahren oder -bescheid",
            Icon = "✅",
            Color = "#4CAF50"
        },
        ["ABL"] = new AbbreviationEntry
        {
            Abbreviation = "ABL",
            FullText = "Ablehnung",
            Category = "Vorgangstyp",
            Description = "Ablehnung eines Antrags oder Vorschlags",
            Icon = "❌",
            Color = "#F44336"
        },
        ["GV"] = new AbbreviationEntry
        {
            Abbreviation = "GV",
            FullText = "Genehmigungsverfahren",
            Category = "Vorgangstyp",
            Description = "Laufendes Genehmigungsverfahren",
            Icon = "⚙️",
            Color = "#FF9800"
        },
        ["BE"] = new AbbreviationEntry
        {
            Abbreviation = "BE",
            FullText = "Bescheid",
            Category = "Vorgangstyp",
            Description = "Verwaltungsbescheid oder Entscheidung",
            Icon = "📋",
            Color = "#2196F3"
        },
        ["ANF"] = new AbbreviationEntry
        {
            Abbreviation = "ANF",
            FullText = "Anfrage",
            Category = "Vorgangstyp",
            Description = "Anfrage oder Informationsersuchen",
            Icon = "❓",
            Color = "#00BCD4"
        },
        ["ANT"] = new AbbreviationEntry
        {
            Abbreviation = "ANT",
            FullText = "Antrag",
            Category = "Vorgangstyp",
            Description = "Formeller Antrag",
            Icon = "📝",
            Color = "#3F51B5"
        },
        ["ENTSCH"] = new AbbreviationEntry
        {
            Abbreviation = "ENTSCH",
            FullText = "Entscheidung",
            Category = "Vorgangstyp",
            Description = "Behördliche Entscheidung",
            Icon = "⚖️",
            Color = "#673AB7"
        },
        
        // Weitere gängige Abkürzungen
        ["WV"] = new AbbreviationEntry
        {
            Abbreviation = "WV",
            FullText = "Wiedervorlage",
            Category = "Workflow",
            Description = "Wiedervorlage eines Vorgangs",
            Icon = "🔄",
            Color = "#795548"
        },
        ["ZStA"] = new AbbreviationEntry
        {
            Abbreviation = "ZStA",
            FullText = "Zur Stellungnahme an",
            Category = "Workflow",
            Description = "Weiterleitung zur Stellungnahme",
            Icon = "➡️",
            Color = "#607D8B"
        },
        ["z.K."] = new AbbreviationEntry
        {
            Abbreviation = "z.K.",
            FullText = "zur Kenntnisnahme",
            Category = "Workflow",
            Description = "Nur zur Information, keine Aktion erforderlich",
            Icon = "ℹ️",
            Color = "#9E9E9E"
        },
        ["m.d.B."] = new AbbreviationEntry
        {
            Abbreviation = "m.d.B.",
            FullText = "mit der Bitte",
            Category = "Workflow",
            Description = "Mit der Bitte um Bearbeitung/Stellungnahme",
            Icon = "🙏",
            Color = "#8BC34A"
        }
    };
}

/// <summary>
/// Eintrag im Abkürzungswörterbuch
/// </summary>
public class AbbreviationEntry
{
    public string Abbreviation { get; set; } = string.Empty;
    public string FullText { get; set; } = string.Empty;
    public string Category { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public string Color { get; set; } = "#000000";
    public List<string> Synonyms { get; set; } = new();
    public List<string> RelatedTerms { get; set; } = new();
    public int UsageCount { get; set; }
}

#endregion

#region Metadata Extraction Result

/// <summary>
/// Ergebnis der Metadata-Extraktion aus Text
/// </summary>
public class MetadataExtractionResult
{
    public string OriginalText { get; set; } = string.Empty;
    public List<MetadataBadge> ExtractedBadges { get; set; } = new();
    public Dictionary<string, object> StructuredData { get; set; } = new();
    public double OverallConfidence { get; set; }
    public List<string> UnrecognizedSegments { get; set; } = new();
    public DateTime ExtractedAt { get; set; } = DateTime.UtcNow;
    
    /// <summary>
    /// Beispiel-Analyse: "2025-11-12 T26 -> T11 STN GV078/22 erneute Änderung"
    /// </summary>
    public static MetadataExtractionResult ParseExample()
    {
        return new MetadataExtractionResult
        {
            OriginalText = "2025-11-12 T26 -> T11 STN GV078/22 erneute Änderung",
            ExtractedBadges = new List<MetadataBadge>
            {
                new MetadataBadge
                {
                    DisplayText = "12.11.2025",
                    RawText = "2025-11-12",
                    Type = BadgeType.Date,
                    Style = BadgeStyle.DefaultStyles[BadgeType.Date],
                    Tooltip = "Datum des Schreibens/Vorgangs: 12. November 2025",
                    Confidence = 1.0,
                    StartPosition = 0,
                    EndPosition = 10,
                    SemanticField = "documentDate",
                    StructuredData = new Dictionary<string, object>
                    {
                        ["year"] = 2025,
                        ["month"] = 11,
                        ["day"] = 12,
                        ["isoDate"] = "2025-11-12"
                    }
                },
                new MetadataBadge
                {
                    DisplayText = "T26",
                    RawText = "T26",
                    Type = BadgeType.Department,
                    Style = BadgeStyle.DefaultStyles[BadgeType.Department],
                    Tooltip = "Von Abteilung T26",
                    Confidence = 0.95,
                    StartPosition = 11,
                    EndPosition = 14,
                    SemanticField = "fromDepartment",
                    StructuredData = new Dictionary<string, object>
                    {
                        ["departmentCode"] = "T26",
                        ["direction"] = "from"
                    }
                },
                new MetadataBadge
                {
                    DisplayText = "T11",
                    RawText = "T11",
                    Type = BadgeType.Department,
                    Style = BadgeStyle.DefaultStyles[BadgeType.Department],
                    Tooltip = "An Abteilung T11",
                    Confidence = 0.95,
                    StartPosition = 18,
                    EndPosition = 21,
                    SemanticField = "toDepartment",
                    StructuredData = new Dictionary<string, object>
                    {
                        ["departmentCode"] = "T11",
                        ["direction"] = "to"
                    }
                },
                new MetadataBadge
                {
                    DisplayText = "Stellungnahme",
                    RawText = "STN",
                    Type = BadgeType.ProcessType,
                    Style = BadgeStyle.DefaultStyles[BadgeType.ProcessType],
                    Tooltip = "Vorgangstyp: Stellungnahme",
                    Confidence = 1.0,
                    StartPosition = 22,
                    EndPosition = 25,
                    SemanticField = "processType",
                    StructuredData = new Dictionary<string, object>
                    {
                        ["abbreviation"] = "STN",
                        ["fullText"] = "Stellungnahme",
                        ["category"] = "Vorgangstyp"
                    }
                },
                new MetadataBadge
                {
                    DisplayText = "GV078/22",
                    RawText = "GV078/22",
                    Type = BadgeType.FileReference,
                    Style = BadgeStyle.DefaultStyles[BadgeType.FileReference],
                    Tooltip = "Aktenzeichen: Genehmigungsverfahren 078 aus 2022",
                    Confidence = 1.0,
                    StartPosition = 26,
                    EndPosition = 34,
                    SemanticField = "fileReference",
                    StructuredData = new Dictionary<string, object>
                    {
                        ["prefix"] = "GV",
                        ["number"] = "078",
                        ["year"] = "22",
                        ["fullYear"] = 2022,
                        ["description"] = "Genehmigungsverfahren"
                    },
                    IsInteractive = true,
                    ClickAction = new BadgeAction
                    {
                        Type = BadgeActionType.Navigate,
                        Target = "file",
                        Parameters = new Dictionary<string, object>
                        {
                            ["fileReference"] = "GV078/22"
                        }
                    }
                },
                new MetadataBadge
                {
                    DisplayText = "erneute Änderung",
                    RawText = "erneute Änderung",
                    Type = BadgeType.Topic,
                    Style = BadgeStyle.DefaultStyles[BadgeType.Topic],
                    Tooltip = "Thema/Betreff: Erneute Stellungnahme zu Änderung",
                    Confidence = 0.85,
                    StartPosition = 35,
                    EndPosition = 51,
                    SemanticField = "subject",
                    StructuredData = new Dictionary<string, object>
                    {
                        ["isRevision"] = true,
                        ["keywords"] = new[] { "erneut", "Änderung", "Stellungnahme" }
                    }
                }
            },
            StructuredData = new Dictionary<string, object>
            {
                ["documentDate"] = "2025-11-12",
                ["fromDepartment"] = "T26",
                ["toDepartment"] = "T11",
                ["processType"] = "Stellungnahme",
                ["processTypeAbbr"] = "STN",
                ["fileReference"] = "GV078/22",
                ["subject"] = "Erneute Stellungnahme zu Genehmigungsverfahren 078/2022",
                ["isRevision"] = true,
                ["year"] = 2022,
                ["referenceNumber"] = "078"
            },
            OverallConfidence = 0.95,
            UnrecognizedSegments = new List<string> { "->" }
        };
    }
}

#endregion
