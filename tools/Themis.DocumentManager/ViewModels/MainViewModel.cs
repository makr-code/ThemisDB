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

    [ObservableProperty]
    private string _currentView = "DocumentBrowser";

    [ObservableProperty]
    private string _searchQuery = string.Empty;

    [ObservableProperty]
    private bool _isLoading = false;

    public MainViewModel(
        IDocumentService documentService,
        ISearchService searchService)
    {
        _documentService = documentService;
        _searchService = searchService;
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
}
