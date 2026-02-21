/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SmartMetadataLayoutEngine.cs                       ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     325                                            ║
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

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.Generic;
using System.Linq;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Features.MetadataForm.Services;

/// <summary>
/// Intelligente Metadaten-Layout-Engine
/// Analysiert Metadaten und erstellt optimale Gruppierungen
/// </summary>
public class SmartMetadataLayoutEngine : ISmartMetadataLayoutEngine
{
    private readonly Dictionary<string, BadgeCategory> _categoryMapping;
    private readonly List<MetadataFieldGroup> _defaultGroups;

    public SmartMetadataLayoutEngine()
    {
        _categoryMapping = MetadataGroupFactory.GetCategoryMapping();
        _defaultGroups = MetadataGroupFactory.CreateDefaultGroups();
    }

    /// <summary>
    /// Erstellt optimales Layout basierend auf Metadaten
    /// </summary>
    public List<MetadataFieldGroup> CreateOptimalLayout(DocumentMetadataBinding metadata)
    {
        var groups = CloneDefaultGroups();

        foreach (var field in metadata.BoundFields)
        {
            var targetGroup = DetectFieldGroup(field);
            targetGroup.Fields.Add(field);
        }

        // Sortiere nach Relevanz (ausgefüllte Felder zuerst)
        return SortByRelevance(groups);
    }

    /// <summary>
    /// Erstellt Layout nur mit ausgefüllten Feldern
    /// </summary>
    public List<MetadataFieldGroup> CreateCompactLayout(DocumentMetadataBinding metadata)
    {
        var fullLayout = CreateOptimalLayout(metadata);

        // Filtere leere Gruppen komplett raus
        var compactLayout = fullLayout
            .Where(g => !g.IsEmpty)
            .ToList();

        // Entferne leere Felder aus Gruppen
        foreach (var group in compactLayout)
        {
            group.Fields = group.Fields
                .Where(f => !string.IsNullOrEmpty(f.CurrentValue))
                .ToList();
        }

        return compactLayout;
    }

    /// <summary>
    /// Erkennt passende Gruppe für ein Feld
    /// </summary>
    public MetadataFieldGroup DetectFieldGroup(MetadataField field)
    {
        var fieldNameLower = field.FieldName.ToLowerInvariant();
        var themisPathLower = field.ThemisPath?.ToLowerInvariant() ?? "";

        // 1. Versuche über Keyword-Matching
        foreach (var kvp in _categoryMapping)
        {
            if (fieldNameLower.Contains(kvp.Key) || themisPathLower.Contains(kvp.Key))
            {
                var group = _defaultGroups.FirstOrDefault(g => g.ColorCategory == kvp.Value);
                if (group != null)
                    return group;
            }
        }

        // 2. Fallback: ThemisPath-basierte Erkennung
        var pathCategory = DetectCategoryFromPath(field.ThemisPath);
        if (pathCategory.HasValue)
        {
            var group = _defaultGroups.FirstOrDefault(g => g.ColorCategory == pathCategory.Value);
            if (group != null)
                return group;
        }

        // 3. Fallback: Feldtyp-basierte Zuordnung
        var typeCategory = DetectCategoryFromFieldType(field.Type.ToString());
        if (typeCategory.HasValue)
        {
            var group = _defaultGroups.FirstOrDefault(g => g.ColorCategory == typeCategory.Value);
            if (group != null)
                return group;
        }

        // 4. Default: Technische Metadaten
        return _defaultGroups.First(g => g.ColorCategory == BadgeCategory.Technisch);
    }

    /// <summary>
    /// Sortiert Gruppen nach Relevanz
    /// </summary>
    public List<MetadataFieldGroup> SortByRelevance(List<MetadataFieldGroup> groups)
    {
        return groups
            .OrderByDescending(g => g.FilledFieldCount) // Ausgefüllte zuerst
            .ThenByDescending(g => g.HasRequiredFields) // Pflichtfelder wichtig
            .ThenBy(g => g.DisplayOrder)                // Original-Reihenfolge
            .ToList();
    }

    /// <summary>
    /// Erkennt Kategorie aus ThemisDB-Pfad
    /// </summary>
    private BadgeCategory? DetectCategoryFromPath(string? themisPath)
    {
        if (string.IsNullOrEmpty(themisPath))
            return null;

        var pathLower = themisPath.ToLowerInvariant();

        // Zeitdaten
        if (pathLower.Contains("datum") || pathLower.Contains("zeitstempel") || 
            pathLower.Contains("timestamp") || pathLower.Contains("created") ||
            pathLower.Contains("modified"))
            return BadgeCategory.Zeit;

        // Organisation
        if (pathLower.Contains("abteilung") || pathLower.Contains("department") ||
            pathLower.Contains("organisation") || pathLower.Contains("behörde"))
            return BadgeCategory.Organisation;

        // Vorgang
        if (pathLower.Contains("akte") || pathLower.Contains("vorgang") ||
            pathLower.Contains("prozess") || pathLower.Contains("verfahren"))
            return BadgeCategory.Vorgang;

        // Status
        if (pathLower.Contains("status") || pathLower.Contains("zustand") ||
            pathLower.Contains("phase") || pathLower.Contains("workflow"))
            return BadgeCategory.Status;

        // Priorität
        if (pathLower.Contains("priorität") || pathLower.Contains("priority") ||
            pathLower.Contains("frist") || pathLower.Contains("deadline"))
            return BadgeCategory.Priorität;

        // Personen
        if (pathLower.Contains("person") || pathLower.Contains("bearbeiter") ||
            pathLower.Contains("ersteller") || pathLower.Contains("user"))
            return BadgeCategory.Personen;

        // Rechtsgrundlagen
        if (pathLower.Contains("gesetz") || pathLower.Contains("paragraph") ||
            pathLower.Contains("legal") || pathLower.Contains("rechtsgrundlage"))
            return BadgeCategory.Rechtsgrundlagen;

        // Finanzen
        if (pathLower.Contains("betrag") || pathLower.Contains("kosten") ||
            pathLower.Contains("budget") || pathLower.Contains("gebühr"))
            return BadgeCategory.Finanzen;

        // Räumlich
        if (pathLower.Contains("ort") || pathLower.Contains("location") ||
            pathLower.Contains("adresse") || pathLower.Contains("gemarkung") ||
            pathLower.Contains("flur"))
            return BadgeCategory.Räumlich;

        // Thematik
        if (pathLower.Contains("thema") || pathLower.Contains("topic") ||
            pathLower.Contains("kategorie") || pathLower.Contains("schlagwort"))
            return BadgeCategory.Thematik;

        // Aktionen
        if (pathLower.Contains("aufgabe") || pathLower.Contains("task") ||
            pathLower.Contains("wiedervorlage") || pathLower.Contains("action"))
            return BadgeCategory.Aktionen;

        return null;
    }

    /// <summary>
    /// Erkennt Kategorie aus Feldtyp
    /// </summary>
    private BadgeCategory? DetectCategoryFromFieldType(string? fieldType)
    {
        if (string.IsNullOrEmpty(fieldType))
            return null;

        var typeLower = fieldType.ToLowerInvariant();

        // Datum-Felder → Zeit
        if (typeLower.Contains("date") || typeLower.Contains("datetime") ||
            typeLower.Contains("timestamp"))
            return BadgeCategory.Zeit;

        // Zahlenfelder → Finanzen
        if (typeLower.Contains("decimal") || typeLower.Contains("money") ||
            typeLower.Contains("currency"))
            return BadgeCategory.Finanzen;

        // Geo-Felder → Räumlich
        if (typeLower.Contains("geo") || typeLower.Contains("location") ||
            typeLower.Contains("coordinate"))
            return BadgeCategory.Räumlich;

        return null;
    }

    /// <summary>
    /// Klont Default-Gruppen (leere Feldlisten)
    /// </summary>
    private List<MetadataFieldGroup> CloneDefaultGroups()
    {
        return _defaultGroups.Select(g => new MetadataFieldGroup
        {
            Id = g.Id,
            Title = g.Title,
            Icon = g.Icon,
            ColorCategory = g.ColorCategory,
            DisplayOrder = g.DisplayOrder,
            Fields = new List<MetadataField>()
        }).ToList();
    }

    /// <summary>
    /// Erstellt Badge-kompatible Gruppierung
    /// </summary>
    public Dictionary<BadgeCategory, List<MetadataField>> CreateBadgeGrouping(DocumentMetadataBinding metadata)
    {
        var result = new Dictionary<BadgeCategory, List<MetadataField>>();

        foreach (var field in metadata.BoundFields.Where(f => !string.IsNullOrEmpty(f.CurrentValue)))
        {
            var group = DetectFieldGroup(field);
            var category = group.ColorCategory;

            if (!result.ContainsKey(category))
                result[category] = new List<MetadataField>();

            result[category].Add(field);
        }

        return result;
    }

    /// <summary>
    /// Berechnet Statistiken für Layout
    /// </summary>
    public MetadataLayoutStatistics CalculateStatistics(List<MetadataFieldGroup> groups)
    {
        return new MetadataLayoutStatistics
        {
            TotalGroups = groups.Count,
            FilledGroups = groups.Count(g => !g.IsEmpty),
            EmptyGroups = groups.Count(g => g.IsEmpty),
            TotalFields = groups.Sum(g => g.Fields.Count),
            FilledFields = groups.Sum(g => g.FilledFieldCount),
            RequiredFields = groups.Sum(g => g.Fields.Count(f => f.IsRequired)),
            CompletionPercentage = CalculateOverallCompletion(groups),
            MostFilledGroup = groups.OrderByDescending(g => g.FilledFieldCount).FirstOrDefault()?.Title ?? "N/A",
            LeastFilledGroup = groups.Where(g => !g.IsEmpty).OrderBy(g => g.FilledFieldCount).FirstOrDefault()?.Title ?? "N/A"
        };
    }

    private double CalculateOverallCompletion(List<MetadataFieldGroup> groups)
    {
        var totalFields = groups.Sum(g => g.Fields.Count);
        if (totalFields == 0) return 0;

        var filledFields = groups.Sum(g => g.FilledFieldCount);
        return (filledFields / (double)totalFields) * 100;
    }
}

/// <summary>
/// Layout-Statistiken
/// </summary>
public class MetadataLayoutStatistics
{
    public int TotalGroups { get; set; }
    public int FilledGroups { get; set; }
    public int EmptyGroups { get; set; }
    public int TotalFields { get; set; }
    public int FilledFields { get; set; }
    public int RequiredFields { get; set; }
    public double CompletionPercentage { get; set; }
    public string MostFilledGroup { get; set; } = string.Empty;
    public string LeastFilledGroup { get; set; } = string.Empty;
}


