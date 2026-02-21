/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainWindow.xaml.cs                                 ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     995                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

﻿using System;
using System.Windows;
using Themis.DocumentManager.ViewModels;
using Themis.DocumentManager.Features.DocumentBrowser.Views;
using Themis.DocumentManager.Features.DocumentBrowser.ViewModels;
using Themis.DocumentManager.Features.Timeline.Views;
using Themis.DocumentManager.Features.Dashboard.Views;
using Themis.DocumentManager.Features.Dashboard.ViewModels;
using Themis.DocumentManager.Features.Gantt.ViewModels;
using Themis.DocumentManager.Features.MetadataForm.ViewModels;
using Themis.DocumentManager.Features.TaskBasket.ViewModels;
using Themis.DocumentManager.Services;
using Themis.DocumentManager.Views.Windows;
using System.Windows.Input;
using System.Windows.Controls;
using Themis.DocumentManager.Features.Favorites.ViewModels;
using Themis.DocumentManager.ViewModels.Navigation;
using Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;
using Themis.DocumentManager.Models;
// Feature view namespaces intentionally not imported to avoid namespace/type collisions
// Types from feature namespaces are referenced with fully-qualified names where needed.

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
    private readonly Themis.DocumentManager.Features.AIChat.ViewModels.AIChatViewModel _aiChatViewModel;

    public MainWindow(
        MainViewModel viewModel,
        IOfficeIntegrationService officeService,
        IThemeService themeService,
        ISettingsService settingsService,
        IAnimationService animationService,
        Themis.DocumentManager.Features.AIChat.ViewModels.AIChatViewModel aiChatViewModel,
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
            _aiChatViewModel = aiChatViewModel;
            DataContext = _viewModel;

            // Wire Intelligent Breadcrumb ViewModel from DI
            try
            {
                var breadcrumbVm = App.GetService<IntelligentBreadcrumbViewModel>();
                if (breadcrumbVm != null && BreadcrumbControl != null)
                {
                    BreadcrumbControl.DataContext = breadcrumbVm;
                    // Initial context to populate breadcrumb
                    var initialContext = new NavigationContext
                    {
                        EntityId = "doc001",
                        EntityType = Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath.EntityType.Document
                    };
                    breadcrumbVm.LoadNavigationPathCommand?.Execute(initialContext);
                }
            }
            catch { }

            // Timeline Ruler is handled by its own ViewModel (no wiring needed)
            
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
            Loaded += (_, __) => LoadTreeViewFromSettings();

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
            // Inner Documents Panel: bind DocumentBrowserViewModel
            var docListVm = App.GetService<DocumentBrowserViewModel>();
            if (docListVm != null && DocumentsList != null)
            {
                DocumentsList.ItemsSource = docListVm.Documents;
                DocumentSearchBox.TextChanged += (s, e) => 
                {
                    docListVm.SearchText = DocumentSearchBox.Text;
                };
            }

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
            // Tasks: bind TasksRightSidebarViewModel
            var tasksVm = App.GetService<TasksRightSidebarViewModel>();
            if (tasksVm != null && RightTasksPanel != null)
            {
                RightTasksPanel.DataContext = tasksVm;
                // Load initial tasks
                await tasksVm.LoadTasksAsync();
            }

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

    /// <summary>
    /// Updates the task context when a document/case/process is selected
    /// This implements VIS-style context-aware task filtering
    /// </summary>
    public async Task UpdateTaskContextAsync(string? entityId, Application.Tasks.Queries.GetMyTasks.LinkedEntityType? entityType)
    {
        try
        {
            var tasksVm = App.GetService<TasksRightSidebarViewModel>();
            if (tasksVm != null)
            {
                await tasksVm.UpdateEntityContextAsync(entityId, entityType);
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Fehler beim Aktualisieren des Aufgaben-Kontexts: {ex.Message}";
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
            Themis.DocumentManager.Features.DocumentBrowser.Views.DocumentBrowserSimpleView => "documents",
            Themis.DocumentManager.Features.Timeline.Views.TimelineViewImproved => "timeline",
            Themis.DocumentManager.Views.FullDashboardSimpleView => "dashboard",
            Themis.DocumentManager.Features.Dashboard.Views.DashboardPreviewView => "start",
            _ => string.Empty
        };
    }

    private static object? CreateContentById(string id)
    {
        return id switch
        {
            "documents" => new Themis.DocumentManager.Features.DocumentBrowser.Views.DocumentBrowserSimpleView(),
            "timeline" => new Themis.DocumentManager.Features.Timeline.Views.TimelineViewImproved(),
            "dashboard" => new Themis.DocumentManager.Views.FullDashboardSimpleView(),
            "start" => new Themis.DocumentManager.Features.Dashboard.Views.DashboardPreviewView(),
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
        if (content is TimelineViewImproved)
            return App.GetService<TimelineViewImproved>() ?? new TimelineViewImproved();
            if (content is Themis.DocumentManager.Views.FullDashboardSimpleView)
            return App.GetService<Themis.DocumentManager.Views.FullDashboardSimpleView>() ?? new Themis.DocumentManager.Views.FullDashboardSimpleView();
        if (content is Themis.DocumentManager.Features.Dashboard.Views.DashboardPreviewView)
            return App.GetService<Themis.DocumentManager.Features.Dashboard.Views.DashboardPreviewView>() ?? new Themis.DocumentManager.Features.Dashboard.Views.DashboardPreviewView();
        return null;
    }

    private static object? CloneContentViaDi(object? content)
    {
        return content switch
        {
            DocumentBrowserSimpleView => App.GetService<DocumentBrowserSimpleView>() ?? new DocumentBrowserSimpleView(),
            TimelineViewImproved => App.GetService<TimelineViewImproved>() ?? new TimelineViewImproved(),
            Themis.DocumentManager.Views.FullDashboardSimpleView => App.GetService<Themis.DocumentManager.Views.FullDashboardSimpleView>() ?? new Themis.DocumentManager.Views.FullDashboardSimpleView(),
            Themis.DocumentManager.Features.Dashboard.Views.DashboardPreviewView => App.GetService<Themis.DocumentManager.Features.Dashboard.Views.DashboardPreviewView>() ?? new Themis.DocumentManager.Features.Dashboard.Views.DashboardPreviewView(),
            _ => null
        };
    }

    #region Sidebar Tab Management

    private TreeViewItem? _contextMenuTreeViewItem;
    private TreeViewItem? _lastClickedTreeViewItem;
    private DateTime _lastTreeViewClickTime = DateTime.MinValue;

    private void LoadTreeViewFromSettings()
    {
        try
        {
            var settings = _settingsService?.LoadTreeViewSettings();
            if (settings != null && settings.RootItems.Count > 0)
            {
                NavigationTreeView.Items.Clear();
                foreach (var item in settings.RootItems)
                {
                    NavigationTreeView.Items.Add(CreateTreeViewItem(item, settings));
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Fehler beim Laden der TreeView-Settings: {ex.Message}");
        }
    }

    private TreeViewItem CreateTreeViewItem(Models.TreeViewItemConfig config, Models.TreeViewSettings settings)
    {
        var item = new TreeViewItem();
        
        // Header mit Icon
        var panel = new System.Windows.Controls.StackPanel
        {
            Orientation = System.Windows.Controls.Orientation.Horizontal
        };
        
        if (settings.ShowIcons && !string.IsNullOrEmpty(config.Icon))
        {
            panel.Children.Add(new TextBlock
            {
                Text = config.Icon,
                Margin = new Thickness(0, 0, 8, 0),
                FontSize = 14
            });
        }
        
        panel.Children.Add(new TextBlock
        {
            Text = config.Header
        });
        
        item.Header = panel;
        item.IsExpanded = config.IsExpanded;
        item.Tag = config; // Store config for navigation

        // Children rekursiv hinzufügen
        foreach (var child in config.Children)
        {
            item.Items.Add(CreateTreeViewItem(child, settings));
        }

        return item;
    }

    private void SidebarTab_Click(object sender, RoutedEventArgs e)
    {
        if (sender is RadioButton btn)
        {
            if (btn.Name == "SidebarTabNavigation")
            {
                NavigationTabContent.Visibility = Visibility.Visible;
                TasksTabContent.Visibility = Visibility.Collapsed;
                FavoritesTabContent.Visibility = Visibility.Collapsed;
            }
            else if (btn.Name == "SidebarTabTasks")
            {
                NavigationTabContent.Visibility = Visibility.Collapsed;
                TasksTabContent.Visibility = Visibility.Visible;
                FavoritesTabContent.Visibility = Visibility.Collapsed;
            }
            else if (btn.Name == "SidebarTabFavorites")
            {
                NavigationTabContent.Visibility = Visibility.Collapsed;
                TasksTabContent.Visibility = Visibility.Collapsed;
                FavoritesTabContent.Visibility = Visibility.Visible;
            }
        }
    }

    #endregion

    #region TreeView Event Handlers

    private void NavigationTreeView_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        var clickedElement = e.OriginalSource as DependencyObject;
        var treeViewItem = GetTreeViewItemFromPoint(clickedElement);
        
        if (treeViewItem != null)
        {
            var timeSinceLastClick = DateTime.Now - _lastTreeViewClickTime;
            if (_lastClickedTreeViewItem == treeViewItem && timeSinceLastClick.TotalMilliseconds < 300)
            {
                return;
            }

            _lastClickedTreeViewItem = treeViewItem;
            _lastTreeViewClickTime = DateTime.Now;
            
            LoadPreviewInCurrentTab(treeViewItem);
            e.Handled = false;
        }
    }

    private void NavigationTreeView_PreviewMouseDoubleClick(object sender, MouseButtonEventArgs e)
    {
        var clickedElement = e.OriginalSource as DependencyObject;
        var treeViewItem = GetTreeViewItemFromPoint(clickedElement);
        
        if (treeViewItem != null)
        {
            LoadPreviewInNewTab(treeViewItem);
            e.Handled = true;
        }
    }

    private void NavigationTreeView_PreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
    {
        var clickedElement = e.OriginalSource as DependencyObject;
        var treeViewItem = GetTreeViewItemFromPoint(clickedElement);
        
        if (treeViewItem != null)
        {
            _contextMenuTreeViewItem = treeViewItem;
            treeViewItem.IsSelected = true;
        }
    }

    private TreeViewItem? GetTreeViewItemFromPoint(DependencyObject? source)
    {
        if (source == null) return null;

        var current = source;
        while (current != null)
        {
            if (current is TreeViewItem item)
                return item;
            current = System.Windows.Media.VisualTreeHelper.GetParent(current);
        }
        return null;
    }

    private void LoadPreviewInCurrentTab(TreeViewItem item)
    {
        var header = item.Header?.ToString() ?? "";
        Console.WriteLine($"Preview in current tab: {header}");
        // TODO: Implementierung für Preview-Loading
    }

    private void LoadPreviewInNewTab(TreeViewItem item)
    {
        var header = item.Header?.ToString() ?? "";
        Console.WriteLine($"Open in new tab: {header}");
        // TODO: Implementierung für neuen Tab
    }

    #endregion

    #region TreeView Context Menu Handlers

    private void TreeViewContext_Open(object sender, RoutedEventArgs e)
    {
        if (_contextMenuTreeViewItem != null)
        {
            LoadPreviewInCurrentTab(_contextMenuTreeViewItem);
        }
    }

    private void TreeViewContext_OpenNewTab(object sender, RoutedEventArgs e)
    {
        if (_contextMenuTreeViewItem != null)
        {
            LoadPreviewInNewTab(_contextMenuTreeViewItem);
        }
    }

    private void TreeViewContext_Rename(object sender, RoutedEventArgs e)
    {
        if (_contextMenuTreeViewItem == null) return;
        
        var itemHeader = _contextMenuTreeViewItem.Header?.ToString() ?? "";
        var result = Microsoft.VisualBasic.Interaction.InputBox(
            "Neuer Name:",
            "Umbenennen",
            itemHeader);
        
        if (!string.IsNullOrWhiteSpace(result))
        {
            _contextMenuTreeViewItem.Header = result;
        }
    }

    private void TreeViewContext_Copy(object sender, RoutedEventArgs e)
    {
        if (_contextMenuTreeViewItem == null) return;
        
        var itemHeader = _contextMenuTreeViewItem.Header?.ToString() ?? "";
        Clipboard.SetText(itemHeader);
        MessageBox.Show($"'{itemHeader}' in Zwischenablage kopiert", "Kopieren", MessageBoxButton.OK, MessageBoxImage.Information);
    }

    private void TreeViewContext_Paste(object sender, RoutedEventArgs e)
    {
        if (_contextMenuTreeViewItem == null) return;

        if (Clipboard.ContainsText())
        {
            var clipboardData = Clipboard.GetText();
            MessageBox.Show($"Einfügen unter '{_contextMenuTreeViewItem.Header}':\n{clipboardData}", "Einfügen", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        else
        {
            MessageBox.Show("Zwischenablage ist leer", "Fehler", MessageBoxButton.OK, MessageBoxImage.Warning);
        }
    }

    private void TreeViewContext_Delete(object sender, RoutedEventArgs e)
    {
        if (_contextMenuTreeViewItem == null) return;

        var itemHeader = _contextMenuTreeViewItem.Header?.ToString() ?? "";
        var result = MessageBox.Show(
            $"Möchten Sie '{itemHeader}' wirklich löschen?",
            "Löschen bestätigen",
            MessageBoxButton.YesNo,
            MessageBoxImage.Question);

        if (result == MessageBoxResult.Yes)
        {
            if (_contextMenuTreeViewItem.Parent is TreeViewItem parentItem)
            {
                parentItem.Items.Remove(_contextMenuTreeViewItem);
            }
            else if (_contextMenuTreeViewItem.Parent is TreeView treeView)
            {
                treeView.Items.Remove(_contextMenuTreeViewItem);
            }
        }
    }

    #endregion
}
