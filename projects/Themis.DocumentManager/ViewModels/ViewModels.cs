/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ViewModels.cs                                      ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     168                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 362722340  2025-12-12  chore: workspace reorganization and build system consolid... ║
    • e35bb0178  2025-12-10  Phase 25: Complete UI implementation (GeoView, GraphView,... ║
    • 60d127110  2025-12-09  feat: Add comprehensive test report for ThemisDB Document... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;
using MediatR;
using Themis.DocumentManager.Domain.Events;

namespace Themis.DocumentManager.ViewModels;

/// <summary>
/// Document detail view model
/// </summary>
public partial class DocumentDetailViewModel : ObservableObject
{
    private readonly IDocumentService _documentService;
    private readonly IMetadataService _metadataService;

    [ObservableProperty]
    private Document? _document;

    [ObservableProperty]
    private ObservableCollection<DocumentChunk> _chunks = new();

    [ObservableProperty]
    private bool _isLoading = false;

    [ObservableProperty]
    private bool _isEditing = false;

    public DocumentDetailViewModel(
        IDocumentService documentService,
        IMetadataService metadataService)
    {
        _documentService = documentService;
        _metadataService = metadataService;
    }

    [RelayCommand]
    private async Task LoadDocumentAsync(string documentId)
    {
        IsLoading = true;
        try
        {
            Document = await _documentService.GetDocumentByIdAsync(documentId);
            if (Document != null)
            {
                var chunks = await _documentService.GetDocumentChunksAsync(documentId);
                Chunks = new ObservableCollection<DocumentChunk>(chunks);
            }
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private void StartEdit()
    {
        IsEditing = true;
    }

    [RelayCommand]
    private async Task SaveAsync()
    {
        if (Document == null) return;

        IsLoading = true;
        try
        {
            await _documentService.UpdateDocumentAsync(Document);
            IsEditing = false;
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private void CancelEdit()
    {
        IsEditing = false;
    }
}

/// <summary>
/// Search view model
/// </summary>
public partial class SearchViewModel : ObservableObject
{
    private readonly ISearchService _searchService;

    [ObservableProperty]
    private string _searchQuery = string.Empty;

    [ObservableProperty]
    private ObservableCollection<SearchResult> _results = new();

    [ObservableProperty]
    private SearchResult? _selectedResult;

    [ObservableProperty]
    private bool _isLoading = false;

    [ObservableProperty]
    private string _searchType = "FullText";

    public SearchViewModel(ISearchService searchService)
    {
        _searchService = searchService;
    }

    [RelayCommand]
    private async Task SearchAsync()
    {
        if (string.IsNullOrWhiteSpace(SearchQuery)) return;

        IsLoading = true;
        try
        {
            IEnumerable<SearchResult> results = SearchType switch
            {
                "FullText" => await _searchService.FullTextSearchAsync(SearchQuery),
                "Vector" => Enumerable.Empty<SearchResult>(), // Needs vector embedding
                _ => await _searchService.FullTextSearchAsync(SearchQuery)
            };

            Results = new ObservableCollection<SearchResult>(results);
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private void ClearSearch()
    {
        SearchQuery = string.Empty;
        Results.Clear();
    }
}

// GeoViewModel moved to separate file: ViewModels/GeoViewModel.cs

// GraphViewModel moved to separate file: ViewModels/GraphViewModel.cs

