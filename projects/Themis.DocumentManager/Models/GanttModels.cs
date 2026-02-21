/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GanttModels.cs                                     ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     301                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Gantt chart for multi-process visualization
/// </summary>
public class GanttChart
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string URN { get; set; } = string.Empty; // urn:themis:gantt:{id}
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime StartDate { get; set; }
    public DateTime EndDate { get; set; }
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
    
    public List<GanttTask> Tasks { get; set; } = new();
    public List<GanttMilestone> Milestones { get; set; } = new();
    public List<GanttDependency> Dependencies { get; set; } = new();
    public List<GanttResource> Resources { get; set; } = new();
    public List<GanttPhase> Phases { get; set; } = new();
    
    public List<string> ProcessIds { get; set; } = new(); // Linked processes
    public GanttCriticalPath? CriticalPath { get; set; }
    
    public GanttViewSettings ViewSettings { get; set; } = new();
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Gantt task (individual work item)
/// </summary>
public class GanttTask
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string ChartId { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime StartDate { get; set; }
    public DateTime EndDate { get; set; }
    public DateTime? ActualStartDate { get; set; }
    public DateTime? ActualEndDate { get; set; }
    public int Progress { get; set; } // 0-100
    public TaskStatus Status { get; set; } = TaskStatus.NotStarted;
    public int Priority { get; set; } = 5; // 1-10
    
    public string? ParentTaskId { get; set; } // For hierarchical tasks
    public string? PhaseId { get; set; }
    public string? ProcessId { get; set; } // Link to administrative process
    
    public List<GanttResource> AssignedResources { get; set; } = new();
    public string Color { get; set; } = "#3b82f6";
    
    // Critical path analysis
    public bool IsCritical { get; set; }
    public TimeSpan SlackTime { get; set; }
    public DateTime EarliestStart { get; set; }
    public DateTime EarliestFinish { get; set; }
    public DateTime LatestStart { get; set; }
    public DateTime LatestFinish { get; set; }
    
    public Dictionary<string, object> CustomFields { get; set; } = new();
}

/// <summary>
/// Task status enumeration
/// </summary>
public enum TaskStatus
{
    NotStarted,
    InProgress,
    OnHold,
    Completed,
    Cancelled,
    Overdue
}

/// <summary>
/// Gantt milestone (zero-duration event)
/// </summary>
public class GanttMilestone
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string ChartId { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime Date { get; set; }
    public MilestoneType Type { get; set; } = MilestoneType.Standard;
    public bool IsAchieved { get; set; }
    public DateTime? AchievedDate { get; set; }
    public string Symbol { get; set; } = "◆";
    public string Color { get; set; } = "#f59e0b";
    public bool IsCritical { get; set; }
}

/// <summary>
/// Milestone types
/// </summary>
public enum MilestoneType
{
    Standard,
    Important,
    AtRisk,
    Achieved
}

/// <summary>
/// Gantt dependency (relationship between tasks)
/// </summary>
public class GanttDependency
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string ChartId { get; set; } = string.Empty;
    public string PredecessorTaskId { get; set; } = string.Empty;
    public string SuccessorTaskId { get; set; } = string.Empty;
    public DependencyType Type { get; set; } = DependencyType.FinishToStart;
    public int LagDays { get; set; } // Can be negative for lead time
    public bool IsCritical { get; set; }
    public string Label { get; set; } = string.Empty;
}

/// <summary>
/// Dependency types
/// </summary>
public enum DependencyType
{
    FinishToStart,  // Predecessor must finish before successor can start (most common)
    StartToStart,   // Successor can start when predecessor starts
    FinishToFinish, // Successor can finish when predecessor finishes
    StartToFinish   // Successor can finish when predecessor starts (rare)
}

/// <summary>
/// Gantt resource (person, equipment, material, etc.)
/// </summary>
public class GanttResource
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Name { get; set; } = string.Empty;
    public ResourceType Type { get; set; } = ResourceType.Person;
    public decimal AllocationPercentage { get; set; } // 0-100
    public decimal Capacity { get; set; } // Hours per day
    public decimal CostPerHour { get; set; }
    public string Department { get; set; } = string.Empty;
    public string Email { get; set; } = string.Empty;
    public List<GanttResourceAllocation> Allocations { get; set; } = new();
}

/// <summary>
/// Resource types
/// </summary>
public enum ResourceType
{
    Person,
    Team,
    Equipment,
    Material,
    Budget,
    External
}

/// <summary>
/// Resource allocation to tasks
/// </summary>
public class GanttResourceAllocation
{
    public string TaskId { get; set; } = string.Empty;
    public string ResourceId { get; set; } = string.Empty;
    public decimal AllocationPercentage { get; set; } // 0-100
    public DateTime StartDate { get; set; }
    public DateTime EndDate { get; set; }
    public decimal AllocatedHours { get; set; }
    public decimal ActualHours { get; set; }
}

/// <summary>
/// Gantt phase (group of related tasks)
/// </summary>
public class GanttPhase
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string ChartId { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime StartDate { get; set; }
    public DateTime EndDate { get; set; }
    public int Order { get; set; }
    public string Color { get; set; } = "#6366f1";
    public List<string> TaskIds { get; set; } = new();
}

/// <summary>
/// Critical path calculation result
/// </summary>
public class GanttCriticalPath
{
    public string ChartId { get; set; } = string.Empty;
    public List<string> TaskIds { get; set; } = new(); // Tasks on critical path
    public TimeSpan TotalDuration { get; set; }
    public DateTime EarliestCompletion { get; set; }
    public DateTime LatestCompletion { get; set; }
    public List<GanttTask> Tasks { get; set; } = new();
    public decimal ScheduleRisk { get; set; } // 0-1, higher = riskier
}

/// <summary>
/// Gantt view settings
/// </summary>
public class GanttViewSettings
{
    public GanttTimeScale TimeScale { get; set; } = GanttTimeScale.Days;
    public bool ShowCriticalPath { get; set; } = true;
    public bool ShowDependencies { get; set; } = true;
    public bool ShowResources { get; set; } = true;
    public bool ShowProgress { get; set; } = true;
    public bool ShowBaseline { get; set; } = false;
    public bool GroupByPhase { get; set; } = true;
    public bool ShowWeekends { get; set; } = false;
    public bool ShowToday { get; set; } = true;
}

/// <summary>
/// Time scale for Gantt chart
/// </summary>
public enum GanttTimeScale
{
    Hours,
    Days,
    Weeks,
    Months,
    Quarters,
    Years
}

/// <summary>
/// Gantt export options
/// </summary>
public class GanttExportOptions
{
    public GanttExportFormat Format { get; set; } = GanttExportFormat.PDF;
    public bool IncludeCriticalPath { get; set; } = true;
    public bool IncludeResources { get; set; } = true;
    public bool IncludeDependencies { get; set; } = true;
    public string? Filename { get; set; }
    public GanttTimeScale TimeScale { get; set; } = GanttTimeScale.Days;
}

/// <summary>
/// Export format options
/// </summary>
public enum GanttExportFormat
{
    PDF,
    Excel,
    CSV,
    PNG,
    SVG,
    MSProject,
    Primavera
}

/// <summary>
/// Gantt filter for querying
/// </summary>
public class GanttChartFilter
{
    public string? CreatedBy { get; set; }
    public DateTime? StartDateAfter { get; set; }
    public DateTime? StartDateBefore { get; set; }
    public List<string>? ProcessIds { get; set; }
    public string? SearchText { get; set; }
}
