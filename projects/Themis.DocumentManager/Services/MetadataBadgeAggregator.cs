/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataBadgeAggregator.cs                         ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     355                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Features.MetadataForm.Services;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Badge-Aggregator - Erstellt Badges aus Metadaten
/// Konvertiert MetadataFields in MetadataBadges für kompakte Anzeige
/// </summary>
public class MetadataBadgeAggregator
{
    private readonly SmartMetadataLayoutEngine _layoutEngine;
    private readonly Dictionary<BadgeCategory, BadgeType> _categoryToBadgeTypeMap;

    public MetadataBadgeAggregator()
    {
        _layoutEngine = new SmartMetadataLayoutEngine();
        _categoryToBadgeTypeMap = CreateCategoryBadgeTypeMapping();
    }

    /// <summary>
    /// Erstellt Badges aus DocumentMetadataBinding
    /// </summary>
    public List<MetadataBadge> CreateBadgesFromMetadata(
        DocumentMetadataBinding metadata, 
        BadgeDisplayMode mode = BadgeDisplayMode.FilledOnly)
    {
        var badges = new List<MetadataBadge>();

        var fieldsToProcess = mode switch
        {
            BadgeDisplayMode.FilledOnly => metadata.BoundFields.Where(f => !string.IsNullOrEmpty(f.CurrentValue)),
            BadgeDisplayMode.Required => metadata.BoundFields.Where(f => f.IsRequired),
            BadgeDisplayMode.Priority => metadata.BoundFields.Where(f => f.IsRequired || !string.IsNullOrEmpty(f.CurrentValue)),
            _ => metadata.BoundFields
        };

        foreach (var field in fieldsToProcess)
        {
            var badge = CreateBadgeFromField(field);
            if (badge != null)
                badges.Add(badge);
        }

        return badges;
    }

    /// <summary>
    /// Erstellt Badge aus einzelnem MetadataField
    /// </summary>
    public MetadataBadge? CreateBadgeFromField(MetadataField field)
    {
        if (string.IsNullOrEmpty(field.CurrentValue))
            return null;

        var group = _layoutEngine.DetectFieldGroup(field);
        var badgeType = _categoryToBadgeTypeMap.GetValueOrDefault(group.ColorCategory, BadgeType.Custom);
        var style = BadgeStyle.DefaultStyles.GetValueOrDefault(badgeType) ?? new BadgeStyle();

        return new MetadataBadge
        {
            DisplayText = FormatDisplayText(field),
            RawText = field.CurrentValue,
            Type = badgeType,
            Style = style,
            Tooltip = $"{field.FieldName}: {field.CurrentValue}\nThemisDB: {field.ThemisPath}",
            Confidence = 1.0 // Manual metadata = 100% confidence
        };
    }

    /// <summary>
    /// Gruppiert Badges nach Kategorie
    /// </summary>
    public Dictionary<BadgeCategory, List<MetadataBadge>> GroupBadges(List<MetadataBadge> badges)
    {
        var grouped = new Dictionary<BadgeCategory, List<MetadataBadge>>();

        foreach (var badge in badges)
        {
            var category = BadgeTypeToCategoryMap(badge.Type);
            
            if (!grouped.ContainsKey(category))
                grouped[category] = new List<MetadataBadge>();

            grouped[category].Add(badge);
        }

        return grouped;
    }

    /// <summary>
    /// Erstellt Badge-Summary (Top N Badges)
    /// </summary>
    public List<MetadataBadge> CreateBadgeSummary(
        DocumentMetadataBinding metadata, 
        int maxBadges = 10,
        SummaryPriority priority = SummaryPriority.RequiredFirst)
    {
        var allBadges = CreateBadgesFromMetadata(metadata, BadgeDisplayMode.Priority);

        var sortedBadges = priority switch
        {
            SummaryPriority.RequiredFirst => SortByRequiredFirst(allBadges, metadata),
            SummaryPriority.AlphabeticalByCategory => SortByCategoryAlphabetical(allBadges),
            SummaryPriority.MostRecent => SortByMostRecent(allBadges, metadata),
            _ => allBadges
        };

        return sortedBadges.Take(maxBadges).ToList();
    }

    /// <summary>
    /// Erstellt kategoriebasierte Badge-Zusammenfassung
    /// </summary>
    public List<CategoryBadgeSummary> CreateCategorySummary(DocumentMetadataBinding metadata)
    {
        var badges = CreateBadgesFromMetadata(metadata, BadgeDisplayMode.FilledOnly);
        var grouped = GroupBadges(badges);

        return grouped.Select(kvp => new CategoryBadgeSummary
        {
            Category = kvp.Key,
            BadgeCount = kvp.Value.Count,
            TopBadges = kvp.Value.Take(3).ToList(),
            Icon = GetCategoryIcon(kvp.Key),
            ColorHex = GetCategoryColor(kvp.Key)
        })
        .OrderByDescending(cs => cs.BadgeCount)
        .ToList();
    }

    /// <summary>
    /// Formatiert Display-Text für Badge
    /// </summary>
    private string FormatDisplayText(MetadataField field)
    {
        var value = field.CurrentValue ?? "";

        // Datum-Felder formatieren
        if (field.Type == FieldType.Date || field.Type == FieldType.DateTime)
        {
            if (DateTime.TryParse(value, out var date))
                return date.ToString("dd.MM.yyyy");
        }

        // Lange Texte kürzen
        if (value.Length > 30)
            return value.Substring(0, 27) + "...";

        return value;
    }

    /// <summary>
    /// Sortiert Badges nach Required-Status
    /// </summary>
    private List<MetadataBadge> SortByRequiredFirst(List<MetadataBadge> badges, DocumentMetadataBinding metadata)
    {
        var requiredFieldNames = metadata.BoundFields
            .Where(f => f.IsRequired)
            .Select(f => f.FieldName)
            .ToHashSet();

        return badges
            .OrderByDescending(b => requiredFieldNames.Contains(ExtractFieldName(b)))
            .ThenBy(b => b.DisplayText)
            .ToList();
    }

    /// <summary>
    /// Sortiert Badges alphabetisch nach Kategorie
    /// </summary>
    private List<MetadataBadge> SortByCategoryAlphabetical(List<MetadataBadge> badges)
    {
        return badges
            .OrderBy(b => BadgeTypeToCategoryMap(b.Type).ToString())
            .ThenBy(b => b.DisplayText)
            .ToList();
    }

    /// <summary>
    /// Sortiert Badges nach Aktualität
    /// </summary>
    private List<MetadataBadge> SortByMostRecent(List<MetadataBadge> badges, DocumentMetadataBinding metadata)
    {
        var fieldUpdateTimes = metadata.BoundFields
            .Where(f => f.LastUpdated.HasValue)
            .ToDictionary(f => f.FieldName, f => f.LastUpdated!.Value);

        return badges
            .OrderByDescending(b =>
            {
                var fieldName = ExtractFieldName(b);
                return fieldUpdateTimes.GetValueOrDefault(fieldName, DateTime.MinValue);
            })
            .ToList();
    }

    /// <summary>
    /// Extrahiert Feldname aus Badge (aus Tooltip)
    /// </summary>
    private string ExtractFieldName(MetadataBadge badge)
    {
        if (string.IsNullOrEmpty(badge.Tooltip))
            return "";

        var parts = badge.Tooltip.Split(':');
        return parts.Length > 0 ? parts[0].Trim() : "";
    }

    /// <summary>
    /// Mapping BadgeType → BadgeCategory
    /// </summary>
    private BadgeCategory BadgeTypeToCategoryMap(BadgeType type)
    {
        return type switch
        {
            BadgeType.Date => BadgeCategory.Zeit,
            BadgeType.Department => BadgeCategory.Organisation,
            BadgeType.Organization => BadgeCategory.Organisation,
            BadgeType.ProcessType => BadgeCategory.Vorgang,
            BadgeType.FileReference => BadgeCategory.Vorgang,
            BadgeType.Status => BadgeCategory.Status,
            BadgeType.Priority => BadgeCategory.Priorität,
            BadgeType.Person => BadgeCategory.Personen,
            BadgeType.Location => BadgeCategory.Räumlich,
            BadgeType.Topic => BadgeCategory.Thematik,
            BadgeType.Action => BadgeCategory.Aktionen,
            BadgeType.Deadline => BadgeCategory.Priorität,
            _ => BadgeCategory.Technisch
        };
    }

    /// <summary>
    /// Mapping BadgeCategory → BadgeType
    /// </summary>
    private Dictionary<BadgeCategory, BadgeType> CreateCategoryBadgeTypeMapping()
    {
        return new Dictionary<BadgeCategory, BadgeType>
        {
            { BadgeCategory.Zeit, BadgeType.Date },
            { BadgeCategory.Organisation, BadgeType.Department },
            { BadgeCategory.Vorgang, BadgeType.ProcessType },
            { BadgeCategory.Status, BadgeType.Status },
            { BadgeCategory.Priorität, BadgeType.Priority },
            { BadgeCategory.Personen, BadgeType.Person },
            { BadgeCategory.Rechtsgrundlagen, BadgeType.Custom },
            { BadgeCategory.Finanzen, BadgeType.Custom },
            { BadgeCategory.Räumlich, BadgeType.Location },
            { BadgeCategory.Thematik, BadgeType.Topic },
            { BadgeCategory.Technisch, BadgeType.Custom },
            { BadgeCategory.Aktionen, BadgeType.Action }
        };
    }

    /// <summary>
    /// Icon für Kategorie
    /// </summary>
    private string GetCategoryIcon(BadgeCategory category)
    {
        return category switch
        {
            BadgeCategory.Zeit => "🕒",
            BadgeCategory.Organisation => "🏢",
            BadgeCategory.Vorgang => "📁",
            BadgeCategory.Status => "⚡",
            BadgeCategory.Priorität => "🔥",
            BadgeCategory.Personen => "👤",
            BadgeCategory.Rechtsgrundlagen => "⚖️",
            BadgeCategory.Finanzen => "💰",
            BadgeCategory.Räumlich => "📍",
            BadgeCategory.Thematik => "🏷️",
            BadgeCategory.Technisch => "⚙️",
            BadgeCategory.Aktionen => "✅",
            _ => "📋"
        };
    }

    /// <summary>
    /// Farbe für Kategorie
    /// </summary>
    private string GetCategoryColor(BadgeCategory category)
    {
        return category switch
        {
            BadgeCategory.Zeit => "#E3F2FD",
            BadgeCategory.Organisation => "#FFF3E0",
            BadgeCategory.Vorgang => "#F3E5F5",
            BadgeCategory.Status => "#FFF9C4",
            BadgeCategory.Priorität => "#FFEBEE",
            BadgeCategory.Personen => "#E8F5E9",
            BadgeCategory.Rechtsgrundlagen => "#E8EAF6",
            BadgeCategory.Finanzen => "#E0F2F1",
            BadgeCategory.Räumlich => "#F9FBE7",
            BadgeCategory.Thematik => "#FCE4EC",
            BadgeCategory.Technisch => "#F5F5F5",
            BadgeCategory.Aktionen => "#FBE9E7",
            _ => "#EEEEEE"
        };
    }
}

/// <summary>
/// Badge-Anzeige-Modus
/// </summary>
public enum BadgeDisplayMode
{
    FilledOnly,     // Nur ausgefüllte Felder
    All,            // Alle Felder
    Required,       // Nur Pflichtfelder
    Priority        // Pflichtfelder + ausgefüllte
}

/// <summary>
/// Sortier-Priorität für Badge-Summary
/// </summary>
public enum SummaryPriority
{
    RequiredFirst,             // Pflichtfelder zuerst
    AlphabeticalByCategory,    // Alphabetisch nach Kategorie
    MostRecent                 // Zuletzt bearbeitet zuerst
}

/// <summary>
/// Kategorie-basierte Badge-Zusammenfassung
/// </summary>
public class CategoryBadgeSummary
{
    public BadgeCategory Category { get; set; }
    public int BadgeCount { get; set; }
    public List<MetadataBadge> TopBadges { get; set; } = new();
    public string Icon { get; set; } = string.Empty;
    public string ColorHex { get; set; } = string.Empty;
}
