/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineViewModel.cs                               ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     833                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
    private ObservableCollection<TimeRulerMark> _timeRulerMarks = new();

    [ObservableProperty]
    private List<TimelineScale> _availableScales = new()
    {
        TimelineScale.OneDay,
        TimelineScale.ThreeDays,
        TimelineScale.OneWeek,
        TimelineScale.TwoWeeks,
        TimelineScale.OneMonth,
        TimelineScale.ThreeMonths,
        TimelineScale.SixMonths,
        TimelineScale.OneYear,
        TimelineScale.FiveYears
    };

    [ObservableProperty]
    private TimelineRange _currentRange;

    [ObservableProperty]
    private TimelineScale _currentScale = TimelineScale.OneMonth;

    [ObservableProperty]
    private double _canvasWidth = 2000.0; // Default canvas width for calculations

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

    [ObservableProperty]
    private ObservableCollection<TimelineGanttBar> _ganttBars = new();

    [ObservableProperty]
    private bool _showGanttOverlay = true;

    [ObservableProperty]
    private bool _showMilestones = true;

    [ObservableProperty]
    private bool _showPhasesInBars = true;

    public TimelineViewModel(ITimelineAggregationService timelineService, INotificationService notificationService)
    {
        _timelineService = timelineService;
        _notificationService = notificationService;
        _currentRange = TimelineRange.FromScale(DateTime.Now, TimelineScale.OneMonth);

        // Initialize commands
        LoadItemsCommand = new AsyncRelayCommand(LoadItemsAsync);
        ZoomInCommand = new AsyncRelayCommand(ZoomInAsync, CanZoomIn);
        ZoomOutCommand = new AsyncRelayCommand(ZoomOutAsync, CanZoomOut);
        ZoomToScaleCommand = new AsyncRelayCommand<TimelineScale>(ZoomToScaleAsync);
        JumpToTodayCommand = new AsyncRelayCommand(JumpToTodayAsync);
        JumpToDateCommand = new AsyncRelayCommand<DateTime>(JumpToDateAsync);
        ApplyFiltersCommand = new AsyncRelayCommand(ApplyFiltersAsync);
        ClearFiltersCommand = new AsyncRelayCommand(ClearFiltersAsync);
        SelectProcessCommand = new AsyncRelayCommand<string>(SelectProcessAsync);
        ClearProcessCommand = new AsyncRelayCommand(ClearProcessAsync);
        NavigateToItemCommand = new AsyncRelayCommand<TimelineItem>(NavigateToItemAsync);
        OpenItemCommand = new AsyncRelayCommand<TimelineItem>(OpenItemAsync);
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
            LoadGanttBars();
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

    public async Task InitializeAsync()
    {
        await LoadItemsAsync();
    }

    public async Task ZoomInAsync()
    {
        var currentIndex = (int)CurrentScale;
        if (currentIndex > 0)
        {
            CurrentScale = (TimelineScale)(currentIndex - 1);
            await ZoomToScaleAsync(CurrentScale);
        }
    }

    public async Task ZoomOutAsync()
    {
        var currentIndex = (int)CurrentScale;
        if (currentIndex < (int)TimelineScale.FiveYears)
        {
            CurrentScale = (TimelineScale)(currentIndex + 1);
            await ZoomToScaleAsync(CurrentScale);
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

    partial void OnCurrentScaleChanged(TimelineScale value)
    {
        CurrentRange = TimelineRange.FromScale(CenterDate, value);
        UpdateSegments();
    }

    partial void OnCanvasWidthChanged(double value)
    {
        if (value > 0)
        {
            UpdateSegments();
        }
    }

    public async Task JumpToTodayAsync()
    {
        CenterDate = DateTime.Now;
        await ZoomToScaleAsync(CurrentScale);
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

    public async Task NavigateToItemAsync(TimelineItem? item)
    {
        if (item == null) return;

        // Navigate to the appropriate view based on object type
        // This would be implemented by the MainViewModel or navigation service
        // For now, just show a notification
        await _notificationService.ShowNotificationAsync(new Notification
        {
            Type = NotificationType.Info,
            Title = "Navigation",
            Message = $"Navigiere zu {item.ObjectType}: {item.Title}"
        });
    }

    public async Task OpenItemAsync(TimelineItem? item)
    {
        if (item == null) return;

        // Open the item in a detailed view
        await _notificationService.ShowNotificationAsync(new Notification
        {
            Type = NotificationType.Info,
            Title = "Öffnen",
            Message = $"Öffne {item.ObjectType}: {item.Title}"
        });
    }

    private void UpdateSegments()
    {
        Segments.Clear();
        TimeRulerMarks.Clear();

        var totalWidth = CanvasWidth > 0 ? CanvasWidth : 2000.0; // Use canvas width or default

        // Generate realistic ruler ticks based on scale
        switch (CurrentScale)
        {
            case TimelineScale.OneYear:
                GenerateYearlyRulerMarks(totalWidth);
                break;
            case TimelineScale.SixMonths:
                GenerateSixMonthsRulerMarks(totalWidth);
                break;
            case TimelineScale.ThreeMonths:
                GenerateThreeMonthsRulerMarks(totalWidth);
                break;
            case TimelineScale.OneMonth:
                GenerateMonthlyRulerMarks(totalWidth);
                break;
            case TimelineScale.TwoWeeks:
                GenerateTwoWeeksRulerMarks(totalWidth);
                break;
            case TimelineScale.OneWeek:
                GenerateWeeklyRulerMarks(totalWidth);
                break;
            case TimelineScale.ThreeDays:
                GenerateThreeDaysRulerMarks(totalWidth);
                break;
            case TimelineScale.OneDay:
                GenerateDailyRulerMarks(totalWidth);
                break;
            default:
                GenerateMonthlyRulerMarks(totalWidth);
                break;
        }
    }

    private void GenerateYearlyRulerMarks(double totalWidth)
    {
        // For 1 year: 365 day ticks (small) + 12 month ticks (large with labels)
        var current = CurrentRange.StartDate.Date;
        var endDate = CurrentRange.EndDate.Date;
        
        // Generate daily ticks (small, no labels)
        while (current <= endDate)
        {
            var position = CalculatePosition(current, totalWidth);
            var isMonthStart = current.Day == 1;
            
            TimeRulerMarks.Add(new TimeRulerMark
            {
                Date = current,
                Label = isMonthStart ? current.ToString("MMM") : string.Empty,
                PositionX = position,
                LabelPositionX = position - 12,
                TickHeight = isMonthStart ? 20 : 5,
                IsMajor = isMonthStart
            });
            
            current = current.AddDays(1);
        }
    }

    private void GenerateSixMonthsRulerMarks(double totalWidth)
    {
        // For 6 months: weekly ticks (small) + month start ticks (large with labels)
        var current = CurrentRange.StartDate.Date;
        var endDate = CurrentRange.EndDate.Date;
        
        while (current <= endDate)
        {
            var position = CalculatePosition(current, totalWidth);
            var isMonthStart = current.Day == 1;
            var isWeekStart = current.DayOfWeek == DayOfWeek.Monday;
            
            if (isMonthStart || isWeekStart)
            {
                TimeRulerMarks.Add(new TimeRulerMark
                {
                    Date = current,
                    Label = isMonthStart ? current.ToString("MMM yy") : string.Empty,
                    PositionX = position,
                    LabelPositionX = position - 15,
                    TickHeight = isMonthStart ? 20 : 8,
                    IsMajor = isMonthStart
                });
            }
            
            current = current.AddDays(1);
        }
    }

    private void GenerateThreeMonthsRulerMarks(double totalWidth)
    {
        // For 3 months: daily ticks (small) + week start ticks (medium) + month ticks (large with labels)
        var current = CurrentRange.StartDate.Date;
        var endDate = CurrentRange.EndDate.Date;
        
        while (current <= endDate)
        {
            var position = CalculatePosition(current, totalWidth);
            var isMonthStart = current.Day == 1;
            var isWeekStart = current.DayOfWeek == DayOfWeek.Monday;
            
            TimeRulerMarks.Add(new TimeRulerMark
            {
                Date = current,
                Label = isMonthStart ? current.ToString("MMM") : (isWeekStart ? $"KW{GetWeekNumber(current)}" : string.Empty),
                PositionX = position,
                LabelPositionX = position - 12,
                TickHeight = isMonthStart ? 20 : (isWeekStart ? 12 : 5),
                IsMajor = isMonthStart || isWeekStart
            });
            
            current = current.AddDays(1);
        }
    }

    private void GenerateMonthlyRulerMarks(double totalWidth)
    {
        // For 1 month: daily ticks (small) + week start (medium with label)
        var current = CurrentRange.StartDate.Date;
        var endDate = CurrentRange.EndDate.Date;
        
        while (current <= endDate)
        {
            var position = CalculatePosition(current, totalWidth);
            var isWeekStart = current.DayOfWeek == DayOfWeek.Monday;
            
            TimeRulerMarks.Add(new TimeRulerMark
            {
                Date = current,
                Label = isWeekStart ? current.ToString("dd.MM") : string.Empty,
                PositionX = position,
                LabelPositionX = position - 15,
                TickHeight = isWeekStart ? 20 : 8,
                IsMajor = isWeekStart
            });
            
            current = current.AddDays(1);
        }
    }

    private void GenerateTwoWeeksRulerMarks(double totalWidth)
    {
        // For 2 weeks: daily ticks (medium with labels)
        var current = CurrentRange.StartDate.Date;
        var endDate = CurrentRange.EndDate.Date;
        
        while (current <= endDate)
        {
            var position = CalculatePosition(current, totalWidth);
            
            TimeRulerMarks.Add(new TimeRulerMark
            {
                Date = current,
                Label = current.ToString("dd.MM"),
                PositionX = position,
                LabelPositionX = position - 15,
                TickHeight = 18,
                IsMajor = true
            });
            
            current = current.AddDays(1);
        }
    }

    private void GenerateWeeklyRulerMarks(double totalWidth)
    {
        // For 1 week: hourly ticks (small) + 6-hour ticks (medium) + day start (large with label)
        var current = CurrentRange.StartDate.Date;
        var endDate = CurrentRange.EndDate.AddDays(1).Date;
        
        while (current <= endDate)
        {
            var position = CalculatePosition(current, totalWidth);
            var isDayStart = current.Hour == 0;
            var isSixHourMark = current.Hour % 6 == 0;
            
            TimeRulerMarks.Add(new TimeRulerMark
            {
                Date = current,
                Label = isDayStart ? current.ToString("ddd dd.MM") : (isSixHourMark ? current.ToString("HH:mm") : string.Empty),
                PositionX = position,
                LabelPositionX = position - 20,
                TickHeight = isDayStart ? 20 : (isSixHourMark ? 12 : 5),
                IsMajor = isDayStart || isSixHourMark
            });
            
            current = current.AddHours(1);
        }
    }

    private void GenerateThreeDaysRulerMarks(double totalWidth)
    {
        // For 3 days: hourly ticks (small) + 6-hour marks (medium with label) + day start (large with label)
        var current = CurrentRange.StartDate.Date;
        var endDate = CurrentRange.EndDate.AddDays(1).Date;
        
        while (current <= endDate)
        {
            var position = CalculatePosition(current, totalWidth);
            var isDayStart = current.Hour == 0;
            var isSixHourMark = current.Hour % 6 == 0;
            
            TimeRulerMarks.Add(new TimeRulerMark
            {
                Date = current,
                Label = isDayStart ? current.ToString("ddd HH:mm") : (isSixHourMark ? current.ToString("HH:mm") : string.Empty),
                PositionX = position,
                LabelPositionX = position - 18,
                TickHeight = isDayStart ? 20 : (isSixHourMark ? 12 : 6),
                IsMajor = isDayStart || isSixHourMark
            });
            
            current = current.AddHours(1);
        }
    }

    private void GenerateDailyRulerMarks(double totalWidth)
    {
        // For 1 day: 15-minute ticks (small) + hourly ticks (large with labels)
        var current = CurrentRange.StartDate.Date;
        var endDate = CurrentRange.EndDate.AddDays(1).Date;
        
        while (current <= endDate)
        {
            var position = CalculatePosition(current, totalWidth);
            var isHourMark = current.Minute == 0;
            
            TimeRulerMarks.Add(new TimeRulerMark
            {
                Date = current,
                Label = isHourMark ? current.ToString("HH:mm") : string.Empty,
                PositionX = position,
                LabelPositionX = position - 15,
                TickHeight = isHourMark ? 20 : 8,
                IsMajor = isHourMark
            });
            
            current = current.AddMinutes(15);
        }
    }

    private double CalculatePosition(DateTime date, double totalWidth)
    {
        var position = (date - CurrentRange.StartDate).TotalMilliseconds / 
                      CurrentRange.Duration.TotalMilliseconds;
        return position * totalWidth;
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

    private void LoadGanttBars()
    {
        GanttBars.Clear();

        // Group items by ProcessId to create Gantt bars
        var processGroups = Items
            .Where(i => !string.IsNullOrEmpty(i.ProcessId))
            .GroupBy(i => i.ProcessId)
            .ToList();

        int swimlaneIndex = 0;
        foreach (var group in processGroups)
        {
            var processItems = group.OrderBy(i => i.Date).ToList();
            if (processItems.Count == 0) continue;

            var startDate = processItems.First().Date;
            var endDate = processItems.Last().Date;
            
            // Calculate progress based on completed items
            var completedCount = processItems.Count(i => i.Status == TimelineStatus.Completed);
            var progress = processItems.Count > 0 ? (double)completedCount / processItems.Count : 0;

            // Extract milestones
            var milestones = processItems
                .Where(i => i.ObjectType == TimelineObjectType.Milestone)
                .Select(m => new GanttMilestone
                {
                    Id = m.Id,
                    Name = m.Title,
                    Date = m.Date,
                    Symbol = m.IconCode,
                    Color = m.Color,
                    IsAchieved = m.Status == TimelineStatus.Completed,
                    Type = m.Priority == TimelinePriority.Urgent ? MilestoneType.Important : MilestoneType.Standard
                })
                .ToList();

            // Create phases (group consecutive items into phases)
            var phases = CreatePhasesFromItems(processItems);

            var ganttBar = new TimelineGanttBar
            {
                Id = Guid.NewGuid().ToString(),
                ProcessId = group.Key ?? string.Empty,
                ProcessName = processItems.FirstOrDefault()?.Title ?? "Unbekannter Prozess",
                StartDate = startDate,
                EndDate = endDate,
                Progress = progress,
                Color = GetProcessColor(swimlaneIndex),
                SwimlaneIndex = swimlaneIndex,
                Milestones = milestones,
                Phases = phases,
                Status = DetermineGanttStatus(processItems),
                AssignedTo = processItems.FirstOrDefault()?.AssignedTo,
                Department = processItems.FirstOrDefault()?.Department
            };

            GanttBars.Add(ganttBar);
            swimlaneIndex++;
        }
    }

    private List<GanttPhase> CreatePhasesFromItems(List<TimelineItem> items)
    {
        var phases = new List<GanttPhase>();
        
        // Simple phase detection: group by status transitions
        var statusGroups = new List<(TimelineStatus Status, DateTime Start, DateTime End)>();
        TimelineStatus? lastStatus = null;
        DateTime? phaseStart = null;

        foreach (var item in items.OrderBy(i => i.Date))
        {
            if (lastStatus != item.Status)
            {
                if (phaseStart.HasValue && lastStatus.HasValue)
                {
                    statusGroups.Add((lastStatus.Value, phaseStart.Value, item.Date));
                }
                lastStatus = item.Status;
                phaseStart = item.Date;
            }
        }

        // Add final phase
        if (phaseStart.HasValue && lastStatus.HasValue && items.Count > 0)
        {
            statusGroups.Add((lastStatus.Value, phaseStart.Value, items.Last().Date));
        }

        // Convert to GanttPhases
        foreach (var (status, start, end) in statusGroups)
        {
            phases.Add(new GanttPhase
            {
                Name = GetPhaseName(status),
                StartDate = start,
                EndDate = end,
                Color = GetPhaseColor(status)
            });
        }

        return phases;
    }

    private GanttBarStatus DetermineGanttStatus(List<TimelineItem> items)
    {
        if (items.All(i => i.Status == TimelineStatus.Completed)) return GanttBarStatus.Completed;
        if (items.Any(i => i.Status == TimelineStatus.Cancelled)) return GanttBarStatus.Cancelled;
        if (items.Any(i => i.Status == TimelineStatus.Overdue)) return GanttBarStatus.Delayed;
        if (items.Any(i => i.Status == TimelineStatus.InProgress)) return GanttBarStatus.InProgress;
        return GanttBarStatus.NotStarted;
    }

    private string GetProcessColor(int index)
    {
        var colors = new[]
        {
            "#3b82f6", // Blue
            "#10b981", // Green
            "#f59e0b", // Amber
            "#8b5cf6", // Purple
            "#ef4444", // Red
            "#06b6d4", // Cyan
            "#f97316", // Orange
            "#ec4899"  // Pink
        };
        return colors[index % colors.Length];
    }

    private string GetPhaseName(TimelineStatus status) => status switch
    {
        TimelineStatus.Open => "Offen",
        TimelineStatus.InProgress => "In Bearbeitung",
        TimelineStatus.Completed => "Abgeschlossen",
        TimelineStatus.Overdue => "Überfällig",
        TimelineStatus.Cancelled => "Abgebrochen",
        _ => "Unbekannt"
    };

    private string GetPhaseColor(TimelineStatus status) => status switch
    {
        TimelineStatus.Open => "#cbd5e1",
        TimelineStatus.InProgress => "#60a5fa",
        TimelineStatus.Completed => "#34d399",
        TimelineStatus.Overdue => "#f87171",
        TimelineStatus.Cancelled => "#94a3b8",
        _ => "#e5e7eb"
    };
}
