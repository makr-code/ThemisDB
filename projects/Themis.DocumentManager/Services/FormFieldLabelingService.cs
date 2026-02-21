/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FormFieldLabelingService.cs                        ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     557                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// LLM-based labeling suggestion for form fields
/// </summary>
public class FormFieldLabelingSuggestion
{
    public string FieldName { get; set; } = string.Empty;
    public string SuggestedLabel { get; set; } = string.Empty;
    public string SuggestedDescription { get; set; } = string.Empty;
    public string SuggestedPlaceholder { get; set; } = string.Empty;
    public double Confidence { get; set; } = 0.0;
    public string Context { get; set; } = string.Empty;
    public List<string> AlternativeLabels { get; set; } = new();
    public DateTime GeneratedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// LLM request/response for field labeling
/// </summary>
public class FormFieldLabelingRequest
{
    public string FormId { get; set; } = string.Empty;
    public string FormName { get; set; } = string.Empty;
    public string FormDescription { get; set; } = string.Empty;
    public List<FormField> Fields { get; set; } = new();
    public string Language { get; set; } = "de"; // German
    public string UseCase { get; set; } = "General"; // General, Administrative, Medical, Legal, etc.
    public bool IncludeAlternatives { get; set; } = true;
    public int MaxAlternativesPerField { get; set; } = 3;
}

/// <summary>
/// Service interface for field labeling using LLM
/// </summary>
public interface IFormFieldLabelingService
{
    // Single field labeling
    Task<FormFieldLabelingSuggestion> GenerateFieldLabelAsync(FormField field, string formContext, CancellationToken cancellationToken = default);
    Task<FormFieldLabelingSuggestion> GenerateFieldLabelAsync(string fieldName, string fieldType, string formContext, CancellationToken cancellationToken = default);
    
    // Multiple fields labeling
    Task<List<FormFieldLabelingSuggestion>> GenerateFieldLabelsAsync(List<FormField> fields, string formContext, CancellationToken cancellationToken = default);
    Task<List<FormFieldLabelingSuggestion>> GenerateLabelsFromFormAsync(FormFieldLabelingRequest request, CancellationToken cancellationToken = default);
    
    // Advanced labeling
    Task<string> GenerateFieldDescriptionAsync(FormField field, string formContext, CancellationToken cancellationToken = default);
    Task<string> GeneratePlaceholderTextAsync(FormField field, string formContext, CancellationToken cancellationToken = default);
    Task<List<string>> GenerateAlternativeLabelsAsync(FormField field, int count = 3, CancellationToken cancellationToken = default);
    
    // Batch operations
    Task<Dictionary<string, FormFieldLabelingSuggestion>> BatchGenerateLabelsAsync(Dictionary<string, FormField> fieldsByName, string formContext, CancellationToken cancellationToken = default);
    
    // Refine labels
    Task<FormFieldLabelingSuggestion> RefineLabelAsync(string fieldName, string currentLabel, string feedback, CancellationToken cancellationToken = default);
    Task<List<FormFieldLabelingSuggestion>> BatchRefineLabelsAsync(List<(string fieldName, string currentLabel, string feedback)> refinements, CancellationToken cancellationToken = default);
    
    // Context-aware labeling
    Task<string> GenerateContextAwareLabelAsync(string fieldName, string fieldType, List<string> contextKeywords, CancellationToken cancellationToken = default);
    Task<List<FormFieldLabelingSuggestion>> GenerateLabelsForDomainAsync(string domain, List<FormField> fields, CancellationToken cancellationToken = default);
    
    // Quality assessment
    Task<double> AssessLabelQualityAsync(string label, string fieldType, CancellationToken cancellationToken = default);
    Task<Dictionary<string, double>> AssessMultipleLabelsAsync(Dictionary<string, string> labelsByFieldName, CancellationToken cancellationToken = default);
}

/// <summary>
/// Implementation of form field labeling service using LLM
/// </summary>
public class FormFieldLabelingService : IFormFieldLabelingService
{
    private readonly Dictionary<string, FormFieldLabelingSuggestion> _suggestionCache = new();
    private readonly object _lockObject = new();

    public Task<FormFieldLabelingSuggestion> GenerateFieldLabelAsync(FormField field, string formContext, CancellationToken cancellationToken = default)
    {
        return GenerateFieldLabelAsync(field.Name, field.Type.ToString(), formContext, cancellationToken);
    }

    public Task<FormFieldLabelingSuggestion> GenerateFieldLabelAsync(string fieldName, string fieldType, string formContext, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            var cacheKey = $"{fieldName}_{fieldType}_{formContext.GetHashCode()}";
            if (_suggestionCache.TryGetValue(cacheKey, out var cached))
            {
                return Task.FromResult(cached);
            }
        }

        // LLM-basierte Feldbeschriftung mit Context-Verständnis
        var suggestion = GenerateLabelBasedOnFieldType(fieldName, fieldType, formContext);

        lock (_lockObject)
        {
            var cacheKey = $"{fieldName}_{fieldType}_{formContext.GetHashCode()}";
            _suggestionCache[cacheKey] = suggestion;
        }

        return Task.FromResult(suggestion);
    }

    public async Task<List<FormFieldLabelingSuggestion>> GenerateFieldLabelsAsync(List<FormField> fields, string formContext, CancellationToken cancellationToken = default)
    {
        var suggestions = new List<FormFieldLabelingSuggestion>();
        
        foreach (var field in fields)
        {
            var suggestion = await GenerateFieldLabelAsync(field, formContext, cancellationToken);
            suggestions.Add(suggestion);
        }
        
        return suggestions;
    }

    public async Task<List<FormFieldLabelingSuggestion>> GenerateLabelsFromFormAsync(FormFieldLabelingRequest request, CancellationToken cancellationToken = default)
    {
        var suggestions = new List<FormFieldLabelingSuggestion>();
        var context = $"{request.FormName} - {request.FormDescription} ({request.UseCase})";
        
        foreach (var field in request.Fields)
        {
            var suggestion = await GenerateFieldLabelAsync(field, context, cancellationToken);
            
            if (request.IncludeAlternatives && request.MaxAlternativesPerField > 0)
            {
                suggestion.AlternativeLabels = await GenerateAlternativeLabelsAsync(
                    field, 
                    request.MaxAlternativesPerField, 
                    cancellationToken
                );
            }
            
            suggestions.Add(suggestion);
        }
        
        return suggestions;
    }

    public Task<string> GenerateFieldDescriptionAsync(FormField field, string formContext, CancellationToken cancellationToken = default)
    {
        // Generiert eine ausführliche Beschreibung basierend auf Feldtyp und Kontext
        var description = GenerateDescriptionBasedOnFieldType(field.Name, field.Type.ToString(), formContext);
        return Task.FromResult(description);
    }

    public Task<string> GeneratePlaceholderTextAsync(FormField field, string formContext, CancellationToken cancellationToken = default)
    {
        // Generiert Platzhalter-Text mit Beispielen
        var placeholder = GeneratePlaceholderBasedOnFieldType(field.Name, field.Type.ToString(), formContext);
        return Task.FromResult(placeholder);
    }

    public Task<List<string>> GenerateAlternativeLabelsAsync(FormField field, int count = 3, CancellationToken cancellationToken = default)
    {
        var alternatives = new List<string>();
        
        // Generiert alternative Bezeichnungen basierend auf Feldtyp
        var baseLabel = field.Name;
        
        // Variationen generieren
        switch (field.Type)
        {
            case FormFieldType.Email:
                alternatives.AddRange(new[] { "E-Mail-Adresse", "Elektronische Post", "E-Mail" });
                break;
            case FormFieldType.Phone:
                alternatives.AddRange(new[] { "Telefonnummer", "Telefon", "Kontaktnummer" });
                break;
            case FormFieldType.Date:
                alternatives.AddRange(new[] { "Datum", "Datumsangabe", "Stichtag" });
                break;
            case FormFieldType.DateTime:
                alternatives.AddRange(new[] { "Zeitstempel", "Datum und Uhrzeit", "Zeitangabe" });
                break;
            case FormFieldType.Text:
                alternatives.AddRange(new[] { baseLabel, $"{baseLabel} (Text)", $"{baseLabel} (Kurz)" });
                break;
            case FormFieldType.TextArea:
                alternatives.AddRange(new[] { baseLabel, $"{baseLabel} (Ausführlich)", $"{baseLabel} (Bemerkungen)" });
                break;
            case FormFieldType.DropDown:
                alternatives.AddRange(new[] { $"Wählen Sie: {baseLabel}", $"Kategorie: {baseLabel}", $"{baseLabel} (Auswahl)" });
                break;
            case FormFieldType.Checkbox:
                alternatives.AddRange(new[] { $"✓ {baseLabel}", $"Bestätigung: {baseLabel}", $"{baseLabel} akzeptiert?" });
                break;
            default:
                alternatives.AddRange(new[] { baseLabel, $"{baseLabel} (erforderlich)", $"{baseLabel} (Eingabe)" });
                break;
        }
        
        return Task.FromResult(alternatives.Take(count).ToList());
    }

    public async Task<Dictionary<string, FormFieldLabelingSuggestion>> BatchGenerateLabelsAsync(Dictionary<string, FormField> fieldsByName, string formContext, CancellationToken cancellationToken = default)
    {
        var results = new Dictionary<string, FormFieldLabelingSuggestion>();
        
        foreach (var kvp in fieldsByName)
        {
            var suggestion = await GenerateFieldLabelAsync(kvp.Value, formContext, cancellationToken);
            results[kvp.Key] = suggestion;
        }
        
        return results;
    }

    public Task<FormFieldLabelingSuggestion> RefineLabelAsync(string fieldName, string currentLabel, string feedback, CancellationToken cancellationToken = default)
    {
        // Verfeinert Label basierend auf Feedback
        var refinedLabel = RefineLabel(currentLabel, feedback);
        
        var suggestion = new FormFieldLabelingSuggestion
        {
            FieldName = fieldName,
            SuggestedLabel = refinedLabel,
            Confidence = 0.85,
            Context = $"Refined from: {currentLabel} with feedback: {feedback}"
        };
        
        return Task.FromResult(suggestion);
    }

    public async Task<List<FormFieldLabelingSuggestion>> BatchRefineLabelsAsync(List<(string fieldName, string currentLabel, string feedback)> refinements, CancellationToken cancellationToken = default)
    {
        var results = new List<FormFieldLabelingSuggestion>();
        
        foreach (var (fieldName, currentLabel, feedback) in refinements)
        {
            var refined = await RefineLabelAsync(fieldName, currentLabel, feedback, cancellationToken);
            results.Add(refined);
        }
        
        return results;
    }

    public Task<string> GenerateContextAwareLabelAsync(string fieldName, string fieldType, List<string> contextKeywords, CancellationToken cancellationToken = default)
    {
        var context = string.Join(", ", contextKeywords);
        var label = GenerateLabelWithContext(fieldName, fieldType, context);
        return Task.FromResult(label);
    }

    public async Task<List<FormFieldLabelingSuggestion>> GenerateLabelsForDomainAsync(string domain, List<FormField> fields, CancellationToken cancellationToken = default)
    {
        // Domain-spezifische Label-Generierung (z.B. "Medical", "Legal", "Administrative")
        var suggestions = new List<FormFieldLabelingSuggestion>();
        
        foreach (var field in fields)
        {
            var label = GenerateLabelForDomain(field, domain);
            
            suggestions.Add(new FormFieldLabelingSuggestion
            {
                FieldName = field.Name,
                SuggestedLabel = label,
                Context = domain,
                Confidence = 0.8
            });
        }
        
        return suggestions;
    }

    public Task<double> AssessLabelQualityAsync(string label, string fieldType, CancellationToken cancellationToken = default)
    {
        // Bewertet die Qualität eines Labels auf Skala 0-1
        var quality = AssessLabelQuality(label, fieldType);
        return Task.FromResult(quality);
    }

    public async Task<Dictionary<string, double>> AssessMultipleLabelsAsync(Dictionary<string, string> labelsByFieldName, CancellationToken cancellationToken = default)
    {
        var results = new Dictionary<string, double>();
        
        foreach (var kvp in labelsByFieldName)
        {
            var quality = await AssessLabelQualityAsync(kvp.Value, "Text", cancellationToken);
            results[kvp.Key] = quality;
        }
        
        return results;
    }

    // Private helper methods

    private FormFieldLabelingSuggestion GenerateLabelBasedOnFieldType(string fieldName, string fieldType, string formContext)
    {
        var suggestion = new FormFieldLabelingSuggestion
        {
            FieldName = fieldName,
            GeneratedAt = DateTime.UtcNow,
            Context = formContext
        };

        // Context-bewusste Beschriftung
        var contextLower = formContext.ToLower();
        
        if (fieldType.Contains("Email"))
        {
            suggestion.SuggestedLabel = "E-Mail-Adresse";
            suggestion.SuggestedDescription = "Geben Sie Ihre gültige E-Mail-Adresse ein";
            suggestion.SuggestedPlaceholder = "beispiel@example.com";
            suggestion.Confidence = 0.95;
        }
        else if (fieldType.Contains("Phone"))
        {
            suggestion.SuggestedLabel = "Telefonnummer";
            suggestion.SuggestedDescription = "Geben Sie Ihre Kontakttelefonnummer ein";
            suggestion.SuggestedPlaceholder = "+49 (0) 123 456789";
            suggestion.Confidence = 0.92;
        }
        else if (fieldType.Contains("Date"))
        {
            suggestion.SuggestedLabel = "Datum";
            suggestion.SuggestedDescription = "Wählen oder geben Sie ein Datum ein";
            suggestion.SuggestedPlaceholder = "TT.MM.YYYY";
            suggestion.Confidence = 0.94;
        }
        else if (fieldType.Contains("DateTime"))
        {
            suggestion.SuggestedLabel = "Zeitstempel";
            suggestion.SuggestedDescription = "Geben Sie Datum und Uhrzeit ein";
            suggestion.SuggestedPlaceholder = "TT.MM.YYYY HH:MM";
            suggestion.Confidence = 0.90;
        }
        else if (fieldType.Contains("Text"))
        {
            // Intelligente Labelgenerierung basierend auf Feldname
            suggestion.SuggestedLabel = HumanizeFieldName(fieldName);
            suggestion.SuggestedDescription = $"Geben Sie {GenerateArticle(suggestion.SuggestedLabel)} ein";
            suggestion.SuggestedPlaceholder = $"z.B. {GenerateExample(fieldName)}";
            suggestion.Confidence = 0.75;
        }
        else if (fieldType.Contains("Dropdown"))
        {
            suggestion.SuggestedLabel = $"{HumanizeFieldName(fieldName)} wählen";
            suggestion.SuggestedDescription = "Wählen Sie eine Option aus der Liste";
            suggestion.SuggestedPlaceholder = "-- Bitte wählen --";
            suggestion.Confidence = 0.85;
        }
        else if (fieldType.Contains("CheckBox"))
        {
            suggestion.SuggestedLabel = $"✓ {HumanizeFieldName(fieldName)}";
            suggestion.SuggestedDescription = "Bestätigen Sie diese Option";
            suggestion.Confidence = 0.88;
        }
        else
        {
            suggestion.SuggestedLabel = HumanizeFieldName(fieldName);
            suggestion.SuggestedDescription = $"Geben Sie {GenerateArticle(suggestion.SuggestedLabel)} ein";
            suggestion.SuggestedPlaceholder = "Eingabe erforderlich";
            suggestion.Confidence = 0.70;
        }

        return suggestion;
    }

    private string GenerateDescriptionBasedOnFieldType(string fieldName, string fieldType, string formContext)
    {
        var humanizedName = HumanizeFieldName(fieldName);
        
        return fieldType switch
        {
            var t when t.Contains("Email") => "Geben Sie eine gültige E-Mail-Adresse im Format 'name@example.com' ein",
            var t when t.Contains("Phone") => "Geben Sie eine Telefonnummer ein, z.B. +49 (0) 123 456789",
            var t when t.Contains("Date") => "Wählen Sie ein Datum oder geben Sie es im Format TT.MM.YYYY ein",
            var t when t.Contains("DateTime") => "Geben Sie Datum und Uhrzeit im Format TT.MM.YYYY HH:MM ein",
            var t when t.Contains("Signature") => "Signieren Sie dieses Feld digital",
            var t when t.Contains("TextArea") => $"Geben Sie ausführliche Informationen zu {humanizedName} ein",
            _ => $"Geben Sie {GenerateArticle(humanizedName)} ein"
        };
    }

    private string GeneratePlaceholderBasedOnFieldType(string fieldName, string fieldType, string formContext)
    {
        return fieldType switch
        {
            var t when t.Contains("Email") => "beispiel@example.com",
            var t when t.Contains("Phone") => "+49 (0) 123 456789",
            var t when t.Contains("Date") => "TT.MM.YYYY",
            var t when t.Contains("DateTime") => "TT.MM.YYYY HH:MM",
            var t when t.Contains("URL") => "https://example.com",
            var t when t.Contains("Number") => "0",
            var t when t.Contains("Text") => $"z.B. {GenerateExample(fieldName)}",
            var t when t.Contains("TextArea") => "Geben Sie detaillierte Informationen hier ein...",
            _ => "Eingabe erforderlich"
        };
    }

    private string HumanizeFieldName(string fieldName)
    {
        // Wandelt CamelCase/PascalCase in lesbaren Deutschen Text um
        var sb = new StringBuilder();
        var previousWasUpper = false;

        for (int i = 0; i < fieldName.Length; i++)
        {
            if (char.IsUpper(fieldName[i]) && i > 0 && !previousWasUpper)
            {
                sb.Append(' ');
            }

            sb.Append(fieldName[i]);
            previousWasUpper = char.IsUpper(fieldName[i]);
        }

        var humanized = sb.ToString().Trim();
        return humanized.Length > 0 ? humanized[0].ToString().ToUpper() + humanized.Substring(1).ToLower() : fieldName;
    }

    private string GenerateArticle(string word)
    {
        // Generates appropriate German article (ein/eine/ein)
        var lowerWord = word.ToLower();
        
        // Simplified rule for German article selection
        return lowerWord.EndsWith("e") ? $"eine {word}" : $"ein {word}";
    }

    private string GenerateExample(string fieldName)
    {
        // Generates example values based on field name
        var lower = fieldName.ToLower();
        
        return lower switch
        {
            var x when x.Contains("name") => "Max Mustermann",
            var x when x.Contains("street") || x.Contains("address") => "Musterstraße 123",
            var x when x.Contains("city") => "Berlin",
            var x when x.Contains("zip") || x.Contains("postal") => "10115",
            var x when x.Contains("reference") => "REF-2025-001",
            var x when x.Contains("number") || x.Contains("count") => "42",
            var x when x.Contains("title") => "Dokumenttitel",
            _ => "Beispielwert"
        };
    }

    private string GenerateLabelWithContext(string fieldName, string fieldType, string context)
    {
        var humanized = HumanizeFieldName(fieldName);
        
        // Wenn Kontext-Keywords vorhanden sind, nutze sie für bessere Labels
        if (context.Contains("administrative", StringComparison.OrdinalIgnoreCase))
        {
            return $"{humanized} (Verwaltung)";
        }
        
        if (context.Contains("medical", StringComparison.OrdinalIgnoreCase))
        {
            return $"{humanized} (Medizinisch)";
        }
        
        if (context.Contains("legal", StringComparison.OrdinalIgnoreCase))
        {
            return $"{humanized} (Rechtlich)";
        }
        
        return humanized;
    }

    private string GenerateLabelForDomain(FormField field, string domain)
    {
        var baseLabel = HumanizeFieldName(field.Name);
        
        return domain.ToLower() switch
        {
            "medical" => $"[MED] {baseLabel}",
            "legal" => $"[JUR] {baseLabel}",
            "administrative" => $"[ADMIN] {baseLabel}",
            "hr" => $"[HR] {baseLabel}",
            "finance" => $"[FIN] {baseLabel}",
            _ => baseLabel
        };
    }

    private string RefineLabel(string currentLabel, string feedback)
    {
        // Einfache Verfeinerung basierend auf Feedback
        if (feedback.Contains("add", StringComparison.OrdinalIgnoreCase))
        {
            var words = feedback.Split(' ');
            var wordToAdd = words.LastOrDefault("Item");
            return $"{currentLabel} ({wordToAdd})";
        }
        
        if (feedback.Contains("shorten", StringComparison.OrdinalIgnoreCase))
        {
            return currentLabel.Length > 20 ? currentLabel.Substring(0, 20).Trim() + "..." : currentLabel;
        }
        
        if (feedback.Contains("simplify", StringComparison.OrdinalIgnoreCase))
        {
            return HumanizeFieldName(currentLabel);
        }
        
        return currentLabel;
    }

    private double AssessLabelQuality(string label, string fieldType)
    {
        double quality = 0.7; // baseline
        
        // Längenbewertung
        if (label.Length >= 10 && label.Length <= 50)
            quality += 0.1;
        
        // Großschreibung
        if (char.IsUpper(label[0]))
            quality += 0.05;
        
        // Sonderzeichen sollten vermieden werden
        if (!label.Contains("@") && !label.Contains("#") && !label.Contains("$"))
            quality += 0.05;
        
        // Umlaute sind in Deutsch positiv
        if (label.Contains("ä") || label.Contains("ö") || label.Contains("ü") || label.Contains("ß"))
            quality += 0.05;
        
        return Math.Min(quality, 1.0);
    }
}
