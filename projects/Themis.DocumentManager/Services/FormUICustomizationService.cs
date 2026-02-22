/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FormUICustomizationService.cs                      ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     737                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace Themis.DocumentManager.Services;

/// <summary>
/// UI customization options for SmartForms
/// </summary>
public class FormUICustomization
{
    public string FormId { get; set; } = string.Empty;
    
    // Theme
    public SmartFormTheme Theme { get; set; } = new();
    
    // Field rendering
    public FieldRenderingOptions FieldRendering { get; set; } = new();
    
    // Validation display
    public ValidationDisplayOptions ValidationDisplay { get; set; } = new();
    
    // Help and hints
    public HelpDisplayOptions HelpDisplay { get; set; } = new();
    
    // Badge customization
    public BadgeCustomizationOptions BadgeCustomization { get; set; } = new();
    
    // Form behavior
    public FormBehaviorOptions FormBehavior { get; set; } = new();
    
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime ModifiedAt { get; set; } = DateTime.UtcNow;
}

/// <summary>
/// Theme configuration for forms
/// </summary>
public class SmartFormTheme
{
    public string ThemeName { get; set; } = "Light";
    public string PrimaryColor { get; set; } = "#2196F3";
    public string SecondaryColor { get; set; } = "#FF9800";
    public string BackgroundColor { get; set; } = "#FFFFFF";
    public string TextColor { get; set; } = "#212121";
    public string BorderColor { get; set; } = "#BDBDBD";
    public double BorderRadius { get; set; } = 4.0;
    public double BorderThickness { get; set; } = 1.0;
    public string FontFamily { get; set; } = "Segoe UI";
    public double FontSize { get; set; } = 12.0;
    public bool UseDarkMode { get; set; } = false;
    public bool UseCompactLayout { get; set; } = false;
}

/// <summary>
/// Field rendering options
/// </summary>
public class FieldRenderingOptions
{
    public bool ShowFieldLabels { get; set; } = true;
    public bool ShowFieldNumbers { get; set; } = false;
    public bool ShowRequiredIndicators { get; set; } = true;
    public string RequiredIndicator { get; set; } = "*";
    public bool ShowFieldDescriptions { get; set; } = true;
    public bool ShowPlaceholderText { get; set; } = true;
    public FieldLabelPosition LabelPosition { get; set; } = FieldLabelPosition.Above;
    public double LabelWidth { get; set; } = 30; // percentage
    public bool ShowFieldBorders { get; set; } = true;
    public bool ShowFieldShadows { get; set; } = false;
    public double FieldSpacing { get; set; } = 10.0;
    public int ColumnsCount { get; set; } = 1;
    public bool UseAutoLayouting { get; set; } = true;
    public bool ShowFieldIcons { get; set; } = true;
    public bool CompactFieldDisplay { get; set; } = false;
}

/// <summary>
/// Label position options
/// </summary>
public enum FieldLabelPosition
{
    Above,      // Label oben über dem Feld
    Left,       // Label links neben dem Feld
    Right,      // Label rechts neben dem Feld
    Floating,   // Label schwebt über dem Feld
    Placeholder // Label wird als Placeholder angezeigt
}

/// <summary>
/// Validation display options
/// </summary>
public class ValidationDisplayOptions
{
    public bool ShowErrorMessages { get; set; } = true;
    public bool ShowWarningMessages { get; set; } = true;
    public bool ShowSuccessMessages { get; set; } = false;
    public bool ShowValidationIcons { get; set; } = true;
    public bool ShowFieldTooltips { get; set; } = true;
    public bool HighlightInvalidFields { get; set; } = true;
    public string ErrorColor { get; set; } = "#F44336";
    public string WarningColor { get; set; } = "#FF9800";
    public string SuccessColor { get; set; } = "#4CAF50";
    public bool ScrollToFirstError { get; set; } = true;
    public bool ValidateOnChange { get; set; } = true;
    public bool ValidateOnBlur { get; set; } = true;
    public bool ShowValidationSummary { get; set; } = false;
}

/// <summary>
/// Help display options
/// </summary>
public class HelpDisplayOptions
{
    public bool ShowHelpText { get; set; } = true;
    public bool ShowHelpIcons { get; set; } = true;
    public bool ShowExamples { get; set; } = true;
    public bool ShowFieldHints { get; set; } = true;
    public bool ShowContextualHelp { get; set; } = true;
    public HelpDisplayMode HelpDisplayMode { get; set; } = HelpDisplayMode.Tooltip;
    public bool ShowLLMGeneratedHints { get; set; } = true;
    public bool EnableFieldCopilot { get; set; } = true;
    public bool ShowRelatedFields { get; set; } = true;
}

/// <summary>
/// Help display mode options
/// </summary>
public enum HelpDisplayMode
{
    Tooltip,    // Als Tooltip anzeigen
    Below,      // Unter dem Feld
    Panel,      // In separate Panel
    Popover,    // Als Popover
    Inline      // Inline unter dem Label
}

/// <summary>
/// Badge customization options
/// </summary>
public class BadgeCustomizationOptions
{
    public bool ShowBadges { get; set; } = true;
    public bool ShowBadgeLabels { get; set; } = true;
    public bool ShowBadgeConfidence { get; set; } = true;
    public double BadgeOpacity { get; set; } = 0.8;
    public double BadgeFontSize { get; set; } = 11.0;
    public bool AnimateBadges { get; set; } = true;
    public bool ShowBadgeTooltips { get; set; } = true;
    public bool AllowBadgeDismissal { get; set; } = false;
    public bool AllowBadgeCustomization { get; set; } = true;
    public BadgeSortOption BadgeSortOption { get; set; } = BadgeSortOption.Confidence;
    public int MaxBadgesDisplayed { get; set; } = 5;
}

/// <summary>
/// Badge sort options
/// </summary>
public enum BadgeSortOption
{
    Confidence,
    Type,
    Alphabetical,
    Recent
}

/// <summary>
/// Form behavior options
/// </summary>
public class FormBehaviorOptions
{
    public bool EnableAutoSave { get; set; } = true;
    public int AutoSaveIntervalSeconds { get; set; } = 30;
    public bool ShowUnsavedIndicator { get; set; } = true;
    public bool ConfirmOnExit { get; set; } = true;
    public bool EnableUndoRedo { get; set; } = true;
    public bool EnableProgressBar { get; set; } = true;
    public bool ShowFieldCompletion { get; set; } = true;
    public bool EnableFieldNavigationByTab { get; set; } = true;
    public bool EnableFieldNavigationByArrows { get; set; } = false;
    public bool EnableFormPrefill { get; set; } = true;
    public bool EnableFieldAutoFill { get; set; } = true;
    public bool EnableSmartSuggestions { get; set; } = true;
    public bool EnableMultipleSubmissions { get; set; } = false;
    public bool ShowResetButton { get; set; } = true;
    public bool ShowPrintButton { get; set; } = false;
    public bool EnableKeyboardShortcuts { get; set; } = true;
}

/// <summary>
/// Service interface for form UI customization
/// </summary>
public interface IFormUICustomizationService
{
    // Customization management
    Task<FormUICustomization> GetFormUICustomizationAsync(string formId, CancellationToken cancellationToken = default);
    Task<FormUICustomization> CreateUICustomizationAsync(FormUICustomization customization, CancellationToken cancellationToken = default);
    Task<FormUICustomization> UpdateUICustomizationAsync(FormUICustomization customization, CancellationToken cancellationToken = default);
    Task<bool> DeleteUICustomizationAsync(string formId, CancellationToken cancellationToken = default);
    
    // Theme management
    Task<SmartFormTheme> GetThemeAsync(string formId, CancellationToken cancellationToken = default);
    Task UpdateThemeAsync(string formId, SmartFormTheme theme, CancellationToken cancellationToken = default);
    Task<List<SmartFormTheme>> GetAvailableThemesAsync(CancellationToken cancellationToken = default);
    Task ApplyThemeAsync(string formId, string themeName, CancellationToken cancellationToken = default);
    
    // Field rendering
    Task<FieldRenderingOptions> GetFieldRenderingAsync(string formId, CancellationToken cancellationToken = default);
    Task UpdateFieldRenderingAsync(string formId, FieldRenderingOptions options, CancellationToken cancellationToken = default);
    
    // Validation display
    Task<ValidationDisplayOptions> GetValidationDisplayAsync(string formId, CancellationToken cancellationToken = default);
    Task UpdateValidationDisplayAsync(string formId, ValidationDisplayOptions options, CancellationToken cancellationToken = default);
    
    // Help display
    Task<HelpDisplayOptions> GetHelpDisplayAsync(string formId, CancellationToken cancellationToken = default);
    Task UpdateHelpDisplayAsync(string formId, HelpDisplayOptions options, CancellationToken cancellationToken = default);
    
    // Badge customization
    Task<BadgeCustomizationOptions> GetBadgeCustomizationAsync(string formId, CancellationToken cancellationToken = default);
    Task UpdateBadgeCustomizationAsync(string formId, BadgeCustomizationOptions options, CancellationToken cancellationToken = default);
    
    // Form behavior
    Task<FormBehaviorOptions> GetFormBehaviorAsync(string formId, CancellationToken cancellationToken = default);
    Task UpdateFormBehaviorAsync(string formId, FormBehaviorOptions options, CancellationToken cancellationToken = default);
    
    // Preset configurations
    Task ApplyPresetConfigurationAsync(string formId, string presetName, CancellationToken cancellationToken = default);
    Task<List<string>> GetAvailablePresetsAsync(CancellationToken cancellationToken = default);
    
    // Export/Import
    Task<string> ExportUICustomizationAsJsonAsync(string formId, CancellationToken cancellationToken = default);
    Task ImportUICustomizationFromJsonAsync(string formId, string jsonConfig, CancellationToken cancellationToken = default);
    
    // Responsive design
    Task<FormUICustomization> GetResponsiveCustomizationAsync(string formId, int screenWidth, CancellationToken cancellationToken = default);
}

/// <summary>
/// Implementation of form UI customization service
/// </summary>
public class FormUICustomizationService : IFormUICustomizationService
{
    private readonly Dictionary<string, FormUICustomization> _customizations = new();
    private readonly Dictionary<string, SmartFormTheme> _availableThemes = new();
    private readonly Dictionary<string, FormUICustomization> _presets = new();
    private readonly object _lockObject = new();

    public FormUICustomizationService()
    {
        InitializeDefaultThemes();
        InitializePresets();
    }

    private void InitializeDefaultThemes()
    {
        lock (_lockObject)
        {
            _availableThemes["Light"] = new SmartFormTheme
            {
                ThemeName = "Light",
                PrimaryColor = "#2196F3",
                BackgroundColor = "#FFFFFF",
                TextColor = "#212121",
                BorderColor = "#BDBDBD"
            };

            _availableThemes["Dark"] = new SmartFormTheme
            {
                ThemeName = "Dark",
                PrimaryColor = "#1976D2",
                BackgroundColor = "#212121",
                TextColor = "#FFFFFF",
                BorderColor = "#424242",
                UseDarkMode = true
            };

            _availableThemes["Compact"] = new SmartFormTheme
            {
                ThemeName = "Compact",
                FontSize = 10.0,
                BorderRadius = 2.0,
                UseCompactLayout = true
            };

            _availableThemes["Modern"] = new SmartFormTheme
            {
                ThemeName = "Modern",
                BorderRadius = 8.0,
                BorderThickness = 2.0,
                PrimaryColor = "#6200EE",
                SecondaryColor = "#03DAC6"
            };
        }
    }

    private void InitializePresets()
    {
        lock (_lockObject)
        {
            // Preset für minimale UI
            _presets["Minimal"] = new FormUICustomization
            {
                FieldRendering = new FieldRenderingOptions
                {
                    ShowFieldLabels = true,
                    ShowFieldDescriptions = false,
                    ShowPlaceholderText = true,
                    ShowFieldBorders = false,
                    CompactFieldDisplay = true
                },
                ValidationDisplay = new ValidationDisplayOptions
                {
                    ShowErrorMessages = true,
                    ShowWarningMessages = false
                }
            };

            // Preset für vollständige UI
            _presets["Full"] = new FormUICustomization
            {
                FieldRendering = new FieldRenderingOptions
                {
                    ShowFieldLabels = true,
                    ShowFieldNumbers = true,
                    ShowFieldDescriptions = true,
                    ShowPlaceholderText = true,
                    ShowFieldBorders = true,
                    CompactFieldDisplay = false
                },
                ValidationDisplay = new ValidationDisplayOptions
                {
                    ShowErrorMessages = true,
                    ShowWarningMessages = true,
                    ShowValidationIcons = true,
                    ShowValidationSummary = true
                }
            };

            // Preset für Assistenten-UI
            _presets["Assistant"] = new FormUICustomization
            {
                HelpDisplay = new HelpDisplayOptions
                {
                    ShowHelpText = true,
                    ShowHelpIcons = true,
                    ShowExamples = true,
                    EnableFieldCopilot = true
                },
                BadgeCustomization = new BadgeCustomizationOptions
                {
                    ShowBadges = true,
                    ShowBadgeConfidence = true,
                    AnimateBadges = true
                },
                FormBehavior = new FormBehaviorOptions
                {
                    EnableAutoSave = true,
                    EnableSmartSuggestions = true
                }
            };
        }
    }

    public Task<FormUICustomization> GetFormUICustomizationAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                return Task.FromResult(customization);
            }
            
            var defaultCustomization = new FormUICustomization { FormId = formId };
            return Task.FromResult(defaultCustomization);
        }
    }

    public Task<FormUICustomization> CreateUICustomizationAsync(FormUICustomization customization, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            customization.CreatedAt = DateTime.UtcNow;
            customization.ModifiedAt = DateTime.UtcNow;
            _customizations[customization.FormId] = customization;
            return Task.FromResult(customization);
        }
    }

    public Task<FormUICustomization> UpdateUICustomizationAsync(FormUICustomization customization, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            customization.ModifiedAt = DateTime.UtcNow;
            _customizations[customization.FormId] = customization;
            return Task.FromResult(customization);
        }
    }

    public Task<bool> DeleteUICustomizationAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            return Task.FromResult(_customizations.Remove(formId));
        }
    }

    public Task<SmartFormTheme> GetThemeAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                return Task.FromResult(customization.Theme);
            }
            
            return Task.FromResult(new SmartFormTheme());
        }
    }

    public Task UpdateThemeAsync(string formId, SmartFormTheme theme, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (!_customizations.ContainsKey(formId))
            {
                _customizations[formId] = new FormUICustomization { FormId = formId };
            }
            
            _customizations[formId].Theme = theme;
            _customizations[formId].ModifiedAt = DateTime.UtcNow;
            return Task.CompletedTask;
        }
    }

    public Task<List<SmartFormTheme>> GetAvailableThemesAsync(CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            return Task.FromResult(_availableThemes.Values.ToList());
        }
    }

    public Task ApplyThemeAsync(string formId, string themeName, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_availableThemes.TryGetValue(themeName, out var theme))
            {
                if (!_customizations.ContainsKey(formId))
                {
                    _customizations[formId] = new FormUICustomization { FormId = formId };
                }
                
                _customizations[formId].Theme = theme;
                _customizations[formId].ModifiedAt = DateTime.UtcNow;
            }
            
            return Task.CompletedTask;
        }
    }

    public Task<FieldRenderingOptions> GetFieldRenderingAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                return Task.FromResult(customization.FieldRendering);
            }
            
            return Task.FromResult(new FieldRenderingOptions());
        }
    }

    public Task UpdateFieldRenderingAsync(string formId, FieldRenderingOptions options, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (!_customizations.ContainsKey(formId))
            {
                _customizations[formId] = new FormUICustomization { FormId = formId };
            }
            
            _customizations[formId].FieldRendering = options;
            _customizations[formId].ModifiedAt = DateTime.UtcNow;
            return Task.CompletedTask;
        }
    }

    public Task<ValidationDisplayOptions> GetValidationDisplayAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                return Task.FromResult(customization.ValidationDisplay);
            }
            
            return Task.FromResult(new ValidationDisplayOptions());
        }
    }

    public Task UpdateValidationDisplayAsync(string formId, ValidationDisplayOptions options, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (!_customizations.ContainsKey(formId))
            {
                _customizations[formId] = new FormUICustomization { FormId = formId };
            }
            
            _customizations[formId].ValidationDisplay = options;
            _customizations[formId].ModifiedAt = DateTime.UtcNow;
            return Task.CompletedTask;
        }
    }

    public Task<HelpDisplayOptions> GetHelpDisplayAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                return Task.FromResult(customization.HelpDisplay);
            }
            
            return Task.FromResult(new HelpDisplayOptions());
        }
    }

    public Task UpdateHelpDisplayAsync(string formId, HelpDisplayOptions options, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (!_customizations.ContainsKey(formId))
            {
                _customizations[formId] = new FormUICustomization { FormId = formId };
            }
            
            _customizations[formId].HelpDisplay = options;
            _customizations[formId].ModifiedAt = DateTime.UtcNow;
            return Task.CompletedTask;
        }
    }

    public Task<BadgeCustomizationOptions> GetBadgeCustomizationAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                return Task.FromResult(customization.BadgeCustomization);
            }
            
            return Task.FromResult(new BadgeCustomizationOptions());
        }
    }

    public Task UpdateBadgeCustomizationAsync(string formId, BadgeCustomizationOptions options, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (!_customizations.ContainsKey(formId))
            {
                _customizations[formId] = new FormUICustomization { FormId = formId };
            }
            
            _customizations[formId].BadgeCustomization = options;
            _customizations[formId].ModifiedAt = DateTime.UtcNow;
            return Task.CompletedTask;
        }
    }

    public Task<FormBehaviorOptions> GetFormBehaviorAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                return Task.FromResult(customization.FormBehavior);
            }
            
            return Task.FromResult(new FormBehaviorOptions());
        }
    }

    public Task UpdateFormBehaviorAsync(string formId, FormBehaviorOptions options, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (!_customizations.ContainsKey(formId))
            {
                _customizations[formId] = new FormUICustomization { FormId = formId };
            }
            
            _customizations[formId].FormBehavior = options;
            _customizations[formId].ModifiedAt = DateTime.UtcNow;
            return Task.CompletedTask;
        }
    }

    public Task ApplyPresetConfigurationAsync(string formId, string presetName, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_presets.TryGetValue(presetName, out var preset))
            {
                var customization = new FormUICustomization
                {
                    FormId = formId,
                    Theme = preset.Theme,
                    FieldRendering = preset.FieldRendering,
                    ValidationDisplay = preset.ValidationDisplay,
                    HelpDisplay = preset.HelpDisplay,
                    BadgeCustomization = preset.BadgeCustomization,
                    FormBehavior = preset.FormBehavior
                };
                
                _customizations[formId] = customization;
            }
            
            return Task.CompletedTask;
        }
    }

    public Task<List<string>> GetAvailablePresetsAsync(CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            return Task.FromResult(_presets.Keys.ToList());
        }
    }

    public Task<string> ExportUICustomizationAsJsonAsync(string formId, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                var json = System.Text.Json.JsonSerializer.Serialize(customization, new System.Text.Json.JsonSerializerOptions 
                { 
                    WriteIndented = true 
                });
                return Task.FromResult(json);
            }
            
            return Task.FromResult(string.Empty);
        }
    }

    public Task ImportUICustomizationFromJsonAsync(string formId, string jsonConfig, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            try
            {
                var customization = System.Text.Json.JsonSerializer.Deserialize<FormUICustomization>(jsonConfig);
                if (customization != null)
                {
                    customization.FormId = formId;
                    customization.ModifiedAt = DateTime.UtcNow;
                    _customizations[formId] = customization;
                }
            }
            catch
            {
                // JSON parsing failed
            }
            
            return Task.CompletedTask;
        }
    }

    public Task<FormUICustomization> GetResponsiveCustomizationAsync(string formId, int screenWidth, CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            if (_customizations.TryGetValue(formId, out var customization))
            {
                var responsiveCustomization = new FormUICustomization
                {
                    FormId = customization.FormId,
                    Theme = customization.Theme,
                    ValidationDisplay = customization.ValidationDisplay,
                    HelpDisplay = customization.HelpDisplay,
                    BadgeCustomization = customization.BadgeCustomization,
                    FormBehavior = customization.FormBehavior
                };

                // Responsive Anpassungen basierend auf Bildschirmbreite
                responsiveCustomization.FieldRendering = new FieldRenderingOptions
                {
                    ShowFieldLabels = customization.FieldRendering.ShowFieldLabels,
                    ShowFieldNumbers = screenWidth > 1200 ? customization.FieldRendering.ShowFieldNumbers : false,
                    ShowFieldDescriptions = screenWidth > 992 ? customization.FieldRendering.ShowFieldDescriptions : false,
                    LabelPosition = screenWidth > 768 ? customization.FieldRendering.LabelPosition : FieldLabelPosition.Above,
                    ColumnsCount = screenWidth > 1200 ? 2 : (screenWidth > 768 ? 1 : 1),
                    CompactFieldDisplay = screenWidth < 768
                };

                return Task.FromResult(responsiveCustomization);
            }
            
            return Task.FromResult(new FormUICustomization { FormId = formId });
        }
    }
}
