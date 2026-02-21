/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SmartFormService.cs                                ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:40:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     550                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f3d84df5c  2025-12-09  Add release docs, benchmarks, and Wikipedia stress test s... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Intelligentes Form-Feld mit automatischer Metadaten-Erkennung
/// </summary>
public class SmartFormField
{
    /// <summary>
    /// Original FormField
    /// </summary>
    public FormField BaseField { get; set; } = null!;
    
    /// <summary>
    /// Erkannte Metadata-Badges im Wert
    /// </summary>
    public List<MetadataBadge> DetectedBadges { get; set; } = new();
    
    /// <summary>
    /// Intelligente Vorschläge für dieses Feld
    /// </summary>
    public List<InputSuggestion> SmartSuggestions { get; set; } = new();
    
    /// <summary>
    /// Automatisch extrahierte Werte basierend auf Badges
    /// </summary>
    public Dictionary<string, object> ExtractedMetadata { get; set; } = new();
    
    /// <summary>
    /// Konfidenz-Score für die Erkennung (0-1)
    /// </summary>
    public double ConfidenceScore { get; set; }
    
    /// <summary>
    /// Smart-Input Konfiguration für dieses Feld
    /// </summary>
    public SmartInputConfig Config { get; set; } = new();
}

/// <summary>
/// Service für intelligente Formular-Autovervollständigung und -Erkennung
/// </summary>
public interface ISmartFormService
{
    /// <summary>
    /// Erstellt ein intelligentes Formular mit Badge-Unterstützung
    /// </summary>
    Task<List<SmartFormField>> CreateSmartFormAsync(
        FormTemplate template,
        SmartInputConfig? globalConfig = null,
        CancellationToken cancellationToken = default);
    
    /// <summary>
    /// Aktualisiert Formularfeld mit intelligenter Erkennung
    /// </summary>
    Task<SmartFormField> UpdateSmartFieldAsync(
        SmartFormField field,
        string userInput,
        CancellationToken cancellationToken = default);
    
    /// <summary>
    /// Generiert intelligente Vorschläge für ein Feld
    /// </summary>
    Task<List<InputSuggestion>> GetFieldSuggestionsAsync(
        SmartFormField field,
        string currentInput,
        CancellationToken cancellationToken = default);
    
    /// <summary>
    /// Pre-fills Formular basierend auf erkannten Badges
    /// </summary>
    Task<Dictionary<string, object>> AutoFillFormAsync(
        FormTemplate template,
        string contextText,
        CancellationToken cancellationToken = default);
    
    /// <summary>
    /// Validiert und optimiert Formularfeld
    /// </summary>
    Task<SmartFormField> OptimizeFieldAsync(
        SmartFormField field,
        CancellationToken cancellationToken = default);
    
    /// <summary>
    /// Extrahiert strukturierte Daten aus natürlichem Text
    /// </summary>
    Task<FormSubmissionData> ExtractFormDataFromTextAsync(
        FormTemplate template,
        string naturalText,
        CancellationToken cancellationToken = default);
}

/// <summary>
/// In-Memory Implementierung des Smart Form Service
/// </summary>
public class SmartFormService : ISmartFormService
{
    private readonly IFormTemplateService _templateService;
    private readonly IMetadataBadgeService _badgeService;
    private readonly ISmartSuggestionService _suggestionService;
    private readonly IFormAuditService _auditService;
    private readonly IBadgePatternService _patternService;

    public SmartFormService(
        IFormTemplateService templateService,
        IMetadataBadgeService badgeService,
        ISmartSuggestionService suggestionService,
        IFormAuditService auditService,
        IBadgePatternService patternService)
    {
        _templateService = templateService;
        _badgeService = badgeService;
        _suggestionService = suggestionService;
        _auditService = auditService;
        _patternService = patternService;
    }

    public async Task<List<SmartFormField>> CreateSmartFormAsync(
        FormTemplate template,
        SmartInputConfig? globalConfig = null,
        CancellationToken cancellationToken = default)
    {
        globalConfig ??= new SmartInputConfig();
        var smartFields = new List<SmartFormField>();

        foreach (var section in template.Sections)
        {
            foreach (var field in section.Fields)
            {
                var smartField = new SmartFormField
                {
                    BaseField = field,
                    Config = new SmartInputConfig
                    {
                        EnableAutoBadging = globalConfig.EnableAutoBadging,
                        EnableSemanticSuggestions = globalConfig.EnableSemanticSuggestions,
                        EnableAutocomplete = globalConfig.EnableAutocomplete,
                        EnableRealtimeValidation = globalConfig.EnableRealtimeValidation,
                        EnableFuzzyMatching = globalConfig.EnableFuzzyMatching,
                        MinimumConfidence = globalConfig.MinimumConfidence,
                        MaxSuggestions = globalConfig.MaxSuggestions,
                        AllowedBadgeTypes = DetermineBadgeTypesForField(field),
                        CustomPatterns = globalConfig.CustomPatterns
                    },
                    ConfidenceScore = 1.0
                };

                smartFields.Add(smartField);
            }
        }

        return await Task.FromResult(smartFields);
    }

    public async Task<SmartFormField> UpdateSmartFieldAsync(
        SmartFormField field,
        string userInput,
        CancellationToken cancellationToken = default)
    {
        // 1. Erkenne Badges im Input
        var extractionResult = await _badgeService.ExtractBadgesAsync(
            userInput,
            field.Config,
            cancellationToken);

        field.DetectedBadges = extractionResult.ExtractedBadges;

        // 2. Extrahiere Metadaten aus Badges
        foreach (var badge in field.DetectedBadges)
        {
            field.ExtractedMetadata[badge.Type.ToString()] = badge.DisplayText;
        }

        // 3. Hole intelligente Vorschläge
        field.SmartSuggestions = await GetFieldSuggestionsAsync(
            field,
            userInput,
            cancellationToken);

        // 4. Berechne Konfidenz-Score
        if (field.DetectedBadges.Any())
        {
            field.ConfidenceScore = field.DetectedBadges.Average(b => b.Confidence);
        }

        return field;
    }

    public async Task<List<InputSuggestion>> GetFieldSuggestionsAsync(
        SmartFormField field,
        string currentInput,
        CancellationToken cancellationToken = default)
    {
        var suggestions = new List<InputSuggestion>();

        if (string.IsNullOrEmpty(currentInput))
            return suggestions;

        try
        {
            // 1. Autocomplete-Vorschläge
            if (field.Config.EnableAutocomplete)
            {
                var autocompleteSuggestions = await _suggestionService.GetAutocompleteAsync(
                    currentInput,
                    field.BaseField.Name,
                    field.Config.MaxSuggestions,
                    cancellationToken);

                suggestions.AddRange(autocompleteSuggestions);
            }

            // 2. Semantische Vorschläge
            if (field.Config.EnableSemanticSuggestions)
            {
                var semanticSuggestions = await _suggestionService.GetSemanticSuggestionsAsync(
                    currentInput,
                    field.Config.MaxSuggestions,
                    cancellationToken);

                suggestions.AddRange(semanticSuggestions);
            }

            // 3. Generische Vorschläge basierend auf Feldtyp
            var typeSuggestions = GetFieldTypeSuggestions(field, currentInput);
            suggestions.AddRange(typeSuggestions);

            // 4. Sortiere nach Relevanz
            return suggestions
                .DistinctBy(s => s.Text)
                .OrderByDescending(s => s.SimilarityScore)
                .Take(field.Config.MaxSuggestions)
                .ToList();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error getting field suggestions: {ex.Message}");
            return new List<InputSuggestion>();
        }
    }

    public async Task<Dictionary<string, object>> AutoFillFormAsync(
        FormTemplate template,
        string contextText,
        CancellationToken cancellationToken = default)
    {
        var filledData = new Dictionary<string, object>();

        // Extrahiere Badges aus Kontext
        var extractionResult = await _badgeService.ExtractBadgesAsync(
            contextText,
            null,
            cancellationToken);

        // Mappiere Badges zu Formularfeldern
        foreach (var section in template.Sections)
        {
            foreach (var field in section.Fields)
            {
                var matchingBadges = extractionResult.ExtractedBadges
                    .Where(b => MatchesBadgeToField(field, b))
                    .ToList();

                if (matchingBadges.Any())
                {
                    // Verwende den höchsten Konfidenz Badge
                    var bestMatch = matchingBadges.OrderByDescending(b => b.Confidence).First();
                    filledData[field.Id] = bestMatch.DisplayText;
                }
            }
        }

        return filledData;
    }

    public async Task<SmartFormField> OptimizeFieldAsync(
        SmartFormField field,
        CancellationToken cancellationToken = default)
    {
        // Bestimme optimale Badge-Typen basierend auf Feldtyp
        field.Config.AllowedBadgeTypes = DetermineBadgeTypesForField(field.BaseField);

        // Setze optimale Konfidenz-Schwelle
        field.Config.MinimumConfidence = DetermineOptimalConfidenceThreshold(field.BaseField);

        // Aktiviere nur relevante Features
        field.Config.EnableAutoBadging = field.BaseField.IsRequired;
        field.Config.EnableSemanticSuggestions = field.BaseField.Type == FormFieldType.TextArea;
        field.Config.EnableAutocomplete = field.BaseField.Type == FormFieldType.ComboBox || 
                                         field.BaseField.Type == FormFieldType.DropDown;

        return await Task.FromResult(field);
    }

    public async Task<FormSubmissionData> ExtractFormDataFromTextAsync(
        FormTemplate template,
        string naturalText,
        CancellationToken cancellationToken = default)
    {
        var submission = new FormSubmissionData
        {
            FormId = template.Id,
            FieldValues = new Dictionary<string, object>(),
            Status = "Extracted",
            SubmittedBy = Environment.UserName
        };

        // 1. Extrahiere alle Badges aus dem Text
        var extractionResult = await _badgeService.ExtractBadgesAsync(
            naturalText,
            null,
            cancellationToken);

        // 2. Für jedes Feld im Template, versuche passende Badges zu finden
        foreach (var section in template.Sections)
        {
            foreach (var field in section.Fields)
            {
                var matchingBadges = extractionResult.ExtractedBadges
                    .Where(b => MatchesBadgeToField(field, b))
                    .OrderByDescending(b => b.Confidence)
                    .ToList();

                if (matchingBadges.Any())
                {
                    submission.FieldValues[field.Id] = matchingBadges.First().DisplayText;
                }
                else if (field.DefaultValue != null)
                {
                    submission.FieldValues[field.Id] = field.DefaultValue;
                }
            }
        }

        // 3. Logge die Extraktion
        await _auditService.LogSubmissionAsync(
            submission,
            Environment.UserName,
            "EXTRACT",
            "Success",
            null,
            cancellationToken);

        return submission;
    }

    /// <summary>
    /// Bestimmt passende Badge-Typen für ein Formularfeld
    /// </summary>
    private List<BadgeType> DetermineBadgeTypesForField(FormField field)
    {
        return field.Type switch
        {
            FormFieldType.Date => new List<BadgeType> { BadgeType.Date },
            FormFieldType.DateTime => new List<BadgeType> { BadgeType.Date, BadgeType.Deadline },
            FormFieldType.Text => new List<BadgeType> 
            { 
                BadgeType.Date, 
                BadgeType.Department, 
                BadgeType.FileReference, 
                BadgeType.Person,
                BadgeType.Organization
            },
            FormFieldType.TextArea => new List<BadgeType>
            {
                BadgeType.Date,
                BadgeType.Department,
                BadgeType.FileReference,
                BadgeType.Person,
                BadgeType.Organization,
                BadgeType.Topic,
                BadgeType.Priority
            },
            _ => new List<BadgeType>()
        };
    }

    /// <summary>
    /// Bestimmt optimale Konfidenz-Schwelle für Feldtyp
    /// </summary>
    private double DetermineOptimalConfidenceThreshold(FormField field)
    {
        return field.Type switch
        {
            FormFieldType.Date => 0.95,      // Hohe Anforderung für Daten
            FormFieldType.Email => 0.9,      // Email sehr wichtig
            FormFieldType.Phone => 0.85,     // Telefon wichtig
            FormFieldType.Text => 0.7,       // Text mittel
            FormFieldType.TextArea => 0.6,   // Textarea flexibler
            _ => 0.7
        };
    }

    /// <summary>
    /// Prüft, ob ein Badge zu einem Feld passt
    /// </summary>
    private bool MatchesBadgeToField(FormField field, MetadataBadge badge)
    {
        var allowedTypes = DetermineBadgeTypesForField(field);
        return allowedTypes.Contains(badge.Type);
    }

    /// <summary>
    /// Generiert Vorschläge basierend auf Feldtyp
    /// </summary>
    private List<InputSuggestion> GetFieldTypeSuggestions(SmartFormField field, string currentInput)
    {
        var suggestions = new List<InputSuggestion>();

        if (field.BaseField.Options.Any())
        {
            // Für Dropdown/ComboBox: zeige Optionen als Vorschläge
            var matchingOptions = field.BaseField.Options
                .Where(o => o.Label.Contains(currentInput, StringComparison.OrdinalIgnoreCase))
                .Select(o => new InputSuggestion
                {
                    Text = o.Value,
                    DisplayText = o.Label,
                    Description = o.Description ?? string.Empty,
                    Type = SuggestionType.Exact,
                    Source = SuggestionSource.SystemTemplate,
                    SimilarityScore = 1.0
                });

            suggestions.AddRange(matchingOptions);
        }

        return suggestions;
    }
}

/// <summary>
/// Service für Formular-Vorschläge basierend auf Kontext und Benutzerverhalten
/// </summary>
public interface IFormContextService
{
    /// <summary>
    /// Extrahiert Kontext aus Dokument
    /// </summary>
    Task<Dictionary<string, string>> ExtractDocumentContextAsync(
        string documentContent,
        CancellationToken cancellationToken = default);
    
    /// <summary>
    /// Recommendet Formular-Template basierend auf Kontext
    /// </summary>
    Task<string?> RecommendFormTemplateAsync(
        Dictionary<string, string> context,
        CancellationToken cancellationToken = default);
    
    /// <summary>
    /// Generiert vorgefüllte Formularwerte aus Kontext
    /// </summary>
    Task<Dictionary<string, object>> GenerateContextBasedValuesAsync(
        FormTemplate template,
        Dictionary<string, string> context,
        CancellationToken cancellationToken = default);
}

/// <summary>
/// In-Memory Implementierung des Form Context Service
/// </summary>
public class FormContextService : IFormContextService
{
    private readonly IMetadataBadgeService _badgeService;
    private readonly ISmartFormService _smartFormService;

    public FormContextService(
        IMetadataBadgeService badgeService,
        ISmartFormService smartFormService)
    {
        _badgeService = badgeService;
        _smartFormService = smartFormService;
    }

    public async Task<Dictionary<string, string>> ExtractDocumentContextAsync(
        string documentContent,
        CancellationToken cancellationToken = default)
    {
        var context = new Dictionary<string, string>();

        // Extrahiere Badges und nutze sie als Kontext
        var extractionResult = await _badgeService.ExtractBadgesAsync(
            documentContent,
            null,
            cancellationToken);

        foreach (var badge in extractionResult.ExtractedBadges)
        {
            var key = badge.Type.ToString();
            if (!context.ContainsKey(key))
                context[key] = badge.DisplayText;
        }

        return context;
    }

    public Task<string?> RecommendFormTemplateAsync(
        Dictionary<string, string> context,
        CancellationToken cancellationToken = default)
    {
        // Empfehle Template basierend auf Kontext
        // Hier würde eine LLM oder regelbasierte Logik verwendet werden
        return Task.FromResult<string?>(null);
    }

    public async Task<Dictionary<string, object>> GenerateContextBasedValuesAsync(
        FormTemplate template,
        Dictionary<string, string> context,
        CancellationToken cancellationToken = default)
    {
        var contextText = string.Join(" ", context.Values);
        var filledData = await _smartFormService.AutoFillFormAsync(
            template,
            contextText,
            cancellationToken);

        return filledData;
    }
}
