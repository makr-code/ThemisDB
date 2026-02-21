/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainViewModel.cs                                   ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     231                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.ViewModels;

/// <summary>
/// Main window view model
/// </summary>
public partial class MainViewModel : ObservableObject
{
    private readonly IDocumentService _documentService;
    private readonly ISearchService _searchService;
    private readonly IOfficeIntegrationService _officeIntegrationService;
    private readonly IThemeService _themeService;

    [ObservableProperty]
    private string _currentView = "DocumentBrowser";

    [ObservableProperty]
    private string _searchQuery = string.Empty;

    [ObservableProperty]
    private bool _isLoading = false;

    [ObservableProperty]
    private UserSwitcherViewModel? userSwitcherViewModel;

    [ObservableProperty]
    private BreadcrumbViewModel? breadcrumbViewModel;

    public MainViewModel(
        IDocumentService documentService,
        ISearchService searchService,
        IOfficeIntegrationService officeIntegrationService,
        IThemeService themeService,
        UserSwitcherViewModel userSwitcherViewModel,
        BreadcrumbViewModel breadcrumbViewModel)
    {
        _documentService = documentService;
        _searchService = searchService;
        _officeIntegrationService = officeIntegrationService;
        _themeService = themeService;
        UserSwitcherViewModel = userSwitcherViewModel;
        BreadcrumbViewModel = breadcrumbViewModel;
    }
    [RelayCommand]
    private async Task NewAsync()
    {
        IsLoading = true;
        try
        {
            // Create a new Word document via Office integration
            var result = await _officeIntegrationService.CreateNewWordDocumentAsync();
            if (result.Success)
            {
                NavigateToDocumentBrowser();
            }
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private async Task OpenAsync()
    {
        try
        {
            // Prompt user to select a document to open
            var ofd = new Microsoft.Win32.OpenFileDialog
            {
                Filter = "Office-Dateien|*.docx;*.xlsx;*.pptx|Alle Dateien|*.*",
                Title = "Dokument öffnen"
            };
            var ok = ofd.ShowDialog() == true;
            if (!ok) return;

            var path = ofd.FileName;
            if (string.IsNullOrWhiteSpace(path)) return;

            // Open based on file extension
            var ext = System.IO.Path.GetExtension(path).ToLowerInvariant();
            if (ext == ".docx")
            {
                await _officeIntegrationService.OpenWordDocumentAsync(path);
            }
            else if (ext == ".xlsx")
            {
                await _officeIntegrationService.OpenExcelWorkbookAsync(path);
            }
            else if (ext == ".pptx")
            {
                await _officeIntegrationService.OpenPowerPointPresentationAsync(path);
            }

            NavigateToDocumentBrowser();
        }
        catch { }
    }

    [RelayCommand]
    private async Task SaveAsync()
    {
        try
        {
            // Placeholder: saving is handled by Office apps; integrate revision save via service when context available
            // For now, navigate to browser to reflect state
            NavigateToDocumentBrowser();
            await Task.CompletedTask;
        }
        catch { }
    }

    [RelayCommand]
    private void NavigateToDocumentBrowser()
    {
        CurrentView = "DocumentBrowser";
    }

    [RelayCommand]
    private void NavigateToSearch()
    {
        CurrentView = "Search";
    }

    [RelayCommand]
    private void NavigateToGeoView()
    {
        CurrentView = "GeoView";
    }

    [RelayCommand]
    private void NavigateToTimeline()
    {
        CurrentView = "Timeline";
    }

    [RelayCommand]
    private void NavigateToGraphView()
    {
        CurrentView = "GraphView";
    }

    [RelayCommand]
    private async Task SearchAsync()
    {
        if (string.IsNullOrWhiteSpace(SearchQuery)) return;

        IsLoading = true;
        try
        {
            await _searchService.FullTextSearchAsync(SearchQuery);
            NavigateToSearch();
        }
        finally
        {
            IsLoading = false;
        }
    }

    #region Phase 29 - Keyboard Shortcuts Commands

    [RelayCommand]
    private void OpenSettings()
    {
        // Wird von MainWindow.xaml.cs gehandelt
    }

    [RelayCommand]
    private void NextTab()
    {
        // Wird von MainWindow.xaml.cs gehandelt
    }

    [RelayCommand]
    private void PreviousTab()
    {
        // Wird von MainWindow.xaml.cs gehandelt
    }

    [RelayCommand]
    private void OpenSearch()
    {
        NavigateToSearch();
    }

    [RelayCommand]
    private void ToggleTheme()
    {
        // Cycle Light -> Dark -> System
        var next = _themeService.CurrentTheme switch
        {
            ThemeService.ThemeMode.Light => ThemeService.ThemeMode.Dark,
            ThemeService.ThemeMode.Dark => ThemeService.ThemeMode.System,
            _ => ThemeService.ThemeMode.Light
        };
        _themeService.CurrentTheme = next;
        _themeService.SaveThemeSetting();
    }

    #endregion
}
