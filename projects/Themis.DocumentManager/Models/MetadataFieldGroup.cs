/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataFieldGroup.cs                              ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     284                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Gruppierung von Metadatenfeldern nach fachlichen/sachlichen Kategorien
/// </summary>
public class MetadataFieldGroup
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Title { get; set; } = string.Empty;
    public string Icon { get; set; } = "📋";
    public BadgeCategory ColorCategory { get; set; }
    public List<MetadataField> Fields { get; set; } = new();
    public int DisplayOrder { get; set; }
    public bool IsExpanded { get; set; } = true;

    /// <summary>
    /// Anzahl der ausgefüllten Felder in dieser Gruppe
    /// </summary>
    public int FilledFieldCount => Fields.Count(f => !string.IsNullOrEmpty(f.CurrentValue));

    /// <summary>
    /// Ist die Gruppe komplett leer (keine ausgefüllten Felder)?
    /// </summary>
    public bool IsEmpty => FilledFieldCount == 0;

    /// <summary>
    /// Prozentsatz der ausgefüllten Felder (0-100)
    /// </summary>
    public double CompletionPercentage =>
        Fields.Count > 0 ? (FilledFieldCount / (double)Fields.Count) * 100 : 0;

    /// <summary>
    /// Hat die Gruppe mindestens ein Pflichtfeld?
    /// </summary>
    public bool HasRequiredFields => Fields.Any(f => f.IsRequired);

    /// <summary>
    /// Sind alle Pflichtfelder ausgefüllt?
    /// </summary>
    public bool RequiredFieldsComplete =>
        !Fields.Any(f => f.IsRequired && string.IsNullOrEmpty(f.CurrentValue));
}

/// <summary>
/// Badge-Kategorien für farbliche Zuordnung
/// Mapping zu den 12 Badge-Typen aus MetadataBadgeModels
/// </summary>
public enum BadgeCategory
{
    Zeit,               // Date badges (blue #E3F2FD)
    Organisation,       // Department, Organization (orange #FFF3E0)
    Vorgang,           // ProcessType, FileReference (purple #F3E5F5)
    Status,            // Status badges (yellow #FFF9C4)
    Priorität,         // Priority badges (red #FFEBEE)
    Personen,          // Person badges (green #E8F5E9)
    Rechtsgrundlagen,  // Legal references (indigo #E8EAF6)
    Finanzen,          // Financial data (teal #E0F2F1)
    Räumlich,          // Location badges (lime #F9FBE7)
    Thematik,          // Topic badges (pink #FCE4EC)
    Technisch,         // Technical metadata (grey #F5F5F5)
    Aktionen           // Action, Deadline badges (deep orange #FBE9E7)
}

/// <summary>
/// Factory für Standard-Gruppendefinitionen
/// </summary>
public static class MetadataGroupFactory
{
    public static List<MetadataFieldGroup> CreateDefaultGroups()
    {
        return new List<MetadataFieldGroup>
        {
            new MetadataFieldGroup
            {
                Id = "group_zeitdaten",
                Title = "Zeitliche Daten",
                Icon = "🕒",
                ColorCategory = BadgeCategory.Zeit,
                DisplayOrder = 1
            },
            new MetadataFieldGroup
            {
                Id = "group_organisation",
                Title = "Organisation",
                Icon = "🏢",
                ColorCategory = BadgeCategory.Organisation,
                DisplayOrder = 2
            },
            new MetadataFieldGroup
            {
                Id = "group_vorgang",
                Title = "Vorgangsdetails",
                Icon = "📁",
                ColorCategory = BadgeCategory.Vorgang,
                DisplayOrder = 3
            },
            new MetadataFieldGroup
            {
                Id = "group_status",
                Title = "Status & Workflow",
                Icon = "⚡",
                ColorCategory = BadgeCategory.Status,
                DisplayOrder = 4
            },
            new MetadataFieldGroup
            {
                Id = "group_prioritaet",
                Title = "Priorität & Fristen",
                Icon = "🔥",
                ColorCategory = BadgeCategory.Priorität,
                DisplayOrder = 5
            },
            new MetadataFieldGroup
            {
                Id = "group_personen",
                Title = "Beteiligte Personen",
                Icon = "👤",
                ColorCategory = BadgeCategory.Personen,
                DisplayOrder = 6
            },
            new MetadataFieldGroup
            {
                Id = "group_rechtsgrundlagen",
                Title = "Rechtsgrundlagen",
                Icon = "⚖️",
                ColorCategory = BadgeCategory.Rechtsgrundlagen,
                DisplayOrder = 7
            },
            new MetadataFieldGroup
            {
                Id = "group_finanzen",
                Title = "Finanzdaten",
                Icon = "💰",
                ColorCategory = BadgeCategory.Finanzen,
                DisplayOrder = 8
            },
            new MetadataFieldGroup
            {
                Id = "group_raeumlich",
                Title = "Räumliche Zuordnung",
                Icon = "📍",
                ColorCategory = BadgeCategory.Räumlich,
                DisplayOrder = 9
            },
            new MetadataFieldGroup
            {
                Id = "group_thematik",
                Title = "Thematische Zuordnung",
                Icon = "🏷️",
                ColorCategory = BadgeCategory.Thematik,
                DisplayOrder = 10
            },
            new MetadataFieldGroup
            {
                Id = "group_technisch",
                Title = "Technische Metadaten",
                Icon = "⚙️",
                ColorCategory = BadgeCategory.Technisch,
                DisplayOrder = 11
            },
            new MetadataFieldGroup
            {
                Id = "group_aktionen",
                Title = "Aktionen & Aufgaben",
                Icon = "✅",
                ColorCategory = BadgeCategory.Aktionen,
                DisplayOrder = 12
            }
        };
    }

    /// <summary>
    /// Erstellt Gruppen-Titel zu Badge-Kategorie Mapping
    /// </summary>
    public static Dictionary<string, BadgeCategory> GetCategoryMapping()
    {
        return new Dictionary<string, BadgeCategory>(StringComparer.OrdinalIgnoreCase)
        {
            // Zeit
            { "datum", BadgeCategory.Zeit },
            { "date", BadgeCategory.Zeit },
            { "erstellt", BadgeCategory.Zeit },
            { "geändert", BadgeCategory.Zeit },
            { "zeitstempel", BadgeCategory.Zeit },

            // Organisation
            { "abteilung", BadgeCategory.Organisation },
            { "department", BadgeCategory.Organisation },
            { "organisation", BadgeCategory.Organisation },
            { "firma", BadgeCategory.Organisation },
            { "behörde", BadgeCategory.Organisation },

            // Vorgang
            { "aktenzeichen", BadgeCategory.Vorgang },
            { "vorgang", BadgeCategory.Vorgang },
            { "prozess", BadgeCategory.Vorgang },
            { "verfahren", BadgeCategory.Vorgang },

            // Status
            { "status", BadgeCategory.Status },
            { "zustand", BadgeCategory.Status },
            { "phase", BadgeCategory.Status },

            // Priorität
            { "priorität", BadgeCategory.Priorität },
            { "priority", BadgeCategory.Priorität },
            { "dringlichkeit", BadgeCategory.Priorität },
            { "frist", BadgeCategory.Priorität },

            // Personen
            { "person", BadgeCategory.Personen },
            { "bearbeiter", BadgeCategory.Personen },
            { "ersteller", BadgeCategory.Personen },
            { "verantwortlich", BadgeCategory.Personen },

            // Rechtsgrundlagen
            { "gesetz", BadgeCategory.Rechtsgrundlagen },
            { "paragraph", BadgeCategory.Rechtsgrundlagen },
            { "rechtsgrundlage", BadgeCategory.Rechtsgrundlagen },
            { "legal", BadgeCategory.Rechtsgrundlagen },

            // Finanzen
            { "betrag", BadgeCategory.Finanzen },
            { "kosten", BadgeCategory.Finanzen },
            { "budget", BadgeCategory.Finanzen },
            { "gebühr", BadgeCategory.Finanzen },

            // Räumlich
            { "ort", BadgeCategory.Räumlich },
            { "location", BadgeCategory.Räumlich },
            { "adresse", BadgeCategory.Räumlich },
            { "gemarkung", BadgeCategory.Räumlich },

            // Thematik
            { "thema", BadgeCategory.Thematik },
            { "topic", BadgeCategory.Thematik },
            { "kategorie", BadgeCategory.Thematik },
            { "schlagwort", BadgeCategory.Thematik },

            // Technisch
            { "datei", BadgeCategory.Technisch },
            { "file", BadgeCategory.Technisch },
            { "format", BadgeCategory.Technisch },
            { "version", BadgeCategory.Technisch },

            // Aktionen
            { "aufgabe", BadgeCategory.Aktionen },
            { "task", BadgeCategory.Aktionen },
            { "deadline", BadgeCategory.Aktionen },
            { "wiedervorlage", BadgeCategory.Aktionen }
        };
    }
}
