using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Input;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.ViewModels;

public partial class TimelineViewModel : ObservableObject
{
    private readonly ITimelineAggregationService _timelineService;
    private readonly INotificationService _notificationService;

    [ObservableProperty]
    private ObservableCollection<TimelineItem> _items = new();

    [ObservableProperty]
    private ObservableCollection<TimelineItem> _filteredItems = new();

    [ObservableProperty]
    private ObservableCollection<TimelineSegment> _segments = new();

    [ObservableProperty]
    private TimelineRange _currentRange;

    [ObservableProperty]
    private TimelineScale _currentScale = TimelineScale.OneMonth;

    [ObservableProperty]
    private DateTime _centerDate = DateTime.Now;

    [ObservableProperty]
    private TimelineFilter _filter = new();

    [ObservableProperty]
    private string? _selectedProcessId;

    [ObservableProperty]
    private bool _isProcessMode;

    [ObservableProperty]
    private bool _showOthersDimmed = true;

    [ObservableProperty]
    private bool _isLoading;

    [ObservableProperty]
    private string _searchText = string.Empty;

    [ObservableProperty]
    private int _totalItemsCount;

    [ObservableProperty]
    private int _filteredItemsCount;

    // Filter chips
    [ObservableProperty]
    private ObservableCollection<TimelineObjectType> _selectedObjectTypes = new();

    [ObservableProperty]
    private ObservableCollection<TimelinePriority> _selectedPriorities = new();

    [ObservableProperty]
    private ObservableCollection<TimelineStatus> _selectedStatuses = new();

    // Statistics
    [ObservableProperty]
    private Dictionary<TimelineObjectType, int> _countByType = new();

    [ObservableProperty]
    private Dictionary<TimelinePriority, int> _countByPriority = new();

    [ObservableProperty]
    private Dictionary<TimelineStatus, int> _countByStatus = new();

    public TimelineViewModel(ITimelineAggregationService timelineService, INotificationService notificationService)
    {
        _timelineService = timelineService;
        _notificationService = notificationService;
        _currentRange = TimelineRange.FromScale(DateTime.Now, TimelineScale.OneMonth);

        // Initialize commands
        LoadItemsCommand = new AsyncRelayCommand(LoadItemsAsync);
        ZoomInCommand = new RelayCommand(ZoomIn, CanZoomIn);
        ZoomOutCommand = new RelayCommand(ZoomOut, CanZoomOut);
        ZoomToScaleCommand = new AsyncRelayCommand<TimelineScale>(ZoomToScaleAsync);
        JumpToTodayCommand = new RelayCommand(JumpToToday);
        JumpToDateCommand = new AsyncRelayCommand<DateTime>(JumpToDateAsync);
        ApplyFiltersCommand = new AsyncRelayCommand(ApplyFiltersAsync);
        ClearFiltersCommand = new AsyncRelayCommand(ClearFiltersAsync);
        SelectProcessCommand = new AsyncRelayCommand<string>(SelectProcessAsync);
        ClearProcessCommand = new AsyncRelayCommand(ClearProcessAsync);
        NavigateToItemCommand = new RelayCommand<TimelineItem>(NavigateToItem);
        OpenItemCommand = new RelayCommand<TimelineItem>(OpenItem);
        RefreshCommand = new AsyncRelayCommand(LoadItemsAsync);

        // Watch for filter changes
        SelectedObjectTypes.CollectionChanged += (s, e) => _ = ApplyFiltersAsync();
        SelectedPriorities.CollectionChanged += (s, e) => _ = ApplyFiltersAsync();
        SelectedStatuses.CollectionChanged += (s, e) => _ = ApplyFiltersAsync();
    }

    public ICommand LoadItemsCommand { get; }
    public ICommand ZoomInCommand { get; }
    public ICommand ZoomOutCommand { get; }
    public ICommand ZoomToScaleCommand { get; }
    public ICommand JumpToTodayCommand { get; }
    public ICommand JumpToDateCommand { get; }
    public ICommand ApplyFiltersCommand { get; }
    public ICommand ClearFiltersCommand { get; }
    public ICommand SelectProcessCommand { get; }
    public ICommand ClearProcessCommand { get; }
    public ICommand NavigateToItemCommand { get; }
    public ICommand OpenItemCommand { get; }
    public ICommand RefreshCommand { get; }

    public async Task LoadItemsAsync()
    {
        try
        {
            IsLoading = true;

            TimelineAggregationResult result;

            if (IsProcessMode && !string.IsNullOrEmpty(SelectedProcessId))
            {
                result = await _timelineService.AggregateProcessItemsAsync(
                    SelectedProcessId,
                    CurrentRange.StartDate,
                    CurrentRange.EndDate,
                    ShowOthersDimmed);
            }
            else
            {
                result = await _timelineService.AggregateAllItemsAsync(
                    CurrentRange.StartDate,
                    CurrentRange.EndDate,
                    null);
            }

            Items.Clear();
            foreach (var item in result.Items.OrderBy(i => i.Date))
            {
                Items.Add(item);
            }

            TotalItemsCount = result.TotalCount;
            CountByType = result.CountByType;
            CountByPriority = result.CountByPriority;
            CountByStatus = result.CountByStatus;

            await ApplyFiltersAsync();
            UpdateSegments();
        }
        catch (Exception ex)
        {
            await _notificationService.ShowNotificationAsync(new Notification
            {
                Type = NotificationType.Error,
                Title = "Fehler beim Laden",
                Message = $"Timeline-Daten konnten nicht geladen werden: {ex.Message}"
            });
        }
        finally
        {
            IsLoading = false;
        }
    }

    public void ZoomIn()
    {
        var currentIndex = (int)CurrentScale;
        if (currentIndex > 0)
        {
            CurrentScale = (TimelineScale)(currentIndex - 1);
            _ = ZoomToScaleAsync(CurrentScale);
        }
    }

    public void ZoomOut()
    {
        var currentIndex = (int)CurrentScale;
        if (currentIndex < (int)TimelineScale.FiveYears)
        {
            CurrentScale = (TimelineScale)(currentIndex + 1);
            _ = ZoomToScaleAsync(CurrentScale);
        }
    }

    public bool CanZoomIn() => (int)CurrentScale > 0;
    public bool CanZoomOut() => (int)CurrentScale < (int)TimelineScale.FiveYears;

    public async Task ZoomToScaleAsync(TimelineScale scale)
    {
        CurrentScale = scale;
        CurrentRange = TimelineRange.FromScale(CenterDate, scale);
        UpdateSegments();
        await LoadItemsAsync();
    }

    public void JumpToToday()
    {
        CenterDate = DateTime.Now;
        _ = ZoomToScaleAsync(CurrentScale);
    }

    public async Task JumpToDateAsync(DateTime date)
    {
        CenterDate = date;
        await ZoomToScaleAsync(CurrentScale);
    }

    public async Task ApplyFiltersAsync()
    {
        Filter.ObjectTypes = SelectedObjectTypes.ToList();
        Filter.Priorities = SelectedPriorities.ToList();
        Filter.Statuses = SelectedStatuses.ToList();
        Filter.SearchText = SearchText;
        Filter.ProcessId = IsProcessMode ? SelectedProcessId : null;

        var filtered = _timelineService.ApplyFilters(Items.ToList(), Filter);

        FilteredItems.Clear();
        foreach (var item in filtered)
        {
            FilteredItems.Add(item);
        }

        FilteredItemsCount = FilteredItems.Count;

        await Task.CompletedTask;
    }

    public async Task ClearFiltersAsync()
    {
        SelectedObjectTypes.Clear();
        SelectedPriorities.Clear();
        SelectedStatuses.Clear();
        SearchText = string.Empty;
        await ApplyFiltersAsync();
    }

    public async Task SelectProcessAsync(string? processId)
    {
        SelectedProcessId = processId;
        IsProcessMode = !string.IsNullOrEmpty(processId);
        await LoadItemsAsync();
    }

    public async Task ClearProcessAsync()
    {
        SelectedProcessId = null;
        IsProcessMode = false;
        await LoadItemsAsync();
    }

    public void NavigateToItem(TimelineItem? item)
    {
        if (item == null) return;

        // Navigate to the appropriate view based on object type
        // This would be implemented by the MainViewModel or navigation service
        // For now, just show a notification
        _notificationService.ShowNotificationAsync(new Notification
        {
            Type = NotificationType.Info,
            Title = "Navigation",
            Message = $"Navigiere zu {item.ObjectType}: {item.Title}"
        });
    }

    public void OpenItem(TimelineItem? item)
    {
        if (item == null) return;

        // Open the item in a detailed view
        _notificationService.ShowNotificationAsync(new Notification
        {
            Type = NotificationType.Info,
            Title = "Öffnen",
            Message = $"Öffne {item.ObjectType}: {item.Title}"
        });
    }

    private void UpdateSegments()
    {
        Segments.Clear();

        var segmentDuration = GetSegmentDuration(CurrentScale);
        var current = CurrentRange.StartDate;

        while (current <= CurrentRange.EndDate)
        {
            var position = (current - CurrentRange.StartDate).TotalMilliseconds / 
                          CurrentRange.Duration.TotalMilliseconds;

            var segment = new TimelineSegment
            {
                Date = current,
                Label = FormatSegmentLabel(current, CurrentScale),
                IsMajor = IsMajorSegment(current, CurrentScale),
                Position = position
            };

            Segments.Add(segment);
            current = current.Add(segmentDuration);
        }
    }

    private TimeSpan GetSegmentDuration(TimelineScale scale) => scale switch
    {
        TimelineScale.OneDay => TimeSpan.FromHours(1),
        TimelineScale.ThreeDays => TimeSpan.FromHours(6),
        TimelineScale.OneWeek => TimeSpan.FromDays(1),
        TimelineScale.TwoWeeks => TimeSpan.FromDays(1),
        TimelineScale.OneMonth => TimeSpan.FromDays(7),
        TimelineScale.ThreeMonths => TimeSpan.FromDays(14),
        TimelineScale.SixMonths => TimeSpan.FromDays(30),
        TimelineScale.OneYear => TimeSpan.FromDays(30),
        TimelineScale.FiveYears => TimeSpan.FromDays(365),
        _ => TimeSpan.FromDays(1)
    };

    private string FormatSegmentLabel(DateTime date, TimelineScale scale) => scale switch
    {
        TimelineScale.OneDay => date.ToString("HH:mm"),
        TimelineScale.ThreeDays => date.ToString("HH:mm"),
        TimelineScale.OneWeek => date.ToString("ddd"),
        TimelineScale.TwoWeeks => date.ToString("dd.MM"),
        TimelineScale.OneMonth => $"KW {GetWeekNumber(date)}",
        TimelineScale.ThreeMonths => date.ToString("dd.MM"),
        TimelineScale.SixMonths => date.ToString("MMM"),
        TimelineScale.OneYear => date.ToString("MMM"),
        TimelineScale.FiveYears => date.ToString("yyyy"),
        _ => date.ToString("dd.MM")
    };

    private bool IsMajorSegment(DateTime date, TimelineScale scale) => scale switch
    {
        TimelineScale.OneDay => date.Hour % 3 == 0,
        TimelineScale.ThreeDays => date.Hour == 0,
        TimelineScale.OneWeek => true,
        TimelineScale.TwoWeeks => date.Day % 7 == 1,
        TimelineScale.OneMonth => date.Day == 1,
        TimelineScale.ThreeMonths => date.Day == 1,
        TimelineScale.SixMonths => date.Day == 1,
        TimelineScale.OneYear => date.Month % 3 == 1,
        TimelineScale.FiveYears => date.Month == 1,
        _ => false
    };

    private int GetWeekNumber(DateTime date)
    {
        var culture = System.Globalization.CultureInfo.CurrentCulture;
        return culture.Calendar.GetWeekOfYear(
            date,
            culture.DateTimeFormat.CalendarWeekRule,
            culture.DateTimeFormat.FirstDayOfWeek);
    }

    partial void OnSearchTextChanged(string value)
    {
        _ = ApplyFiltersAsync();
    }

    partial void OnShowOthersDimmedChanged(bool value)
    {
        _ = LoadItemsAsync();
    }
}
