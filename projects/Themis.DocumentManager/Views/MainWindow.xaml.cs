using System;
using System.Windows;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Views.Dashboard;
using Themis.DocumentManager.Views.Windows;
using System.Windows.Input;

namespace Themis.DocumentManager.Views;

public partial class MainWindow : Window
{
    public static readonly RoutedUICommand FocusSearchCommand = new("FocusSearch", "FocusSearch", typeof(MainWindow));

    private readonly MainViewModel _viewModel;
    private readonly IThemeService _themeService;
    private readonly ISettingsService _settingsService;
    private readonly IAnimationService _animationService;

    public MainWindow(
        MainViewModel viewModel,
        IOfficeIntegrationService officeService,
        IThemeService themeService,
        ISettingsService settingsService,
        IAnimationService animationService,
        AIChatViewModel aiChatViewModel,
        StatusMonitorService statusMonitor,
        IFormTemplateService? formTemplateService = null)
    {
        try
        {
            InitializeComponent();
            _viewModel = viewModel;
            _themeService = themeService;
            _settingsService = settingsService;
            _animationService = animationService;
            DataContext = _viewModel;

            // Route VM command to settings dialog
            CommandBindings.Add(new CommandBinding(_viewModel.OpenSettingsCommand, OpenSettingsCommand_Executed));
            CommandBindings.Add(new CommandBinding(_viewModel.NextTabCommand, NextTabCommand_Executed));
            CommandBindings.Add(new CommandBinding(_viewModel.PreviousTabCommand, PreviousTabCommand_Executed));
            CommandBindings.Add(new CommandBinding(FocusSearchCommand, FocusSearchCommand_Executed));
        }
        catch (Exception ex)
        {
            Console.WriteLine($"MainWindow Error: {ex}");
            throw;
        }
    }

    /// <summary>
    /// Toggles between fullscreen and normal window state
    /// </summary>
    private void MenuFullscreen_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized;
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler: {ex.Message}";
        }
    }

    /// <summary>
    /// Opens the Audit Log Viewer window
    /// </summary>
    private void MenuAuditLogs_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var auditWindow = new AuditLogViewerWindow
            {
                Owner = this,
                WindowStartupLocation = WindowStartupLocation.CenterOwner
            };
            auditWindow.ShowDialog();
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Öffnen der Audit-Logs: {ex.Message}";
        }
    }

    /// <summary>
    /// Sets light theme
    /// </summary>
    private void MenuThemeLight_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            MenuThemeLight.IsChecked = true;
            MenuThemeDark.IsChecked = false;
            MenuThemeSystem.IsChecked = false;
            _themeService.CurrentTheme = ThemeService.ThemeMode.Light;
            _themeService.SaveThemeSetting();
            StatusText.Text = "Helles Theme aktiviert";
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Theme-Wechsel: {ex.Message}";
        }
    }

    /// <summary>
    /// Sets dark theme
    /// </summary>
    private void MenuThemeDark_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            MenuThemeLight.IsChecked = false;
            MenuThemeDark.IsChecked = true;
            MenuThemeSystem.IsChecked = false;
            _themeService.CurrentTheme = ThemeService.ThemeMode.Dark;
            _themeService.SaveThemeSetting();
            StatusText.Text = "Dunkles Theme aktiviert";
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Theme-Wechsel: {ex.Message}";
        }
    }

    /// <summary>
    /// Sets system theme
    /// </summary>
    private void MenuThemeSystem_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            MenuThemeLight.IsChecked = false;
            MenuThemeDark.IsChecked = false;
            MenuThemeSystem.IsChecked = true;
            _themeService.CurrentTheme = ThemeService.ThemeMode.System;
            _themeService.SaveThemeSetting();
            StatusText.Text = "System-Theme aktiviert";
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Theme-Wechsel: {ex.Message}";
        }
    }

    /// <summary>
    /// Öffnet den Settings-Dialog über DI
    /// </summary>
    private void OpenSettingsCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            var dlg = new Views.Settings.SettingsDialog(_themeService, _settingsService, _animationService)
            {
                Owner = this,
                WindowStartupLocation = WindowStartupLocation.CenterOwner
            };
            dlg.ShowDialog();
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Öffnen der Einstellungen: {ex.Message}";
        }
    }

    private void NextTabCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            if (CenterContent.Items.Count == 0) return;
            var nextIndex = (CenterContent.SelectedIndex + 1) % CenterContent.Items.Count;
            CenterContent.SelectedIndex = nextIndex;
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Wechseln des Tabs: {ex.Message}";
        }
    }

    private void PreviousTabCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            if (CenterContent.Items.Count == 0) return;
            var prevIndex = (CenterContent.SelectedIndex - 1 + CenterContent.Items.Count) % CenterContent.Items.Count;
            CenterContent.SelectedIndex = prevIndex;
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Wechseln des Tabs: {ex.Message}";
        }
    }

    private void FocusSearchCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            SearchBox.Focus();
            SearchBox.SelectAll();
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Fokussieren der Suche: {ex.Message}";
        }
    }

    /// <summary>
    /// Beendet die Anwendung
    /// </summary>
    private void MenuExit_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            System.Windows.Application.Current.Shutdown();
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Beenden: {ex.Message}";
        }
    }
}
