/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SettingsDialog.xaml.cs                             ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     355                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Animation;
using Themis.DocumentManager.Services;
using System.Windows.Input;

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

            // Shortcut Settings: prefill from current service
            var svc = KeyboardShortcutService.Instance;
            var conv = new KeyGestureConverter();
            ShortcutSaveMetadata.Text = conv.ConvertToString(svc.Shortcuts["SaveMetadata"]);
            ShortcutReloadMetadata.Text = conv.ConvertToString(svc.Shortcuts["ReloadMetadata"]);
            ShortcutFinalizeMetadata.Text = conv.ConvertToString(svc.Shortcuts["FinalizeMetadata"]);
            ShortcutOpenSettings.Text = conv.ConvertToString(svc.Shortcuts["OpenSettings"]);
                ShortcutCloseTab.Text = conv.ConvertToString(svc.Shortcuts["CloseTab"]);
                ShortcutDuplicateTab.Text = conv.ConvertToString(svc.Shortcuts["DuplicateTab"]);
                ShortcutSidebarGraph.Text = conv.ConvertToString(svc.Shortcuts["SwitchSidebarGraph"]);
                ShortcutSidebarMap.Text = conv.ConvertToString(svc.Shortcuts["SwitchSidebarMap"]);
            ShortcutOpenTabInNewWindow.Text = conv.ConvertToString(svc.Shortcuts["OpenTabInNewWindow"]);
            ShortcutFavoriteAdd.Text = conv.ConvertToString(svc.Shortcuts["FavoriteAdd"]);
            ShortcutFavoriteRemove.Text = conv.ConvertToString(svc.Shortcuts["FavoriteRemove"]);
                ShortcutCloseOthers.Text = conv.ConvertToString(svc.Shortcuts["CloseOthers"]);
                ShortcutSearchTabs.Text = conv.ConvertToString(svc.Shortcuts["SearchTabs"]);
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

            // Persist shortcut strings
            _settingsService.SetSetting("Shortcut.SaveMetadata", ShortcutSaveMetadata.Text);
            _settingsService.SetSetting("Shortcut.ReloadMetadata", ShortcutReloadMetadata.Text);
            _settingsService.SetSetting("Shortcut.FinalizeMetadata", ShortcutFinalizeMetadata.Text);
            _settingsService.SetSetting("Shortcut.OpenSettings", ShortcutOpenSettings.Text);
                _settingsService.SetSetting("Shortcut.CloseTab", ShortcutCloseTab.Text);
                _settingsService.SetSetting("Shortcut.DuplicateTab", ShortcutDuplicateTab.Text);
                _settingsService.SetSetting("Shortcut.SwitchSidebarGraph", ShortcutSidebarGraph.Text);
                _settingsService.SetSetting("Shortcut.SwitchSidebarMap", ShortcutSidebarMap.Text);
                _settingsService.SetSetting("Shortcut.OpenTabInNewWindow", ShortcutOpenTabInNewWindow.Text);
                _settingsService.SetSetting("Shortcut.FavoriteAdd", ShortcutFavoriteAdd.Text);
                _settingsService.SetSetting("Shortcut.FavoriteRemove", ShortcutFavoriteRemove.Text);
                _settingsService.SetSetting("Shortcut.CloseOthers", ShortcutCloseOthers.Text);
                _settingsService.SetSetting("Shortcut.SearchTabs", ShortcutSearchTabs.Text);

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

    private void TreeViewSettings_Click(object sender, RoutedEventArgs e)
    {
        ShowPanel(TreeViewSettingsPanel);
        
        // Load TreeViewSettingsView into ContentControl if not already loaded
        if (TreeViewSettingsContent.Content == null)
        {
            TreeViewSettingsContent.Content = new TreeViewSettingsView();
        }
    }

    private void ShowPanel(FrameworkElement targetPanel)
    {
        // Hide all panels
        ThemeSettingsPanel.Visibility = Visibility.Collapsed;
        GeneralSettingsPanel.Visibility = Visibility.Collapsed;
        PerformanceSettingsPanel.Visibility = Visibility.Collapsed;
        AccessibilitySettingsPanel.Visibility = Visibility.Collapsed;
        TreeViewSettingsPanel.Visibility = Visibility.Collapsed;

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

    #region Shortcut Settings

    private void SaveShortcuts_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var svc = KeyboardShortcutService.Instance;
            svc.UpdateShortcut("SaveMetadata", ParseGesture(ShortcutSaveMetadata.Text));
            svc.UpdateShortcut("ReloadMetadata", ParseGesture(ShortcutReloadMetadata.Text));
            svc.UpdateShortcut("FinalizeMetadata", ParseGesture(ShortcutFinalizeMetadata.Text));
            svc.UpdateShortcut("OpenSettings", ParseGesture(ShortcutOpenSettings.Text));
            svc.UpdateShortcut("CloseTab", ParseGesture(ShortcutCloseTab.Text));
            svc.UpdateShortcut("DuplicateTab", ParseGesture(ShortcutDuplicateTab.Text));
            svc.UpdateShortcut("SwitchSidebarGraph", ParseGesture(ShortcutSidebarGraph.Text));
            svc.UpdateShortcut("SwitchSidebarMap", ParseGesture(ShortcutSidebarMap.Text));
            svc.UpdateShortcut("OpenTabInNewWindow", ParseGesture(ShortcutOpenTabInNewWindow.Text));
            svc.UpdateShortcut("FavoriteAdd", ParseGesture(ShortcutFavoriteAdd.Text));
            svc.UpdateShortcut("FavoriteRemove", ParseGesture(ShortcutFavoriteRemove.Text));
            svc.UpdateShortcut("CloseOthers", ParseGesture(ShortcutCloseOthers.Text));
            svc.UpdateShortcut("SearchTabs", ParseGesture(ShortcutSearchTabs.Text));
            _hasChanges = false;
            MessageBox.Show("Shortcuts gespeichert.", "Erfolg", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Fehler beim Speichern der Shortcuts: {ex.Message}", "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }

    private void ResetShortcuts_Click(object sender, RoutedEventArgs e)
    {
        var svc = KeyboardShortcutService.Instance;
        svc.LoadDefaults();
        ShortcutSaveMetadata.Text = "Ctrl+S";
        ShortcutReloadMetadata.Text = "F5";
        ShortcutFinalizeMetadata.Text = "Ctrl+F";
        ShortcutOpenSettings.Text = "Ctrl+,";
            ShortcutCloseTab.Text = "Ctrl+W";
            ShortcutDuplicateTab.Text = "Ctrl+D";
            ShortcutSidebarGraph.Text = "Ctrl+Shift+G";
            ShortcutSidebarMap.Text = "Ctrl+Shift+M";
        ShortcutOpenTabInNewWindow.Text = "Ctrl+Shift+N";
        ShortcutFavoriteAdd.Text = "Ctrl+Shift+F";
        ShortcutFavoriteRemove.Text = "Ctrl+Shift+R";
        ShortcutCloseOthers.Text = "Ctrl+Shift+K";
        ShortcutSearchTabs.Text = "Ctrl+P";
        MessageBox.Show("Shortcuts zurückgesetzt.", "Info", MessageBoxButton.OK, MessageBoxImage.Information);
    }

    private KeyGesture ParseGesture(string text)
    {
        var converter = new KeyGestureConverter();
        var obj = converter.ConvertFromString(text);
        if (obj is KeyGesture kg)
            return kg;
        throw new InvalidOperationException("Ungültiger Shortcut: " + text);
    }

    #endregion
}
