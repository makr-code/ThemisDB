using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System.Collections.ObjectModel;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.ViewModels;

/// <summary>
/// Document browser view model
/// </summary>
public partial class DocumentBrowserViewModel : ObservableObject
{
    private readonly IDocumentService _documentService;

    [ObservableProperty]
    private ObservableCollection<Document> _documents = new();

    [ObservableProperty]
    private Document? _selectedDocument;

    [ObservableProperty]
    private bool _isLoading = false;

    public DocumentBrowserViewModel(IDocumentService documentService)
    {
        _documentService = documentService;
        LoadDocumentsCommand.Execute(null);
    }

    [RelayCommand]
    private async Task LoadDocumentsAsync()
    {
        IsLoading = true;
        try
        {
            var docs = await _documentService.GetAllDocumentsAsync();
            Documents = new ObservableCollection<Document>(docs);
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private async Task DeleteDocumentAsync(string documentId)
    {
        if (string.IsNullOrEmpty(documentId)) return;

        var success = await _documentService.DeleteDocumentAsync(documentId);
        if (success)
        {
            var doc = Documents.FirstOrDefault(d => d.Id == documentId);
            if (doc != null) Documents.Remove(doc);
        }
    }

    [RelayCommand]
    private async Task RefreshAsync()
    {
        await LoadDocumentsAsync();
    }
}

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

/// <summary>
/// Geo view model
/// </summary>
public partial class GeoViewModel : ObservableObject
{
    private readonly IGeoService _geoService;

    [ObservableProperty]
    private ObservableCollection<Document> _documentsOnMap = new();

    [ObservableProperty]
    private Document? _selectedDocument;

    [ObservableProperty]
    private double _centerLatitude = 51.1657; // Germany

    [ObservableProperty]
    private double _centerLongitude = 10.4515;

    [ObservableProperty]
    private double _searchRadius = 50.0; // km

    [ObservableProperty]
    private bool _isLoading = false;

    public GeoViewModel(IGeoService geoService)
    {
        _geoService = geoService;
    }

    [RelayCommand]
    private async Task SearchByLocationAsync()
    {
        IsLoading = true;
        try
        {
            var docs = await _geoService.GetDocumentsByLocationAsync(
                CenterLatitude, CenterLongitude, SearchRadius);
            DocumentsOnMap = new ObservableCollection<Document>(docs);
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private async Task SearchByRegionAsync(object[] bounds)
    {
        if (bounds.Length != 4) return;

        IsLoading = true;
        try
        {
            var docs = await _geoService.GetDocumentsByRegionAsync(
                (double)bounds[0], (double)bounds[1],
                (double)bounds[2], (double)bounds[3]);
            DocumentsOnMap = new ObservableCollection<Document>(docs);
        }
        finally
        {
            IsLoading = false;
        }
    }
}

/// <summary>
/// Timeline view model
/// </summary>
public partial class TimelineViewModel : ObservableObject
{
    private readonly ITimelineService _timelineService;

    [ObservableProperty]
    private ObservableCollection<TimelineEvent> _events = new();

    [ObservableProperty]
    private TimelineEvent? _selectedEvent;

    [ObservableProperty]
    private DateTime _startDate = DateTime.Now.AddMonths(-1);

    [ObservableProperty]
    private DateTime _endDate = DateTime.Now;

    [ObservableProperty]
    private bool _isLoading = false;

    public TimelineViewModel(ITimelineService timelineService)
    {
        _timelineService = timelineService;
        LoadEventsCommand.Execute(null);
    }

    [RelayCommand]
    private async Task LoadEventsAsync()
    {
        IsLoading = true;
        try
        {
            var events = await _timelineService.GetEventsAsync(StartDate, EndDate);
            Events = new ObservableCollection<TimelineEvent>(events);
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private async Task FilterByDateRangeAsync()
    {
        await LoadEventsAsync();
    }
}

/// <summary>
/// Graph view model
/// </summary>
public partial class GraphViewModel : ObservableObject
{
    private readonly IGraphService _graphService;
    private readonly IDocumentService _documentService;

    [ObservableProperty]
    private ObservableCollection<Document> _nodes = new();

    [ObservableProperty]
    private ObservableCollection<DocumentRelation> _edges = new();

    [ObservableProperty]
    private Document? _selectedNode;

    [ObservableProperty]
    private string _startDocumentId = string.Empty;

    [ObservableProperty]
    private int _maxDepth = 3;

    [ObservableProperty]
    private bool _isLoading = false;

    public GraphViewModel(
        IGraphService graphService,
        IDocumentService documentService)
    {
        _graphService = graphService;
        _documentService = documentService;
    }

    [RelayCommand]
    private async Task TraverseAsync()
    {
        if (string.IsNullOrWhiteSpace(StartDocumentId)) return;

        IsLoading = true;
        try
        {
            var docs = await _graphService.TraverseGraphAsync(StartDocumentId, MaxDepth);
            Nodes = new ObservableCollection<Document>(docs);

            // Load edges for visualization
            var allEdges = new List<DocumentRelation>();
            foreach (var doc in docs)
            {
                var docEdges = await _graphService.GetDocumentRelationsAsync(doc.Id);
                allEdges.AddRange(docEdges);
            }

            Edges = new ObservableCollection<DocumentRelation>(allEdges.Distinct());
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private async Task LoadNodeRelationsAsync(string documentId)
    {
        if (string.IsNullOrWhiteSpace(documentId)) return;

        IsLoading = true;
        try
        {
            var relations = await _graphService.GetDocumentRelationsAsync(documentId);
            Edges = new ObservableCollection<DocumentRelation>(relations);
        }
        finally
        {
            IsLoading = false;
        }
    }
}
