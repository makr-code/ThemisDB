using System;
using System.Windows;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Views.Dashboard;
using Themis.DocumentManager.Views.Windows;
using System.Windows.Input;
using System.Windows.Controls;
using Themis.DocumentManager.ViewModels.Favorites;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Views;

public partial class MainWindow : Window
{
    public static readonly RoutedUICommand FocusSearchCommand = new("FocusSearch", "FocusSearch", typeof(MainWindow));
    public static readonly RoutedUICommand CloseTabCommand = new("CloseTab", "CloseTab", typeof(MainWindow));
    public static readonly RoutedUICommand NewTabCommand = new("NewTab", "NewTab", typeof(MainWindow));
    public static readonly RoutedUICommand DuplicateTabCommand = new("DuplicateTab", "DuplicateTab", typeof(MainWindow));
    public static readonly RoutedUICommand ToggleLeftSidebarCommand = new("ToggleLeftSidebar", "ToggleLeftSidebar", typeof(MainWindow));
    public static readonly RoutedUICommand ToggleRightSidebarCommand = new("ToggleRightSidebar", "ToggleRightSidebar", typeof(MainWindow));

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
            CommandBindings.Add(new CommandBinding(CloseTabCommand, CloseTabCommand_Executed));
            CommandBindings.Add(new CommandBinding(NewTabCommand, NewTabCommand_Executed));
            CommandBindings.Add(new CommandBinding(DuplicateTabCommand, DuplicateTabCommand_Executed));
            CommandBindings.Add(new CommandBinding(ToggleLeftSidebarCommand, ToggleLeftSidebarCommand_Executed));
            CommandBindings.Add(new CommandBinding(ToggleRightSidebarCommand, ToggleRightSidebarCommand_Executed));

            Loaded += (_, __) => RestoreTabs();
            Loaded += (_, __) => RestoreSidebars();
            Loaded += (_, __) => RestoreSidebarTabs();
            Loaded += async (_, __) => await WireLeftPanelsAsync();
            Loaded += async (_, __) => await WireRightPanelsAsync();
            Closing += (_, __) => SaveTabs();
            Closing += (_, __) => SaveSidebars();
            Closing += (_, __) => SaveSidebarTabs();
        }
        catch (Exception ex)
        {
            Console.WriteLine($"MainWindow Error: {ex}");
            throw;
        }
    }
    private async Task WireLeftPanelsAsync()
    {
        try
        {
            // Explorer: bind DocumentBrowserViewModel
            var docVm = App.GetService<DocumentBrowserViewModel>();
            if (docVm != null)
            {
                LeftExplorerPanel.DataContext = docVm;
                await docVm.LoadDocumentsAsync();
            }

            // Favorites: bind FavoritesViewModel
            var favVm = App.GetService<FavoritesViewModel>();
            if (favVm != null)
            {
                LeftFavoritesPanel.DataContext = favVm;
                await favVm.LoadFavoritesAsync();
            }

            // Processes: bind ProcessLinkingDialogViewModel
            var processVm = App.GetService<ProcessLinkingDialogViewModel>();
            if (processVm != null && LeftProcessesPanel != null)
            {
                LeftProcessesPanel.DataContext = processVm;
                // Initialize with a default context (can be updated when a document is selected)
                await processVm.InitializeAsync("default", "Document");
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Verdrahten der Seitenleisten: {ex.Message}";
        }
    }

    private async Task WireRightPanelsAsync()
    {
        try
        {
            // Metadata: bind MetadataFormViewModel
            var metadataVm = App.GetService<MetadataFormViewModel>();
            if (metadataVm != null && RightMetadataPanel != null)
            {
                RightMetadataPanel.DataContext = metadataVm;
                // Initialize with sample document metadata
                await metadataVm.InitializeAsync("current-document", "Document");
            }

            // Revisions: bind to IRevisionService
            var revisionService = App.GetService<IRevisionService>();
            if (revisionService != null && RightRevisionPanel != null)
            {
                // Create a simple ListBox to display revisions
                var revisionsListBox = new ListBox { ItemsSource = new System.Collections.ObjectModel.ObservableCollection<DocumentRevision>() };
                RightRevisionPanel.Children.Add(revisionsListBox);
            }

            // Comments: bind DocumentCollaborationViewModel
            var collaborationVm = App.GetService<DocumentCollaborationViewModel>();
            if (collaborationVm != null && RightCommentsPanel != null)
            {
                RightCommentsPanel.DataContext = collaborationVm;
                // Initialize collaboration view
                var authService = App.GetService<IAuthenticationService>();
                var userId = authService?.CurrentUserId ?? "anonymous";
                var userName = authService?.CurrentUserName ?? "User";
                await collaborationVm.InitializeAsync("current-document-id", userId, userName);
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Verdrahten der rechten Seitenleiste: {ex.Message}";
        }
    }

    private void SaveTabs()
    {
        try
        {
            var list = new List<string>();
            foreach (var item in CenterContent.Items)
            {
                if (item is TabItem ti && ti.Visibility == Visibility.Visible)
                {
                    var id = GetContentId(ti.Content);
                    if (!string.IsNullOrEmpty(id)) list.Add(id);
                }
            }
            _settingsService.SetSetting("OpenTabs", string.Join("|", list));
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Speichern der Tabs: {ex.Message}";
        }
    }

    private void SaveSidebars()
    {
        try
        {
            _settingsService.SetSetting("LeftSidebarWidth", (int)ColLeft.Width.Value);
            _settingsService.SetSetting("RightSidebarWidth", (int)ColRight.Width.Value);
            _settingsService.SetSetting("LeftSidebarVisible", ColLeft.Width.Value > 0);
            _settingsService.SetSetting("RightSidebarVisible", ColRight.Width.Value > 0);
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Speichern der Seitenleisten: {ex.Message}";
        }
    }

    private void RestoreSidebars()
    {
        try
        {
            var leftVisible = _settingsService.GetSetting("LeftSidebarVisible", true);
            var rightVisible = _settingsService.GetSetting("RightSidebarVisible", true);
            var leftWidth = _settingsService.GetSetting("LeftSidebarWidth", 280);
            var rightWidth = _settingsService.GetSetting("RightSidebarWidth", 320);

            ColLeft.Width = new GridLength(leftVisible ? leftWidth : 0);
            ColRight.Width = new GridLength(rightVisible ? rightWidth : 0);
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Wiederherstellen der Seitenleisten: {ex.Message}";
        }
    }

    private void RestoreTabs()
    {
        try
        {
            var raw = _settingsService.GetSetting("OpenTabs", string.Empty);
            if (string.IsNullOrWhiteSpace(raw)) return;
            var ids = raw.Split('|');
            // Start-Tab bleibt bestehen; weitere je ID hinzufügen
            for (int i = 1; i < ids.Length; i++)
            {
                var content = CreateContentById(ids[i]);
                if (content == null) continue;
                var header = GetHeaderById(ids[i]);
                var tab = new TabItem { Header = header, Content = content, Visibility = Visibility.Visible };
                CenterContent.Items.Add(tab);
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Wiederherstellen der Tabs: {ex.Message}";
        }
    }

    private void SaveSidebarTabs()
    {
        try
        {
            var leftTabIndex = LeftSidebarTabs?.SelectedIndex ?? 0;
            var rightTabIndex = RightSidebarTabs?.SelectedIndex ?? 0;
            var centerTabIndex = CenterContent?.SelectedIndex ?? 0;
            
            _settingsService.SetSetting("LeftSidebarTabIndex", leftTabIndex);
            _settingsService.SetSetting("RightSidebarTabIndex", rightTabIndex);
            _settingsService.SetSetting("CenterTabIndex", centerTabIndex);
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Speichern der Tab-Indizes: {ex.Message}";
        }
    }

    private void RestoreSidebarTabs()
    {
        try
        {
            var leftTabIndex = _settingsService.GetSetting("LeftSidebarTabIndex", 0);
            var rightTabIndex = _settingsService.GetSetting("RightSidebarTabIndex", 0);
            var centerTabIndex = _settingsService.GetSetting("CenterTabIndex", 0);
            
            if (LeftSidebarTabs != null && leftTabIndex >= 0 && leftTabIndex < LeftSidebarTabs.Items.Count)
            {
                LeftSidebarTabs.SelectedIndex = leftTabIndex;
            }
            
            if (RightSidebarTabs != null && rightTabIndex >= 0 && rightTabIndex < RightSidebarTabs.Items.Count)
            {
                RightSidebarTabs.SelectedIndex = rightTabIndex;
            }
            
            if (CenterContent != null && centerTabIndex >= 0 && centerTabIndex < CenterContent.Items.Count)
            {
                CenterContent.SelectedIndex = centerTabIndex;
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Wiederherstellen der Tab-Indizes: {ex.Message}";
        }
    }

    private static string GetContentId(object? content)
    {
        return content switch
        {
            DocumentBrowserSimpleView => "documents",
            TimelineSimpleView => "timeline",
            FullDashboardSimpleView => "dashboard",
            DashboardPreviewView => "start",
            _ => string.Empty
        };
    }

    private static object? CreateContentById(string id)
    {
        return id switch
        {
            "documents" => new DocumentBrowserSimpleView(),
            "timeline" => new TimelineSimpleView(),
            "dashboard" => new FullDashboardSimpleView(),
            "start" => new DashboardPreviewView(),
            _ => null
        };
    }

    private static string GetHeaderById(string id)
    {
        return id switch
        {
            "documents" => "📑 Documents",
            "timeline" => "🔗 Timeline",
            "dashboard" => "📊 Dashboard",
            "start" => "📊 Start",
            _ => "Tab"
        };
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

    private void CloseTabCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            if (CenterContent.SelectedItem is TabItem tab)
            {
                // Nicht entfernen, nur ausblenden; Start-Tab bleibt immer sichtbar
                if (tab.Name == "TabStart") return;
                tab.Visibility = Visibility.Collapsed;

                // Auf nächsten sichtbaren Tab wechseln
                for (int i = 0; i < CenterContent.Items.Count; i++)
                {
                    var item = CenterContent.Items[i] as TabItem;
                    if (item?.Visibility == Visibility.Visible)
                    {
                        CenterContent.SelectedItem = item;
                        break;
                    }
                }
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Schließen des Tabs: {ex.Message}";
        }
    }

    private void NewTabCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            var content = App.GetService<DocumentBrowserSimpleView>() ?? new DocumentBrowserSimpleView();
            var tab = new TabItem { Header = "📑 Documents", Content = content, Visibility = Visibility.Visible };
            CenterContent.Items.Add(tab);
            CenterContent.SelectedItem = tab;
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Öffnen eines neuen Tabs: {ex.Message}";
        }
    }

    private void DuplicateTabCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            if (CenterContent.SelectedItem is TabItem current)
            {
                // Duplizieren: gleicher Header, neuer Content je nach Typ
                object? newContent = CloneContentViaDi(current.Content);

                var tab = new TabItem
                {
                    Header = current.Header,
                    Content = newContent ?? new DocumentBrowserSimpleView(),
                    Visibility = Visibility.Visible
                };
                CenterContent.Items.Add(tab);
                CenterContent.SelectedItem = tab;
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Duplizieren des Tabs: {ex.Message}";
        }
    }

    private void ToggleLeftSidebarCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            var isCollapsed = ColLeft.Width.Value < 1;
            if (isCollapsed)
            {
                var width = _settingsService.GetSetting("LeftSidebarWidth", 280);
                ColLeft.Width = new GridLength(width);
                _settingsService.SetSetting("LeftSidebarVisible", true);
            }
            else
            {
                _settingsService.SetSetting("LeftSidebarWidth", (int)ColLeft.Width.Value);
                ColLeft.Width = new GridLength(0);
                _settingsService.SetSetting("LeftSidebarVisible", false);
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Umschalten der linken Seitenleiste: {ex.Message}";
        }
    }

    private void ToggleRightSidebarCommand_Executed(object sender, System.Windows.Input.ExecutedRoutedEventArgs e)
    {
        try
        {
            var isCollapsed = ColRight.Width.Value < 1;
            if (isCollapsed)
            {
                var width = _settingsService.GetSetting("RightSidebarWidth", 320);
                ColRight.Width = new GridLength(width);
                _settingsService.SetSetting("RightSidebarVisible", true);
            }
            else
            {
                _settingsService.SetSetting("RightSidebarWidth", (int)ColRight.Width.Value);
                ColRight.Width = new GridLength(0);
                _settingsService.SetSetting("RightSidebarVisible", false);
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Umschalten der rechten Seitenleiste: {ex.Message}";
        }
    }

    private void MenuOpenTabInNewWindow_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (CenterContent.SelectedItem is TabItem current)
            {
                var win = new Window
                {
                    Title = current.Header?.ToString() ?? "ThemisDB",
                    Width = 1200,
                    Height = 800,
                    WindowStartupLocation = WindowStartupLocation.CenterOwner,
                    Owner = this,
                    Content = CloneContent(current.Content)
                };
                win.Show();
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Öffnen in neuem Fenster: {ex.Message}";
        }
    }

    private static object? CloneContent(object? content)
    {
        if (content is DocumentBrowserSimpleView)
            return App.GetService<DocumentBrowserSimpleView>() ?? new DocumentBrowserSimpleView();
        if (content is TimelineSimpleView)
            return App.GetService<TimelineSimpleView>() ?? new TimelineSimpleView();
        if (content is FullDashboardSimpleView)
            return App.GetService<FullDashboardSimpleView>() ?? new FullDashboardSimpleView();
        if (content is DashboardPreviewView)
            return App.GetService<DashboardPreviewView>() ?? new DashboardPreviewView();
        return null;
    }

    private static object? CloneContentViaDi(object? content)
    {
        return content switch
        {
            DocumentBrowserSimpleView => App.GetService<DocumentBrowserSimpleView>() ?? new DocumentBrowserSimpleView(),
            TimelineSimpleView => App.GetService<TimelineSimpleView>() ?? new TimelineSimpleView(),
            FullDashboardSimpleView => App.GetService<FullDashboardSimpleView>() ?? new FullDashboardSimpleView(),
            DashboardPreviewView => App.GetService<DashboardPreviewView>() ?? new DashboardPreviewView(),
            _ => null
        };
    }
}
