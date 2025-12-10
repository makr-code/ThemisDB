using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Animation;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Views.Settings;

/// <summary>
/// Settings Dialog für Theme-Management, Allgemeine Einstellungen, Performance und Accessibility.
/// Phase 28 - Settings & Persistence System.
/// </summary>
public partial class SettingsDialog : Window
{
    private readonly IThemeService _themeService;
    private readonly ISettingsService _settingsService;
    private readonly IAnimationService _animationService;

    private bool _hasChanges = false;

    public SettingsDialog(
        IThemeService themeService,
        ISettingsService settingsService,
        IAnimationService animationService)
    {
        _themeService = themeService ?? throw new ArgumentNullException(nameof(themeService));
        _settingsService = settingsService ?? throw new ArgumentNullException(nameof(settingsService));
        _animationService = animationService ?? throw new ArgumentNullException(nameof(animationService));

        InitializeComponent();
        LoadSettings();
        
        // Apply initial fade-in animation
        Opacity = 0;
        Loaded += (s, e) => _animationService.FadeIn(this, 300);
    }

    /// <summary>
    /// Lädt gespeicherte Einstellungen in die UI
    /// </summary>
    private void LoadSettings()
    {
        try
        {
            // Theme Settings
            var themeMode = _themeService.CurrentTheme;
            switch (themeMode)
            {
                case ThemeService.ThemeMode.Light:
                    LightModeRadio.IsChecked = true;
                    break;
                case ThemeService.ThemeMode.Dark:
                    DarkModeRadio.IsChecked = true;
                    break;
                case ThemeService.ThemeMode.System:
                    SystemModeRadio.IsChecked = true;
                    break;
            }

            HighContrastCheckBox.IsChecked = _themeService.IsHighContrast;

            // Performance Settings
            EnableAnimationsCheckBox.IsChecked = _settingsService.GetSetting("EnableAnimations", true);
            CacheSizeSlider.Value = _settingsService.GetSetting("CacheSize", 500);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Fehler beim Laden der Einstellungen: {ex.Message}",
                          "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    /// <summary>
    /// Speichert Einstellungen
    /// </summary>
    private void SaveSettings()
    {
        try
        {
            // Theme Settings
            if (LightModeRadio.IsChecked == true)
                _themeService.CurrentTheme = ThemeService.ThemeMode.Light;
            else if (DarkModeRadio.IsChecked == true)
                _themeService.CurrentTheme = ThemeService.ThemeMode.Dark;
            else
                _themeService.CurrentTheme = ThemeService.ThemeMode.System;

            _themeService.IsHighContrast = HighContrastCheckBox.IsChecked ?? false;
            _themeService.SaveThemeSetting();

            // Performance Settings
            _settingsService.SetSetting("EnableAnimations", EnableAnimationsCheckBox.IsChecked ?? true);
            _settingsService.SetSetting("CacheSize", (int)CacheSizeSlider.Value);

            _settingsService.Save();
            _hasChanges = false;
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Fehler beim Speichern der Einstellungen: {ex.Message}",
                          "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    #region Navigation Handlers

    private void ThemeSettings_Click(object sender, RoutedEventArgs e)
    {
        ShowPanel(ThemeSettingsPanel);
    }

    private void GeneralSettings_Click(object sender, RoutedEventArgs e)
    {
        ShowPanel(GeneralSettingsPanel);
    }

    private void PerformanceSettings_Click(object sender, RoutedEventArgs e)
    {
        ShowPanel(PerformanceSettingsPanel);
    }

    private void AccessibilitySettings_Click(object sender, RoutedEventArgs e)
    {
        ShowPanel(AccessibilitySettingsPanel);
    }

    private void ShowPanel(FrameworkElement targetPanel)
    {
        // Hide all panels
        ThemeSettingsPanel.Visibility = Visibility.Collapsed;
        GeneralSettingsPanel.Visibility = Visibility.Collapsed;
        PerformanceSettingsPanel.Visibility = Visibility.Collapsed;
        AccessibilitySettingsPanel.Visibility = Visibility.Collapsed;

        // Show target panel with animation
        targetPanel.Visibility = Visibility.Visible;
        
        if (EnableAnimationsCheckBox.IsChecked == true)
        {
            _animationService.SlideIn(targetPanel, 300);
        }
    }

    #endregion

    #region Theme Settings Handlers

    private void ThemeMode_Changed(object sender, RoutedEventArgs e)
    {
        _hasChanges = true;
        
        // Live preview (optional)
        if (LightModeRadio.IsChecked == true)
            _themeService.CurrentTheme = ThemeService.ThemeMode.Light;
        else if (DarkModeRadio.IsChecked == true)
            _themeService.CurrentTheme = ThemeService.ThemeMode.Dark;
        else
            _themeService.CurrentTheme = ThemeService.ThemeMode.System;
    }

    private void HighContrast_Changed(object sender, RoutedEventArgs e)
    {
        _hasChanges = true;
        _themeService.IsHighContrast = HighContrastCheckBox.IsChecked ?? false;
    }

    private void ResetColors_Click(object sender, RoutedEventArgs e)
    {
        var result = MessageBox.Show(
            "Möchten Sie alle Farbeinstellungen auf die Standardwerte zurücksetzen?",
            "Bestätigung",
            MessageBoxButton.YesNo,
            MessageBoxImage.Question);

        if (result == MessageBoxResult.Yes)
        {
            // Reset to default colors
            _themeService.CurrentTheme = ThemeService.ThemeMode.System;
            SystemModeRadio.IsChecked = true;
            _hasChanges = true;
        }
    }

    #endregion

    #region Performance Settings Handlers

    private void EnableAnimations_Changed(object sender, RoutedEventArgs e)
    {
        _hasChanges = true;
        _animationService.IsEnabled = EnableAnimationsCheckBox.IsChecked ?? true;
    }

    #endregion

    #region Action Handlers

    private void Apply_Click(object sender, RoutedEventArgs e)
    {
        SaveSettings();
        DialogResult = true;
        Close();
    }

    private void Cancel_Click(object sender, RoutedEventArgs e)
    {
        if (_hasChanges)
        {
            var result = MessageBox.Show(
                "Sie haben ungespeicherte Änderungen. Möchten Sie wirklich abbrechen?",
                "Ungespeicherte Änderungen",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning);

            if (result == MessageBoxResult.No)
                return;
        }

        DialogResult = false;
        Close();
    }

    #endregion
}
