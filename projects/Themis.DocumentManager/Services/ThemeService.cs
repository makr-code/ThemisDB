/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemeService.cs                                    ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     217                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Windows;
using ModernWpf;
using ModernWpf.Controls;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service für Theme-Management und Light/Dark-Mode Handling.
/// Phase 27 - UI Styling & Theme System.
/// </summary>
public class ThemeService : IThemeService
{
    /// <summary>
    /// Theme-Modi für die Anwendung
    /// </summary>
    public enum ThemeMode
    {
        Light,
        Dark,
        System
    }

    private ThemeMode _currentTheme = ThemeMode.System;
    private bool _isHighContrast = false;
    private readonly ISettingsService? _settingsService;

    /// <summary>
    /// Event beim Theme-Wechsel
    /// </summary>
    public event EventHandler<ThemeChangedEventArgs>? ThemeChanged;

    public ThemeService(ISettingsService? settingsService = null)
    {
        _settingsService = settingsService;
    }

    /// <summary>
    /// Aktuelles Theme
    /// </summary>
    public ThemeMode CurrentTheme
    {
        get => _currentTheme;
        set
        {
            if (_currentTheme != value)
            {
                _currentTheme = value;
                ApplyTheme(value);
                OnThemeChanged(new ThemeChangedEventArgs(value, _isHighContrast));
            }
        }
    }

    /// <summary>
    /// High Contrast Mode
    /// </summary>
    public bool IsHighContrast
    {
        get => _isHighContrast;
        set
        {
            if (_isHighContrast != value)
            {
                _isHighContrast = value;
                OnThemeChanged(new ThemeChangedEventArgs(_currentTheme, value));
            }
        }
    }

    /// <summary>
    /// Initialisiert den ThemeService mit gespeicherten Einstellungen
    /// </summary>
    public void Initialize()
    {
        var savedTheme = LoadThemeSetting();
        ApplyTheme(savedTheme);
        _currentTheme = savedTheme;
    }

    /// <summary>
    /// Wendet das Theme auf die Anwendung an
    /// </summary>
    private void ApplyTheme(ThemeMode theme)
    {
        var isDark = theme == ThemeMode.Dark;
        
        try
        {
            ThemeManager.Current.ApplicationTheme = isDark ? ApplicationTheme.Dark : ApplicationTheme.Light;
            
            // Wende Custom Themis-Farben an
            ApplyCustomThemisColors(isDark);
        }
        catch (Exception ex)
        {
            // Fallback auf System-Theme
            System.Diagnostics.Debug.WriteLine($"Theme-Anwendung fehlgeschlagen: {ex.Message}");
        }
    }

    /// <summary>
    /// Wendet Custom Themis-Farben an
    /// </summary>
    private void ApplyCustomThemisColors(bool isDark)
    {
        var resources = System.Windows.Application.Current?.Resources;
        if (resources == null) return;
        
        if (isDark)
        {
            // Dark Mode Farben
            resources["ThemisPrimaryColor"] = System.Windows.Media.Color.FromRgb(66, 135, 245);      // Blau
            resources["ThemisSecondaryColor"] = System.Windows.Media.Color.FromRgb(106, 195, 245);   // Hellblau
            resources["ThemisAccentColor"] = System.Windows.Media.Color.FromRgb(255, 152, 0);        // Orange
            resources["ThemisBackgroundColor"] = System.Windows.Media.Color.FromRgb(32, 32, 32);     // Dunkelgrau
            resources["ThemisTextColor"] = System.Windows.Media.Color.FromRgb(230, 230, 230);        // Hellgrau
        }
        else
        {
            // Light Mode Farben
            resources["ThemisPrimaryColor"] = System.Windows.Media.Color.FromRgb(25, 103, 210);      // Dunkelblau
            resources["ThemisSecondaryColor"] = System.Windows.Media.Color.FromRgb(66, 135, 245);    // Blau
            resources["ThemisAccentColor"] = System.Windows.Media.Color.FromRgb(255, 87, 34);        // Rot-Orange
            resources["ThemisBackgroundColor"] = System.Windows.Media.Color.FromRgb(255, 255, 255); // Weiß
            resources["ThemisTextColor"] = System.Windows.Media.Color.FromRgb(33, 33, 33);           // Sehr Dunkelgrau
        }
    }

    /// <summary>
    /// Speichert die aktuelle Theme-Einstellung
    /// </summary>
    public void SaveThemeSetting()
    {
        if (_settingsService != null)
        {
            _settingsService.SetSetting("ThemeMode", _currentTheme.ToString());
            _settingsService.SetSetting("HighContrast", _isHighContrast);
            _settingsService.Save();
        }
        
        System.Diagnostics.Debug.WriteLine($"Theme gespeichert: {CurrentTheme}");
    }

    /// <summary>
    /// Ruft das Theme aus gespeicherten Einstellungen ab
    /// </summary>
    public ThemeMode LoadThemeSetting()
    {
        if (_settingsService != null)
        {
            var themeString = _settingsService.GetSetting("ThemeMode", ThemeMode.System.ToString());
            if (Enum.TryParse<ThemeMode>(themeString, out var theme))
            {
                _isHighContrast = _settingsService.GetSetting("HighContrast", false);
                return theme;
            }
        }
        
        return ThemeMode.System;
    }

    /// <summary>
    /// Event-Handler für Theme-Änderungen
    /// </summary>
    protected virtual void OnThemeChanged(ThemeChangedEventArgs e)
    {
        ThemeChanged?.Invoke(this, e);
    }
}

/// <summary>
/// Event-Arguments für Theme-Änderungen
/// </summary>
public class ThemeChangedEventArgs : EventArgs
{
    public ThemeService.ThemeMode Theme { get; }
    public bool IsHighContrast { get; }

    public ThemeChangedEventArgs(ThemeService.ThemeMode theme, bool isHighContrast)
    {
        Theme = theme;
        IsHighContrast = isHighContrast;
    }
}

/// <summary>
/// Interface für Theme-Service
/// </summary>
public interface IThemeService
{
    ThemeService.ThemeMode CurrentTheme { get; set; }
    bool IsHighContrast { get; set; }
    event EventHandler<ThemeChangedEventArgs>? ThemeChanged;
    void Initialize();
    void SaveThemeSetting();
    ThemeService.ThemeMode LoadThemeSetting();
}
