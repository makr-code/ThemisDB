/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineRulerViewModel.cs                          ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:40:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     310                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.ViewModels;

public partial class TimelineRulerViewModel : ObservableObject
{
    private readonly ITimelineAggregationService? _timelineService;
    
    [ObservableProperty]
    private ObservableCollection<RulerTick> _rulerTicks = new();
    
    [ObservableProperty]
    private ObservableCollection<CompactProcessBar> _compactProcessBars = new();
    
    [ObservableProperty]
    private ObservableCollection<EventMarker> _eventMarkers = new();
    
    [ObservableProperty]
    private DateTime _centerDate = DateTime.Now;
    
    [ObservableProperty]
    private TimelineScale _currentScale = TimelineScale.OneMonth;
    
    [ObservableProperty]
    private string _currentDateRange = string.Empty;
    
    [ObservableProperty]
    private double _todayPositionX;
    
    [ObservableProperty]
    private bool _showTodayMarker = true;
    
    [ObservableProperty]
    private ObservableCollection<TimelineScale> _availableScales = new()
    {
        TimelineScale.OneWeek,
        TimelineScale.TwoWeeks,
        TimelineScale.OneMonth,
        TimelineScale.ThreeMonths,
        TimelineScale.SixMonths
    };

    public TimelineRulerViewModel()
    {
        _timelineService = App.GetService<ITimelineAggregationService>();
        UpdateDateRangeLabel();
    }

    public async Task InitializeAsync()
    {
        await LoadRulerDataAsync();
    }

    [RelayCommand]
    private async Task NavigatePreviousAsync()
    {
        CenterDate = CurrentScale switch
        {
            TimelineScale.OneWeek => CenterDate.AddDays(-7),
            TimelineScale.TwoWeeks => CenterDate.AddDays(-14),
            TimelineScale.OneMonth => CenterDate.AddMonths(-1),
            TimelineScale.ThreeMonths => CenterDate.AddMonths(-3),
            TimelineScale.SixMonths => CenterDate.AddMonths(-6),
            _ => CenterDate.AddMonths(-1)
        };
        await LoadRulerDataAsync();
    }

    [RelayCommand]
    private async Task NavigateNextAsync()
    {
        CenterDate = CurrentScale switch
        {
            TimelineScale.OneWeek => CenterDate.AddDays(7),
            TimelineScale.TwoWeeks => CenterDate.AddDays(14),
            TimelineScale.OneMonth => CenterDate.AddMonths(1),
            TimelineScale.ThreeMonths => CenterDate.AddMonths(3),
            TimelineScale.SixMonths => CenterDate.AddMonths(6),
            _ => CenterDate.AddMonths(1)
        };
        await LoadRulerDataAsync();
    }

    [RelayCommand]
    private async Task JumpToTodayAsync()
    {
        CenterDate = DateTime.Now;
        await LoadRulerDataAsync();
    }

    partial void OnCurrentScaleChanged(TimelineScale value)
    {
        _ = LoadRulerDataAsync();
    }

    private async Task LoadRulerDataAsync()
    {
        UpdateDateRangeLabel();
        
        var range = TimelineRange.FromScale(CenterDate, CurrentScale);
        var canvasWidth = 1200.0; // Logical width
        
        // Generate ruler ticks
        RulerTicks.Clear();
        var tickInterval = GetTickInterval(CurrentScale);
        var current = range.StartDate;
        
        while (current <= range.EndDate)
        {
            var position = CalculatePosition(current, range, canvasWidth);
            var isMajor = IsMajorTick(current, CurrentScale);
            
            RulerTicks.Add(new RulerTick
            {
                Date = current,
                Label = FormatTickLabel(current, CurrentScale, isMajor),
                PositionX = position,
                LabelPositionX = position - 15,
                TickHeight = isMajor ? 12 : 6,
                IsMajor = isMajor
            });
            
            current = current.Add(tickInterval);
        }
        
        // Calculate today position
        if (DateTime.Now >= range.StartDate && DateTime.Now <= range.EndDate)
        {
            TodayPositionX = CalculatePosition(DateTime.Now, range, canvasWidth);
            ShowTodayMarker = true;
        }
        else
        {
            ShowTodayMarker = false;
        }
        
        // Load compact data
        await LoadCompactDataAsync(range, canvasWidth);
    }

    private async Task LoadCompactDataAsync(TimelineRange range, double canvasWidth)
    {
        if (_timelineService == null) return;
        
        try
        {
            var result = await _timelineService.AggregateAllItemsAsync(
                range.StartDate, range.EndDate, null);
            
            // Create compact process bars
            CompactProcessBars.Clear();
            var processGroups = result.Items
                .Where(i => !string.IsNullOrEmpty(i.ProcessId))
                .GroupBy(i => i.ProcessId);
            
            foreach (var group in processGroups)
            {
                var items = group.OrderBy(i => i.Date).ToList();
                if (items.Count == 0) continue;
                
                var startDate = items.First().Date;
                var endDate = items.Last().Date;
                var startX = CalculatePosition(startDate, range, canvasWidth);
                var endX = CalculatePosition(endDate, range, canvasWidth);
                
                CompactProcessBars.Add(new CompactProcessBar
                {
                    ProcessId = group.Key ?? string.Empty,
                    ProcessName = items.First().Title,
                    StartX = startX,
                    Width = Math.Max(endX - startX, 4),
                    Color = items.First().Color
                });
            }
            
            // Create event markers (deadlines, milestones)
            EventMarkers.Clear();
            var importantEvents = result.Items
                .Where(i => i.ObjectType == TimelineObjectType.Deadline || 
                           i.ObjectType == TimelineObjectType.Milestone ||
                           i.Priority == TimelinePriority.Urgent)
                .Take(50); // Limit to avoid clutter
            
            foreach (var evt in importantEvents)
            {
                var posX = CalculatePosition(evt.Date, range, canvasWidth);
                EventMarkers.Add(new EventMarker
                {
                    Id = evt.Id,
                    Title = evt.Title,
                    Date = evt.Date,
                    PositionX = posX - 3,
                    Color = evt.Color
                });
            }
        }
        catch
        {
            // Fallback if service not available
        }
    }

    private double CalculatePosition(DateTime date, TimelineRange range, double width)
    {
        var totalDuration = range.Duration.TotalMilliseconds;
        var offset = (date - range.StartDate).TotalMilliseconds;
        return (offset / totalDuration) * width;
    }

    private TimeSpan GetTickInterval(TimelineScale scale) => scale switch
    {
        TimelineScale.OneWeek => TimeSpan.FromDays(1),
        TimelineScale.TwoWeeks => TimeSpan.FromDays(2),
        TimelineScale.OneMonth => TimeSpan.FromDays(7),
        TimelineScale.ThreeMonths => TimeSpan.FromDays(14),
        TimelineScale.SixMonths => TimeSpan.FromDays(30),
        _ => TimeSpan.FromDays(7)
    };

    private bool IsMajorTick(DateTime date, TimelineScale scale) => scale switch
    {
        TimelineScale.OneWeek => date.DayOfWeek == DayOfWeek.Monday,
        TimelineScale.TwoWeeks => date.DayOfWeek == DayOfWeek.Monday,
        TimelineScale.OneMonth => date.Day == 1 || date.Day == 15,
        TimelineScale.ThreeMonths => date.Day == 1,
        TimelineScale.SixMonths => date.Day == 1,
        _ => true
    };

    private string FormatTickLabel(DateTime date, TimelineScale scale, bool isMajor)
    {
        if (!isMajor) return string.Empty;
        
        return scale switch
        {
            TimelineScale.OneWeek => date.ToString("dd.MM"),
            TimelineScale.TwoWeeks => date.ToString("dd.MM"),
            TimelineScale.OneMonth => date.Day == 1 ? date.ToString("MMM") : date.ToString("dd"),
            TimelineScale.ThreeMonths => date.ToString("MMM"),
            TimelineScale.SixMonths => date.ToString("MMM yy"),
            _ => date.ToString("dd.MM")
        };
    }

    private void UpdateDateRangeLabel()
    {
        var range = TimelineRange.FromScale(CenterDate, CurrentScale);
        CurrentDateRange = $"{range.StartDate:dd.MM.yyyy} - {range.EndDate:dd.MM.yyyy}";
    }
}

// Helper models for ruler visualization
public class RulerTick
{
    public DateTime Date { get; set; }
    public string Label { get; set; } = string.Empty;
    public double PositionX { get; set; }
    public double LabelPositionX { get; set; }
    public double TickHeight { get; set; }
    public bool IsMajor { get; set; }
}

public class CompactProcessBar
{
    public string ProcessId { get; set; } = string.Empty;
    public string ProcessName { get; set; } = string.Empty;
    public double StartX { get; set; }
    public double Width { get; set; }
    public string Color { get; set; } = "#3b82f6";
}

public class EventMarker
{
    public string Id { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public DateTime Date { get; set; }
    public double PositionX { get; set; }
    public string Color { get; set; } = "#ef4444";
}
