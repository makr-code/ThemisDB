/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataLayoutService.cs                           ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     153                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.UI;
using YamlDotNet.Serialization;
using YamlDotNet.Serialization.NamingConventions;

namespace Themis.DocumentManager.Features.MetadataForm.Services;

/// <summary>
/// Lädt die Metadaten-Layout-Konfiguration aus YAML und baut gruppierte Metadatenfelder.
/// </summary>
public class MetadataLayoutService : IMetadataLayoutService
{
    private readonly string _configPath;
    private readonly IDeserializer _deserializer;

    public MetadataLayoutService(string? configPath = null)
    {
        _configPath = configPath ?? Path.Combine(AppContext.BaseDirectory, "Config", "metadata_layout.yaml");
        _deserializer = new DeserializerBuilder()
            .WithNamingConvention(CamelCaseNamingConvention.Instance)
            .IgnoreUnmatchedProperties()
            .Build();
    }

    public LayoutConfig LoadLayout()
    {
        if (File.Exists(_configPath))
        {
            var yaml = File.ReadAllText(_configPath);
            var config = _deserializer.Deserialize<LayoutConfig>(yaml);
            return config ?? GetFallbackLayout();
        }

        return GetFallbackLayout();
    }

    public List<MetadataFieldGroup> BuildGroups(DocumentMetadataBinding binding, LayoutConfig? layoutConfig = null)
    {
        var config = layoutConfig ?? LoadLayout();
        var groups = new List<MetadataFieldGroup>();

        foreach (var groupDef in config.Groups.OrderBy(g => g.DisplayOrder))
        {
            var group = new MetadataFieldGroup
            {
                Id = groupDef.Id ?? Guid.NewGuid().ToString(),
                Title = groupDef.Title ?? "Metadaten",
                Icon = groupDef.Icon ?? "📋",
                DisplayOrder = groupDef.DisplayOrder,
                IsExpanded = config.Strategy is CollapseStrategy.ShowAllExpanded
            };

            foreach (var fieldDef in groupDef.Fields ?? Enumerable.Empty<FieldDefinition>())
            {
                var bound = binding.BoundFields.FirstOrDefault(f =>
                    f.ThemisPath.Equals(fieldDef.Path, StringComparison.OrdinalIgnoreCase) ||
                    f.FieldName.Equals(fieldDef.Name, StringComparison.OrdinalIgnoreCase));

                group.Fields.Add(new MetadataField
                {
                    FieldName = fieldDef.Name ?? bound?.FieldName ?? "Unbenannt",
                    ThemisPath = fieldDef.Path ?? bound?.ThemisPath ?? string.Empty,
                    Type = fieldDef.Type,
                    IsRequired = fieldDef.Required || (bound?.IsRequired ?? false),
                    DefaultValue = bound?.DefaultValue,
                    CurrentValue = bound?.CurrentValue ?? fieldDef.DefaultValue,
                    Options = fieldDef.Options  // Transfer dropdown options from YAML
                });
            }

            groups.Add(group);
        }

        return groups;
    }

    private LayoutConfig GetFallbackLayout()
    {
        return new LayoutConfig
        {
            Strategy = CollapseStrategy.HideEmptyFields,
            Groups = MetadataGroupFactory.CreateDefaultGroups()
                .Select(g => new GroupDefinition
                {
                    Id = g.Id,
                    Title = g.Title,
                    Icon = g.Icon,
                    DisplayOrder = g.DisplayOrder,
                    Fields = new List<FieldDefinition>()
                }).ToList()
        };
    }
}

public class LayoutConfig
{
    public CollapseStrategy Strategy { get; set; } = CollapseStrategy.HideEmptyFields;
    public List<GroupDefinition> Groups { get; set; } = new();
}

public class GroupDefinition
{
    public string? Id { get; set; }
    public string? Title { get; set; }
    public string? Icon { get; set; }
    public int DisplayOrder { get; set; }
    public List<FieldDefinition>? Fields { get; set; }
}

public class FieldDefinition
{
    public string? Name { get; set; }
    public string? Path { get; set; }
    public FieldType Type { get; set; } = FieldType.Text;
    public bool Required { get; set; }
    public string? DefaultValue { get; set; }
    public List<string>? Options { get; set; }
}


