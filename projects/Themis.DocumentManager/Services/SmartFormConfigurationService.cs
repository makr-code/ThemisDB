/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SmartFormConfigurationService.cs                   ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     556                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
/// Configuration options for SmartForm display and behavior
/// </summary>
public class SmartFormDisplayConfig
{
    public string FormId { get; set; } = string.Empty;
    public string TemplateName { get; set; } = string.Empty;
    
    // Layout-Konfiguration
    public SmartFormLayoutConfig Layout { get; set; } = new();
    
    // Feld-Anpassungen
    public Dictionary<string, SmartFieldDisplayConfig> FieldConfigs { get; set; } = new();
    
    // Section-Anpassungen
    public Dictionary<string, SmartSectionDisplayConfig> SectionConfigs { get; set; } = new();
    
    // Allgemeine Einstellungen
    public bool EnableAutoBadging { get; set; } = true;
    public bool EnableFieldLabeling { get; set; } = true;
    public bool EnableLLMSupport { get; set; } = true;
    public bool EnableFieldTooltips { get; set; } = true;
    public bool EnableFieldValidationMessages { get; set; } = true;
    public bool EnableFieldDescriptions { get; set; } = true;
    
    // Styling
    public SmartFormStyling Styling { get; set; } = new();
    
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime ModifiedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = "System";
}

/// <summary>
/// Layout configuration for SmartForms
/// </summary>
public class SmartFormLayoutConfig
{
    public int ColumnsCount { get; set; } = 1;
    public double ColumnWidthPercentage { get; set; } = 100;
    public double FieldSpacing { get; set; } = 10;
    public double SectionSpacing { get; set; } = 20;
    public bool ShowFieldNumbers { get; set; } = false;
    public bool ShowProgressBar { get; set; } = true;
    public bool CompactMode { get; set; } = false;
    public string Orientation { get; set; } = "Vertical"; // Vertical, Horizontal
}

/// <summary>
/// Styling configuration for SmartForms
/// </summary>
public class SmartFormStyling
{
    public string PrimaryColor { get; set; } = "#2196F3";
    public string AccentColor { get; set; } = "#FF9800";
    public string ErrorColor { get; set; } = "#F44336";
    public string SuccessColor { get; set; } = "#4CAF50";
    public string BackgroundColor { get; set; } = "#FFFFFF";
    public string FontFamily { get; set; } = "Segoe UI";
    public double FontSize { get; set; } = 12;
    public double LabelFontSize { get; set; } = 13;
    public bool UseGermanFont { get; set; } = true;
}

/// <summary>
/// Configuration for individual form fields
/// </summary>
public class SmartFieldDisplayConfig
{
    public string FieldId { get; set; } = string.Empty;
    public string FieldName { get; set; } = string.Empty;
    
    // Label und Beschreibung
    public string? CustomLabel { get; set; }
    public string? CustomDescription { get; set; }
    public string? CustomPlaceholder { get; set; }
    public string? HelpText { get; set; }
    
    // LLM-generierte Inhalte
    public string? LLMGeneratedLabel { get; set; }
    public string? LLMGeneratedDescription { get; set; }
    public double LLMLabelConfidence { get; set; } = 0.0;
    
    // Sichtbarkeit und Verhalten
    public bool IsVisible { get; set; } = true;
    public bool IsRequired { get; set; } = false;
    public bool IsReadOnly { get; set; } = false;
    public bool EnableAutoBadging { get; set; } = true;
    public bool EnableSuggestions { get; set; } = true;
    
    // Styling
    public string? CustomForegroundColor { get; set; }
    public string? CustomBackgroundColor { get; set; }
    public double? CustomFontSize { get; set; }
    public bool IsBold { get; set; } = false;
    public bool IsItalic { get; set; } = false;
    
    // Validierung
    public string? CustomValidationMessage { get; set; }
    public int? MinLength { get; set; }
    public int? MaxLength { get; set; }
    
    // Badge-Konfiguration
    public List<string> AllowedBadgeTypes { get; set; } = new();
    public double MinimumBadgeConfidence { get; set; } = 0.75;
    
    // Anzeige-Optionen
    public int FieldWidth { get; set; } = 100; // in Prozent
    public int SortOrder { get; set; } = 0;
    
    public DateTime ModifiedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// Configuration for form sections
/// </summary>
public class SmartSectionDisplayConfig
{
    public string SectionId { get; set; } = string.Empty;
    public string SectionName { get; set; } = string.Empty;
    
    public string? CustomTitle { get; set; }
    public string? CustomDescription { get; set; }
    
    // LLM-generierte Inhalte
    public string? LLMGeneratedTitle { get; set; }
    public string? LLMGeneratedDescription { get; set; }
    
    public bool IsExpanded { get; set; } = true;
    public bool IsCollapsible { get; set; } = true;
    public bool IsVisible { get; set; } = true;
    
    public int ColumnsCount { get; set; } = 1;
    public int SortOrder { get; set; } = 0;
    
    // Styling
    public string? CustomBackgroundColor { get; set; }
    public string? CustomBorderColor { get; set; }
    public string? CustomTitleColor { get; set; }
    
    public DateTime ModifiedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// Service interface for SmartForm configuration management
/// </summary>
public interface ISmartFormConfigurationService
{
    // Configuration management
    Task<SmartFormDisplayConfig> GetFormConfigAsync(string formId, CancellationToken cancellationToken = default);
    Task<SmartFormDisplayConfig> CreateFormConfigAsync(SmartFormDisplayConfig config, CancellationToken cancellationToken = default);
    Task<SmartFormDisplayConfig> UpdateFormConfigAsync(SmartFormDisplayConfig config, CancellationToken cancellationToken = default);
    Task<bool> DeleteFormConfigAsync(string formId, CancellationToken cancellationToken = default);
    Task<List<SmartFormDisplayConfig>> GetAllConfigsAsync(CancellationToken cancellationToken = default);
    
    // Field configuration
    Task<SmartFieldDisplayConfig> GetFieldConfigAsync(string formId, string fieldId, CancellationToken cancellationToken = default);
    Task<SmartFieldDisplayConfig> UpdateFieldConfigAsync(string formId, SmartFieldDisplayConfig fieldConfig, CancellationToken cancellationToken = default);
    Task<List<SmartFieldDisplayConfig>> GetFieldConfigsForFormAsync(string formId, CancellationToken cancellationToken = default);
    
    // Section configuration
    Task<SmartSectionDisplayConfig> GetSectionConfigAsync(string formId, string sectionId, CancellationToken cancellationToken = default);
    Task<SmartSectionDisplayConfig> UpdateSectionConfigAsync(string formId, SmartSectionDisplayConfig sectionConfig, CancellationToken cancellationToken = default);
    
    // Label management
    Task<string> GetCustomLabelAsync(string formId, string fieldId, CancellationToken cancellationToken = default);
    Task UpdateCustomLabelAsync(string formId, string fieldId, string label, CancellationToken cancellationToken = default);
    
    // Layout management
    Task<SmartFormLayoutConfig> GetLayoutConfigAsync(string formId, CancellationToken cancellationToken = default);
    Task UpdateLayoutConfigAsync(string formId, SmartFormLayoutConfig layoutConfig, CancellationToken cancellationToken = default);
    
    // Styling management
    Task<SmartFormStyling> GetStylingAsync(string formId, CancellationToken cancellationToken = default);
    Task UpdateStylingAsync(string formId, SmartFormStyling styling, CancellationToken cancellationToken = default);
    
    // Batch operations
    Task ApplyConfigToMultipleFieldsAsync(string formId, List<string> fieldIds, Action<SmartFieldDisplayConfig> configAction, CancellationToken cancellationToken = default);
    Task ResetFormConfigAsync(string formId, CancellationToken cancellationToken = default);
    
    // Import/Export
    Task<string> ExportConfigAsJsonAsync(string formId, CancellationToken cancellationToken = default);
    Task ImportConfigFromJsonAsync(string formId, string jsonConfig, CancellationToken cancellationToken = default);
}

/// <summary>
/// Implementation of SmartForm configuration service
/// </summary>
public class SmartFormConfigurationService : ISmartFormConfigurationService
{
    private readonly Dictionary<string, SmartFormDisplayConfig> _formConfigs = new();
    private readonly object _lockObject = new();

    public Task<SmartFormDisplayConfig> GetFormConfigAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var config))
            {
                return Task.FromResult(config);
            }
            
            // Default config wenn nicht vorhanden
            var defaultConfig = new SmartFormDisplayConfig
            {
                FormId = formId,
                TemplateName = formId,
                CreatedAt = DateTime.UtcNow
            };
            
            return Task.FromResult(defaultConfig);
        }
    }

    public Task<SmartFormDisplayConfig> CreateFormConfigAsync(SmartFormDisplayConfig config, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            config.CreatedAt = DateTime.UtcNow;
            config.ModifiedAt = DateTime.UtcNow;
            _formConfigs[config.FormId] = config;
            return Task.FromResult(config);
        }
    }

    public Task<SmartFormDisplayConfig> UpdateFormConfigAsync(SmartFormDisplayConfig config, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            config.ModifiedAt = DateTime.UtcNow;
            _formConfigs[config.FormId] = config;
            return Task.FromResult(config);
        }
    }

    public Task<bool> DeleteFormConfigAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            return Task.FromResult(_formConfigs.Remove(formId));
        }
    }

    public Task<List<SmartFormDisplayConfig>> GetAllConfigsAsync(CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            return Task.FromResult(_formConfigs.Values.ToList());
        }
    }

    public Task<SmartFieldDisplayConfig> GetFieldConfigAsync(string formId, string fieldId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var formConfig))
            {
                if (formConfig.FieldConfigs.TryGetValue(fieldId, out var fieldConfig))
                {
                    return Task.FromResult(fieldConfig);
                }
            }
            
            // Default field config
            var defaultFieldConfig = new SmartFieldDisplayConfig
            {
                FieldId = fieldId,
                FieldName = fieldId
            };
            
            return Task.FromResult(defaultFieldConfig);
        }
    }

    public Task<SmartFieldDisplayConfig> UpdateFieldConfigAsync(string formId, SmartFieldDisplayConfig fieldConfig, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var formConfig))
            {
                fieldConfig.ModifiedAt = DateTime.UtcNow;
                formConfig.FieldConfigs[fieldConfig.FieldId] = fieldConfig;
                formConfig.ModifiedAt = DateTime.UtcNow;
            }
            
            return Task.FromResult(fieldConfig);
        }
    }

    public Task<List<SmartFieldDisplayConfig>> GetFieldConfigsForFormAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var formConfig))
            {
                return Task.FromResult(formConfig.FieldConfigs.Values.ToList());
            }
            
            return Task.FromResult(new List<SmartFieldDisplayConfig>());
        }
    }

    public Task<SmartSectionDisplayConfig> GetSectionConfigAsync(string formId, string sectionId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var formConfig))
            {
                if (formConfig.SectionConfigs.TryGetValue(sectionId, out var sectionConfig))
                {
                    return Task.FromResult(sectionConfig);
                }
            }
            
            var defaultSectionConfig = new SmartSectionDisplayConfig
            {
                SectionId = sectionId,
                SectionName = sectionId
            };
            
            return Task.FromResult(defaultSectionConfig);
        }
    }

    public Task<SmartSectionDisplayConfig> UpdateSectionConfigAsync(string formId, SmartSectionDisplayConfig sectionConfig, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var formConfig))
            {
                sectionConfig.ModifiedAt = DateTime.UtcNow;
                formConfig.SectionConfigs[sectionConfig.SectionId] = sectionConfig;
                formConfig.ModifiedAt = DateTime.UtcNow;
            }
            
            return Task.FromResult(sectionConfig);
        }
    }

    public Task<string> GetCustomLabelAsync(string formId, string fieldId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var formConfig) &&
                formConfig.FieldConfigs.TryGetValue(fieldId, out var fieldConfig))
            {
                // Priorisierung: CustomLabel > LLMGeneratedLabel > FieldName
                return Task.FromResult(
                    fieldConfig.CustomLabel ?? 
                    fieldConfig.LLMGeneratedLabel ?? 
                    fieldConfig.FieldName
                );
            }
            
            return Task.FromResult(fieldId);
        }
    }

    public Task UpdateCustomLabelAsync(string formId, string fieldId, string label, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var formConfig))
            {
                if (!formConfig.FieldConfigs.ContainsKey(fieldId))
                {
                    formConfig.FieldConfigs[fieldId] = new SmartFieldDisplayConfig
                    {
                        FieldId = fieldId,
                        FieldName = fieldId
                    };
                }
                
                formConfig.FieldConfigs[fieldId].CustomLabel = label;
                formConfig.FieldConfigs[fieldId].ModifiedAt = DateTime.UtcNow;
                formConfig.ModifiedAt = DateTime.UtcNow;
            }
            
            return Task.CompletedTask;
        }
    }

    public Task<SmartFormLayoutConfig> GetLayoutConfigAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var config))
            {
                return Task.FromResult(config.Layout);
            }
            
            return Task.FromResult(new SmartFormLayoutConfig());
        }
    }

    public Task UpdateLayoutConfigAsync(string formId, SmartFormLayoutConfig layoutConfig, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var config))
            {
                config.Layout = layoutConfig;
                config.ModifiedAt = DateTime.UtcNow;
            }
            
            return Task.CompletedTask;
        }
    }

    public Task<SmartFormStyling> GetStylingAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var config))
            {
                return Task.FromResult(config.Styling);
            }
            
            return Task.FromResult(new SmartFormStyling());
        }
    }

    public Task UpdateStylingAsync(string formId, SmartFormStyling styling, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var config))
            {
                config.Styling = styling;
                config.ModifiedAt = DateTime.UtcNow;
            }
            
            return Task.CompletedTask;
        }
    }

    public Task ApplyConfigToMultipleFieldsAsync(string formId, List<string> fieldIds, Action<SmartFieldDisplayConfig> configAction, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var formConfig))
            {
                foreach (var fieldId in fieldIds)
                {
                    if (!formConfig.FieldConfigs.ContainsKey(fieldId))
                    {
                        formConfig.FieldConfigs[fieldId] = new SmartFieldDisplayConfig
                        {
                            FieldId = fieldId,
                            FieldName = fieldId
                        };
                    }
                    
                    configAction(formConfig.FieldConfigs[fieldId]);
                    formConfig.FieldConfigs[fieldId].ModifiedAt = DateTime.UtcNow;
                }
                
                formConfig.ModifiedAt = DateTime.UtcNow;
            }
            
            return Task.CompletedTask;
        }
    }

    public Task ResetFormConfigAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var config))
            {
                config.FieldConfigs.Clear();
                config.SectionConfigs.Clear();
                config.Layout = new SmartFormLayoutConfig();
                config.Styling = new SmartFormStyling();
                config.ModifiedAt = DateTime.UtcNow;
            }
            
            return Task.CompletedTask;
        }
    }

    public Task<string> ExportConfigAsJsonAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_formConfigs.TryGetValue(formId, out var config))
            {
                var json = System.Text.Json.JsonSerializer.Serialize(config, new System.Text.Json.JsonSerializerOptions 
                { 
                    WriteIndented = true 
                });
                return Task.FromResult(json);
            }
            
            return Task.FromResult(string.Empty);
        }
    }

    public Task ImportConfigFromJsonAsync(string formId, string jsonConfig, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            try
            {
                var config = System.Text.Json.JsonSerializer.Deserialize<SmartFormDisplayConfig>(jsonConfig);
                if (config != null)
                {
                    config.FormId = formId;
                    config.ModifiedAt = DateTime.UtcNow;
                    _formConfigs[formId] = config;
                }
            }
            catch
            {
                // JSON parsing failed
            }
            
            return Task.CompletedTask;
        }
    }
}
