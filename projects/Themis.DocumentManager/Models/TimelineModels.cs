/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineModels.cs                                  ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     302                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Represents an item on the timeline (badge visualization)
/// </summary>
public class TimelineItem
{
    public string Id { get; set; } = string.Empty;
    public string ObjectId { get; set; } = string.Empty; // ID of underlying object (inbox, reminder, etc.)
    public TimelineObjectType ObjectType { get; set; }
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime Date { get; set; }
    public TimelinePriority Priority { get; set; }
    public TimelineStatus Status { get; set; }
    public string? AssignedTo { get; set; }
    public string? Department { get; set; }
    public string? ProcessId { get; set; } // Link to process if process-related
    public string IconCode { get; set; } = string.Empty; // Unicode icon
    public string Color { get; set; } = "#3b82f6"; // Hex color
    public bool IsProcessRelated { get; set; }
    public bool IsHighlighted { get; set; } // For process-aware mode
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Timeline object types with visual coding
/// </summary>
public enum TimelineObjectType
{
    Inbox,      // 📥 Blue
    Deadline,   // ⏰ Red
    Task,       // 📋 Orange
    Approval,   // ✅ Green
    Document,   // 📄 Turquoise
    Process,    // ⚙️ Purple
    Workflow,   // 🔄 Indigo
    Comment,    // 💬 Gray
    Milestone,  // 📌 Gold
    Alert       // ⚠️ Dark Red
}

/// <summary>
/// Timeline priority levels
/// </summary>
public enum TimelinePriority
{
    Low,
    Normal,
    High,
    Urgent
}

/// <summary>
/// Timeline item status
/// </summary>
public enum TimelineStatus
{
    Open,
    InProgress,
    Completed,
    Overdue,
    Cancelled
}

/// <summary>
/// Timeline zoom/scale levels
/// </summary>
public enum TimelineScale
{
    OneDay,      // 1 day, 1 hour segments
    ThreeDays,   // 3 days, 6 hour segments
    OneWeek,     // 1 week, 1 day segments
    TwoWeeks,    // 2 weeks, 1 day segments
    OneMonth,    // 1 month, 1 week segments
    ThreeMonths, // 3 months, 2 week segments
    SixMonths,   // 6 months, 1 month segments
    OneYear,     // 1 year, 1 month segments
    FiveYears    // 5 years, 1 year segments
}

/// <summary>
/// Timeline filter configuration
/// </summary>
public class TimelineFilter
{
    public List<TimelineObjectType> ObjectTypes { get; set; } = new();
    public List<TimelinePriority> Priorities { get; set; } = new();
    public List<TimelineStatus> Statuses { get; set; } = new();
    public DateTime? StartDate { get; set; }
    public DateTime? EndDate { get; set; }
    public List<string> AssignedUsers { get; set; } = new();
    public List<string> Departments { get; set; } = new();
    public string? ProcessId { get; set; } // Filter by specific process
    public bool ShowOnlyProcessRelated { get; set; }
    public string? SearchText { get; set; }
}

/// <summary>
/// Timeline visible range
/// </summary>
public class TimelineRange
{
    public DateTime StartDate { get; set; }
    public DateTime EndDate { get; set; }
    public TimelineScale Scale { get; set; }
    
    public TimeSpan Duration => EndDate - StartDate;
    
    public static TimelineRange FromScale(DateTime centerDate, TimelineScale scale)
    {
        return scale switch
        {
            TimelineScale.OneDay => new TimelineRange 
            { 
                StartDate = centerDate.Date, 
                EndDate = centerDate.Date.AddDays(1),
                Scale = scale
            },
            TimelineScale.ThreeDays => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddDays(-1), 
                EndDate = centerDate.Date.AddDays(2),
                Scale = scale
            },
            TimelineScale.OneWeek => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddDays(-3), 
                EndDate = centerDate.Date.AddDays(4),
                Scale = scale
            },
            TimelineScale.TwoWeeks => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddDays(-7), 
                EndDate = centerDate.Date.AddDays(7),
                Scale = scale
            },
            TimelineScale.OneMonth => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddDays(-15), 
                EndDate = centerDate.Date.AddDays(15),
                Scale = scale
            },
            TimelineScale.ThreeMonths => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddMonths(-1).AddDays(-15), 
                EndDate = centerDate.Date.AddMonths(2).AddDays(-15),
                Scale = scale
            },
            TimelineScale.SixMonths => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddMonths(-3), 
                EndDate = centerDate.Date.AddMonths(3),
                Scale = scale
            },
            TimelineScale.OneYear => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddMonths(-6), 
                EndDate = centerDate.Date.AddMonths(6),
                Scale = scale
            },
            TimelineScale.FiveYears => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddYears(-2).AddMonths(-6), 
                EndDate = centerDate.Date.AddYears(2).AddMonths(6),
                Scale = scale
            },
            _ => new TimelineRange 
            { 
                StartDate = centerDate.Date.AddMonths(-1), 
                EndDate = centerDate.Date.AddMonths(1),
                Scale = TimelineScale.OneMonth
            }
        };
    }
}

/// <summary>
/// Timeline segment (ruler marks)
/// </summary>
public class TimelineSegment
{
    public DateTime Date { get; set; }
    public string Label { get; set; } = string.Empty;
    public bool IsMajor { get; set; } // Major tick (longer line)
    public double Position { get; set; } // X position on canvas (0-1)
}

/// <summary>
/// Timeline ruler mark for visualization
/// </summary>
public class TimeRulerMark
{
    public DateTime Date { get; set; }
    public string Label { get; set; } = string.Empty;
    public double PositionX { get; set; }
    public double LabelPositionX { get; set; }
    public double TickHeight { get; set; } = 10; // Height of tick mark in pixels
    public bool IsMajor { get; set; } // Major ticks have labels
}

/// <summary>
/// Timeline configuration
/// </summary>
public class TimelineConfiguration
{
    public TimelineScale DefaultScale { get; set; } = TimelineScale.OneMonth;
    public int ItemHeight { get; set; } = 40;
    public int TimelineHeight { get; set; } = 80;
    public int SegmentSpacing { get; set; } = 100; // Pixels between segments
    public bool EnableVirtualization { get; set; } = true;
    public bool EnableGrouping { get; set; } = true; // Group items when dense
    public int GroupingThreshold { get; set; } = 5; // Items per segment before grouping
    public bool EnableAnimation { get; set; } = true;
    public int AnimationDurationMs { get; set; } = 300;
    public bool ShowTooltips { get; set; } = true;
    public bool EnableDragAndDrop { get; set; } = true;
}

/// <summary>
/// Timeline aggregation result
/// </summary>
public class TimelineAggregationResult
{
    public List<TimelineItem> Items { get; set; } = new();
    public int TotalCount { get; set; }
    public int FilteredCount { get; set; }
    public DateTime OldestDate { get; set; }
    public DateTime NewestDate { get; set; }
    public Dictionary<TimelineObjectType, int> CountByType { get; set; } = new();
    public Dictionary<TimelinePriority, int> CountByPriority { get; set; } = new();
    public Dictionary<TimelineStatus, int> CountByStatus { get; set; } = new();
}

/// <summary>
/// Timeline group (when items are clustered)
/// </summary>
public class TimelineGroup
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public DateTime Date { get; set; }
    public List<TimelineItem> Items { get; set; } = new();
    public int Count => Items.Count;
    public TimelineObjectType PrimaryType { get; set; } // Most common type in group
    public string IconCode { get; set; } = "📊";
    public string Color { get; set; } = "#6b7280";
}

/// <summary>
/// Gantt bar representing a process duration (for timeline visualization)
/// </summary>
public class TimelineGanttBar
{
    public string Id { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public string ProcessName { get; set; } = string.Empty;
    public DateTime StartDate { get; set; }
    public DateTime EndDate { get; set; }
    public double Progress { get; set; } // 0-1 (0-100%)
    public string Color { get; set; } = "#3b82f6";
    public int SwimlaneIndex { get; set; } // Y-position in swimlanes
    public List<GanttMilestone> Milestones { get; set; } = new();
    public List<GanttPhase> Phases { get; set; } = new();
    public GanttBarStatus Status { get; set; }
    public string? AssignedTo { get; set; }
    public string? Department { get; set; }
}

/// <summary>
/// Gantt bar status (for timeline)
/// </summary>
public enum GanttBarStatus
{
    NotStarted,
    InProgress,
    OnHold,
    Completed,
    Delayed,
    Cancelled
}
