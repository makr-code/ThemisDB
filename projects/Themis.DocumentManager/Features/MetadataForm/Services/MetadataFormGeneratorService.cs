/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataFormGeneratorService.cs                    ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     318                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using YamlDotNet.Serialization;
using YamlDotNet.Serialization.NamingConventions;

namespace Themis.DocumentManager.Features.MetadataForm.Services;

/// <summary>
/// Service zur Generierung von Metadaten-Formularen aus YAML-Templates
/// </summary>
public interface IMetadataFormGeneratorService
{
    Task<MetadataFormDefinition> LoadFormTemplateAsync(string entityType);
    Task<MetadataFormDefinition> LoadFormTemplateFromPathAsync(string filePath);
    List<MetadataFormDefinition> GetAvailableTemplates();
}

public class MetadataFormGeneratorService : IMetadataFormGeneratorService
{
    private readonly string _configPath;
    private readonly IDeserializer _deserializer;

    public MetadataFormGeneratorService(string? configPath = null)
    {
        _configPath = configPath ?? Path.Combine(AppContext.BaseDirectory, "Config");
        _deserializer = new DeserializerBuilder()
            .WithNamingConvention(CamelCaseNamingConvention.Instance)
            .IgnoreUnmatchedProperties()
            .Build();
    }

    public async Task<MetadataFormDefinition> LoadFormTemplateAsync(string entityType)
    {
        var fileName = $"metadata_{entityType.ToLowerInvariant()}.yaml";
        var filePath = Path.Combine(_configPath, fileName);
        return await LoadFormTemplateFromPathAsync(filePath);
    }

    public async Task<MetadataFormDefinition> LoadFormTemplateFromPathAsync(string filePath)
    {
        try
        {
            if (!File.Exists(filePath))
                throw new FileNotFoundException($"Metadaten-Template nicht gefunden: {filePath}");

            var yaml = await File.ReadAllTextAsync(filePath);
            var data = _deserializer.Deserialize<Dictionary<string, object>>(yaml);

            var form = new MetadataFormDefinition
            {
                Title = data?.ContainsKey("title") == true ? data["title"]?.ToString() ?? "Metadaten" : "Metadaten",
                Description = data?.ContainsKey("description") == true ? data["description"]?.ToString() ?? string.Empty : string.Empty,
                Sections = new ObservableCollection<MetadataFormSection>()
            };

            // Parse sections
            if (data?.ContainsKey("sections") == true && data["sections"] is List<object> sections)
            {
                var sectionOrder = 0;
                foreach (var section in sections)
                {
                    if (section is Dictionary<object, object> sectionDict)
                    {
                        var formSection = new MetadataFormSection
                        {
                            Id = Guid.NewGuid().ToString(),
                            Title = sectionDict.ContainsKey("title") ? sectionDict["title"]?.ToString() ?? $"Section {sectionOrder}" : $"Section {sectionOrder}",
                            Description = sectionDict.ContainsKey("description") ? sectionDict["description"]?.ToString() ?? string.Empty : string.Empty,
                            DisplayOrder = sectionOrder,
                            Fields = new ObservableCollection<MetadataFormField>()
                        };

                        // Parse fields in section
                        if (sectionDict.ContainsKey("fields") && sectionDict["fields"] is List<object> fields)
                        {
                            var fieldOrder = 0;
                            foreach (var field in fields)
                            {
                                if (field is Dictionary<object, object> fieldDict)
                                {
                                    var formField = ParseField(fieldDict, fieldOrder);
                                    formSection.Fields.Add(formField);
                                    fieldOrder++;
                                }
                            }
                        }

                        form.Sections.Add(formSection);
                        sectionOrder++;
                    }
                }
            }

            return form;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[ERROR] Loading metadata template: {ex}");
            throw;
        }
    }

    private MetadataFormField ParseField(Dictionary<object, object> fieldDict, int order)
    {
        var labelValue = fieldDict.ContainsKey("label") 
            ? fieldDict["label"]?.ToString() ?? string.Empty 
            : (fieldDict.ContainsKey("name") ? fieldDict["name"]?.ToString() ?? $"Field {order}" : $"Field {order}");

        var field = new MetadataFormField
        {
            Id = Guid.NewGuid().ToString(),
            Name = fieldDict.ContainsKey("name") ? fieldDict["name"]?.ToString() ?? $"Field_{order}" : $"Field_{order}",
            Label = labelValue,
            Description = fieldDict.ContainsKey("description") ? fieldDict["description"]?.ToString() ?? string.Empty : string.Empty,
            Type = fieldDict.ContainsKey("type") ? fieldDict["type"]?.ToString() ?? "text" : "text",
            DisplayOrder = order,
            IsRequired = fieldDict.ContainsKey("required") && fieldDict["required"]?.ToString()?.ToLowerInvariant() == "true",
            Placeholder = fieldDict.ContainsKey("placeholder") ? fieldDict["placeholder"]?.ToString() ?? string.Empty : string.Empty,
            DefaultValue = fieldDict.ContainsKey("default") ? fieldDict["default"]?.ToString() ?? string.Empty : string.Empty
        };

        // Parse options for dropdown/radio/checkbox fields
        if ((field.Type == "dropdown" || field.Type == "select" || field.Type == "radio" || field.Type == "checkbox") 
            && fieldDict.ContainsKey("options"))
        {
            if (fieldDict["options"] is List<object> options)
            {
                field.Options = new ObservableCollection<MetadataFormFieldOption>(
                    options.Select((o, i) => new MetadataFormFieldOption
                    {
                        Value = o?.ToString() ?? string.Empty,
                        Label = o?.ToString() ?? string.Empty,
                        Order = i
                    })
                );
            }
        }

        // Validierungsregeln
        if (fieldDict.ContainsKey("validation"))
        {
            if (fieldDict["validation"] is Dictionary<object, object> validation)
            {
                if (validation.ContainsKey("minLength"))
                    field.ValidationMinLength = int.TryParse(validation["minLength"].ToString(), out var min) ? min : 0;
                if (validation.ContainsKey("maxLength"))
                    field.ValidationMaxLength = int.TryParse(validation["maxLength"].ToString(), out var max) ? max : int.MaxValue;
                if (validation.ContainsKey("pattern"))
                    field.ValidationPattern = validation["pattern"].ToString();
            }
        }

        return field;
    }

    public List<MetadataFormDefinition> GetAvailableTemplates()
    {
        var templates = new List<MetadataFormDefinition>();
        var templates_names = new[] { "datei", "dokument", "vorgang", "akte", "ablage" };

        foreach (var name in templates_names)
        {
            try
            {
                var filePath = Path.Combine(_configPath, $"metadata_{name}.yaml");
                if (File.Exists(filePath))
                {
                    var template = LoadFormTemplateFromPathAsync(filePath).GetAwaiter().GetResult();
                    templates.Add(template);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"[WARN] Could not load template {name}: {ex.Message}");
            }
        }

        return templates;
    }
}

#region Data Models

public class MetadataFormDefinition
{
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public ObservableCollection<MetadataFormSection> Sections { get; set; } = new();
}

public class MetadataFormSection
{
    public string Id { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public int DisplayOrder { get; set; }
    public ObservableCollection<MetadataFormField> Fields { get; set; } = new();
    public bool IsExpanded { get; set; } = true;
}

public class MetadataFormField
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string Type { get; set; } = "text"; // text, email, phone, number, date, datetime, dropdown, radio, checkbox, textarea, tags, file, color
    public int DisplayOrder { get; set; }
    public bool IsRequired { get; set; }
    public string Placeholder { get; set; } = string.Empty;
    public string DefaultValue { get; set; } = string.Empty;
    public string? Value { get; set; }

    // Dropdown/Select Options
    public ObservableCollection<MetadataFormFieldOption> Options { get; set; } = new();

    // Validierung
    public int ValidationMinLength { get; set; }
    public int ValidationMaxLength { get; set; } = int.MaxValue;
    public string? ValidationPattern { get; set; }

    // Computed Properties
    public bool HasError { get; set; }
    public string? ErrorMessage { get; set; }

    public bool Validate()
    {
        HasError = false;
        ErrorMessage = null;

        // Required check
        if (IsRequired && (string.IsNullOrWhiteSpace(Value) || string.IsNullOrWhiteSpace(DefaultValue)))
        {
            HasError = true;
            ErrorMessage = $"{Label} ist erforderlich";
            return false;
        }

        if (string.IsNullOrWhiteSpace(Value) && string.IsNullOrWhiteSpace(DefaultValue))
            return true;

        var val = Value ?? DefaultValue;

        // Min length check
        if (ValidationMinLength > 0 && val.Length < ValidationMinLength)
        {
            HasError = true;
            ErrorMessage = $"Mindestlänge: {ValidationMinLength} Zeichen";
            return false;
        }

        // Max length check
        if (ValidationMaxLength < int.MaxValue && val.Length > ValidationMaxLength)
        {
            HasError = true;
            ErrorMessage = $"Maximallänge: {ValidationMaxLength} Zeichen";
            return false;
        }

        // Pattern check
        if (!string.IsNullOrWhiteSpace(ValidationPattern))
        {
            try
            {
                var regex = new System.Text.RegularExpressions.Regex(ValidationPattern);
                if (!regex.IsMatch(val))
                {
                    HasError = true;
                    ErrorMessage = $"Format ungültig";
                    return false;
                }
            }
            catch
            {
                System.Diagnostics.Debug.WriteLine($"Invalid regex pattern: {ValidationPattern}");
            }
        }

        return true;
    }
}

public class MetadataFormFieldOption
{
    public string Value { get; set; } = string.Empty;
    public string Label { get; set; } = string.Empty;
    public int Order { get; set; }
}

#endregion


