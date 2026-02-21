/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineAggregationService.cs                      ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:40:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     597                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 60d127110  2025-12-09  feat: Add comprehensive test report for ThemisDB Document... ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service for aggregating all timeline items from various sources
/// </summary>
public interface ITimelineAggregationService
{
    /// <summary>
    /// Aggregate all items from all sources (inbox, reminders, processes, documents, etc.)
    /// </summary>
    Task<TimelineAggregationResult> AggregateAllItemsAsync(
        DateTime? startDate = null,
        DateTime? endDate = null,
        TimelineFilter? filter = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Aggregate items for a specific process (process-aware mode)
    /// </summary>
    Task<TimelineAggregationResult> AggregateProcessItemsAsync(
        string processId,
        DateTime? startDate = null,
        DateTime? endDate = null,
        bool showOthersDimmed = true,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get timeline items from inbox
    /// </summary>
    Task<List<TimelineItem>> GetInboxItemsAsync(
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get timeline items from reminders/deadlines
    /// </summary>
    Task<List<TimelineItem>> GetReminderItemsAsync(
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get timeline items from process events
    /// </summary>
    Task<List<TimelineItem>> GetProcessEventItemsAsync(
        string? processId = null,
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get timeline items from document events
    /// </summary>
    Task<List<TimelineItem>> GetDocumentEventItemsAsync(
        string? processId = null,
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Get timeline items from workflow steps
    /// </summary>
    Task<List<TimelineItem>> GetWorkflowItemsAsync(
        string? processId = null,
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default);

    /// <summary>
    /// Apply filters to timeline items
    /// </summary>
    List<TimelineItem> ApplyFilters(List<TimelineItem> items, TimelineFilter filter);

    /// <summary>
    /// Highlight process-related items and dim others
    /// </summary>
    void ApplyProcessHighlighting(List<TimelineItem> items, string processId);

    /// <summary>
    /// Group items by date/time segment when density is high
    /// </summary>
    List<TimelineGroup> GroupItemsByDensity(
        List<TimelineItem> items,
        TimelineRange range,
        int densityThreshold = 5);
}

public class TimelineAggregationService : ITimelineAggregationService
{
    private readonly IInboxService _inboxService;
    private readonly IReminderService _reminderService;
    private readonly IProcessTimelineService _timelineService;
    private readonly IAdministrativeStructureService _adminService;
    private readonly ICosigningService _cosigningService;

    public TimelineAggregationService(
        IInboxService inboxService,
        IReminderService reminderService,
        IProcessTimelineService timelineService,
        IAdministrativeStructureService adminService,
        ICosigningService cosigningService)
    {
        _inboxService = inboxService;
        _reminderService = reminderService;
        _timelineService = timelineService;
        _adminService = adminService;
        _cosigningService = cosigningService;
    }

    public async Task<TimelineAggregationResult> AggregateAllItemsAsync(
        DateTime? startDate = null,
        DateTime? endDate = null,
        TimelineFilter? filter = null,
        CancellationToken cancellationToken = default)
    {
        var allItems = new List<TimelineItem>();

        // Aggregate from all sources
        var inboxTask = GetInboxItemsAsync(startDate, endDate, cancellationToken);
        var reminderTask = GetReminderItemsAsync(startDate, endDate, cancellationToken);
        var processTask = GetProcessEventItemsAsync(null, startDate, endDate, cancellationToken);
        var documentTask = GetDocumentEventItemsAsync(null, startDate, endDate, cancellationToken);
        var workflowTask = GetWorkflowItemsAsync(null, startDate, endDate, cancellationToken);

        await Task.WhenAll(inboxTask, reminderTask, processTask, documentTask, workflowTask);

        allItems.AddRange(await inboxTask);
        allItems.AddRange(await reminderTask);
        allItems.AddRange(await processTask);
        allItems.AddRange(await documentTask);
        allItems.AddRange(await workflowTask);

        // Apply filters if provided
        var filteredItems = filter != null 
            ? ApplyFilters(allItems, filter) 
            : allItems;

        // Build aggregation result
        return BuildAggregationResult(allItems, filteredItems);
    }

    public async Task<TimelineAggregationResult> AggregateProcessItemsAsync(
        string processId,
        DateTime? startDate = null,
        DateTime? endDate = null,
        bool showOthersDimmed = true,
        CancellationToken cancellationToken = default)
    {
        // Get all items first
        var result = await AggregateAllItemsAsync(startDate, endDate, null, cancellationToken);

        // Apply process highlighting
        ApplyProcessHighlighting(result.Items, processId);

        // Filter out non-process items if requested
        if (!showOthersDimmed)
        {
            result.Items = result.Items.Where(i => i.IsProcessRelated).ToList();
            result.FilteredCount = result.Items.Count;
        }

        return result;
    }

    public async Task<List<TimelineItem>> GetInboxItemsAsync(
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default)
    {
        var inboxItems = await _inboxService.GetAllInboxItemsAsync();

        return inboxItems
            .Where(i => (!startDate.HasValue || i.ReceivedAt >= startDate.Value) &&
                       (!endDate.HasValue || i.ReceivedAt <= endDate.Value))
            .Select(i => new TimelineItem
            {
                Id = $"inbox-{i.Id}",
                ObjectId = i.Id,
                ObjectType = TimelineObjectType.Inbox,
                Title = i.Subject,
                Description = i.Description ?? string.Empty,
                Date = i.ReceivedAt,
                Priority = MapInboxPriority(i.Priority),
                Status = MapInboxStatus(i.Status),
                AssignedTo = i.AssignedTo,
                ProcessId = i.RelatedProcessId,
                IconCode = "📥",
                Color = "#3b82f6",
                IsProcessRelated = !string.IsNullOrEmpty(i.RelatedProcessId),
                Metadata = new Dictionary<string, object>
                {
                    ["sender"] = i.Sender ?? "Unknown",
                    ["isRead"] = i.IsRead
                }
            })
            .ToList();
    }

    public async Task<List<TimelineItem>> GetReminderItemsAsync(
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default)
    {
        var reminders = await _reminderService.GetAllRemindersAsync();

        return reminders
            .Where(r => (!startDate.HasValue || r.DueDate >= startDate.Value) &&
                       (!endDate.HasValue || r.DueDate <= endDate.Value))
            .Select(r => new TimelineItem
            {
                Id = $"reminder-{r.Id}",
                ObjectId = r.Id,
                ObjectType = TimelineObjectType.Deadline,
                Title = r.Subject,
                Description = r.Description ?? string.Empty,
                Date = r.DueDate,
                Priority = MapReminderPriority(r),
                Status = MapReminderStatus(r),
                AssignedTo = r.AssignedTo,
                ProcessId = r.ProcessId,
                IconCode = "⏰",
                Color = r.IsOverdue ? "#dc2626" : "#f97316",
                IsProcessRelated = !string.IsNullOrEmpty(r.ProcessId),
                Metadata = new Dictionary<string, object>
                {
                    ["isOverdue"] = r.IsOverdue,
                    ["isCompleted"] = r.IsCompleted,
                    ["escalationLevel"] = r.EscalationLevels?.Count ?? 0
                }
            })
            .ToList();
    }

    public async Task<List<TimelineItem>> GetProcessEventItemsAsync(
        string? processId = null,
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default)
    {
        // Get process timeline events
        var events = processId != null
            ? await _timelineService.GetEventsByProcessAsync(processId, startDate)
            : await _timelineService.GetAllEventsAsync();

        return events
            .Where(e => (!startDate.HasValue || e.Timestamp >= startDate.Value) &&
                       (!endDate.HasValue || e.Timestamp <= endDate.Value))
            .Select(e => new TimelineItem
            {
                Id = $"event-{e.Id}",
                ObjectId = e.Id,
                ObjectType = MapEventType(e.EventType),
                Title = e.EventType.ToString(),
                Description = e.Description ?? string.Empty,
                Date = e.Timestamp,
                Priority = TimelinePriority.Normal,
                Status = TimelineStatus.Completed,
                AssignedTo = e.Actor,
                ProcessId = e.ProcessId,
                IconCode = GetEventIcon(e.EventType),
                Color = GetEventColor(e.EventType),
                IsProcessRelated = !string.IsNullOrEmpty(e.ProcessId),
                Metadata = new Dictionary<string, object>
                {
                    ["eventType"] = e.EventType.ToString(),
                    ["changes"] = e.Changes ?? new Dictionary<string, object>()
                }
            })
            .ToList();
    }

    public async Task<List<TimelineItem>> GetDocumentEventItemsAsync(
        string? processId = null,
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default)
    {
        // In a real implementation, this would query document events from ThemisDB
        // For now, return empty list as placeholder
        await Task.CompletedTask;
        return new List<TimelineItem>();
    }

    public async Task<List<TimelineItem>> GetWorkflowItemsAsync(
        string? processId = null,
        DateTime? startDate = null,
        DateTime? endDate = null,
        CancellationToken cancellationToken = default)
    {
        var cosignings = await _cosigningService.GetAllCosigningsAsync();

        if (processId != null)
        {
            cosignings = cosignings.Where(c => c.ProcessId == processId).ToList();
        }

        var items = new List<TimelineItem>();

        foreach (var cosigning in cosignings)
        {
            foreach (var step in cosigning.Steps)
            {
                if (step.CompletedAt.HasValue &&
                    (!startDate.HasValue || step.CompletedAt.Value >= startDate.Value) &&
                    (!endDate.HasValue || step.CompletedAt.Value <= endDate.Value))
                {
                    items.Add(new TimelineItem
                    {
                        Id = $"workflow-{cosigning.Id}-{step.StepNumber}",
                        ObjectId = cosigning.Id,
                        ObjectType = TimelineObjectType.Workflow,
                        Title = $"Mitzeichnung: {step.CosignerRole}",
                        Description = step.Comment ?? string.Empty,
                        Date = step.CompletedAt.Value,
                        Priority = TimelinePriority.Normal,
                        Status = MapCosigningStatus(step.Status),
                        AssignedTo = step.CosignerId,
                        ProcessId = cosigning.ProcessId,
                        IconCode = GetCosigningIcon(step.Status),
                        Color = GetCosigningColor(step.Status),
                        IsProcessRelated = !string.IsNullOrEmpty(cosigning.ProcessId),
                        Metadata = new Dictionary<string, object>
                        {
                            ["status"] = step.Status.ToString(),
                            ["stepNumber"] = step.StepNumber
                        }
                    });
                }
            }
        }

        return items;
    }

    public List<TimelineItem> ApplyFilters(List<TimelineItem> items, TimelineFilter filter)
    {
        var filtered = items.AsEnumerable();

        if (filter.ObjectTypes.Any())
        {
            filtered = filtered.Where(i => filter.ObjectTypes.Contains(i.ObjectType));
        }

        if (filter.Priorities.Any())
        {
            filtered = filtered.Where(i => filter.Priorities.Contains(i.Priority));
        }

        if (filter.Statuses.Any())
        {
            filtered = filtered.Where(i => filter.Statuses.Contains(i.Status));
        }

        if (filter.StartDate.HasValue)
        {
            filtered = filtered.Where(i => i.Date >= filter.StartDate.Value);
        }

        if (filter.EndDate.HasValue)
        {
            filtered = filtered.Where(i => i.Date <= filter.EndDate.Value);
        }

        if (filter.AssignedUsers.Any())
        {
            filtered = filtered.Where(i => 
                !string.IsNullOrEmpty(i.AssignedTo) && 
                filter.AssignedUsers.Contains(i.AssignedTo));
        }

        if (filter.Departments.Any())
        {
            filtered = filtered.Where(i => 
                !string.IsNullOrEmpty(i.Department) && 
                filter.Departments.Contains(i.Department));
        }

        if (!string.IsNullOrEmpty(filter.ProcessId))
        {
            filtered = filtered.Where(i => i.ProcessId == filter.ProcessId);
        }

        if (filter.ShowOnlyProcessRelated)
        {
            filtered = filtered.Where(i => i.IsProcessRelated);
        }

        if (!string.IsNullOrEmpty(filter.SearchText))
        {
            var searchLower = filter.SearchText.ToLowerInvariant();
            filtered = filtered.Where(i => 
                i.Title.ToLowerInvariant().Contains(searchLower) ||
                i.Description.ToLowerInvariant().Contains(searchLower));
        }

        return filtered.ToList();
    }

    public void ApplyProcessHighlighting(List<TimelineItem> items, string processId)
    {
        foreach (var item in items)
        {
            item.IsHighlighted = item.ProcessId == processId;
        }
    }

    public List<TimelineGroup> GroupItemsByDensity(
        List<TimelineItem> items,
        TimelineRange range,
        int densityThreshold = 5)
    {
        var groups = new List<TimelineGroup>();
        var itemsByDate = items
            .GroupBy(i => i.Date.Date)
            .OrderBy(g => g.Key);

        foreach (var dateGroup in itemsByDate)
        {
            if (dateGroup.Count() >= densityThreshold)
            {
                // Group items
                var group = new TimelineGroup
                {
                    Date = dateGroup.Key,
                    Items = dateGroup.ToList(),
                    PrimaryType = dateGroup
                        .GroupBy(i => i.ObjectType)
                        .OrderByDescending(g => g.Count())
                        .First()
                        .Key
                };

                groups.Add(group);
            }
        }

        return groups;
    }

    // Helper methods for mapping
    private TimelinePriority MapInboxPriority(InboxPriority priority) => priority switch
    {
        InboxPriority.Low => TimelinePriority.Low,
        InboxPriority.Normal => TimelinePriority.Normal,
        InboxPriority.High => TimelinePriority.High,
        InboxPriority.Urgent => TimelinePriority.Urgent,
        _ => TimelinePriority.Normal
    };

    private TimelineStatus MapInboxStatus(InboxStatus status) => status switch
    {
        InboxStatus.New => TimelineStatus.Open,
        InboxStatus.Assigned => TimelineStatus.Open,
        InboxStatus.InProgress => TimelineStatus.InProgress,
        InboxStatus.Completed => TimelineStatus.Completed,
        InboxStatus.Archived => TimelineStatus.Completed,
        _ => TimelineStatus.Open
    };

    private TimelinePriority MapReminderPriority(Reminder reminder)
    {
        if (reminder.IsOverdue) return TimelinePriority.Urgent;
        if (reminder.DueDate <= DateTime.UtcNow.AddDays(1)) return TimelinePriority.High;
        return TimelinePriority.Normal;
    }

    private TimelineStatus MapReminderStatus(Reminder reminder)
    {
        if (reminder.IsCompleted) return TimelineStatus.Completed;
        if (reminder.IsOverdue) return TimelineStatus.Overdue;
        return TimelineStatus.Open;
    }

    private TimelineStatus MapCosigningStatus(CosigningStepStatus status) => status switch
    {
        CosigningStepStatus.Pending => TimelineStatus.Open,
        CosigningStepStatus.Approved => TimelineStatus.Completed,
        CosigningStepStatus.Rejected => TimelineStatus.Cancelled,
        CosigningStepStatus.Skipped => TimelineStatus.Cancelled,
        _ => TimelineStatus.Open
    };

    private TimelineObjectType MapEventType(ProcessEventType eventType)
    {
        // Map process event types to timeline object types
        var eventStr = eventType.ToString().ToLowerInvariant();
        if (eventStr.Contains("document")) return TimelineObjectType.Document;
        if (eventStr.Contains("workflow")) return TimelineObjectType.Workflow;
        if (eventStr.Contains("comment")) return TimelineObjectType.Comment;
        if (eventStr.Contains("approved") || eventStr.Contains("signed")) return TimelineObjectType.Approval;
        return TimelineObjectType.Process;
    }

    private string GetEventIcon(ProcessEventType eventType) => eventType.ToString().ToLowerInvariant() switch
    {
        var s when s.Contains("created") => "➕",
        var s when s.Contains("opened") => "📂",
        var s when s.Contains("closed") => "📁",
        var s when s.Contains("approved") => "✅",
        var s when s.Contains("rejected") => "❌",
        var s when s.Contains("signed") => "✍️",
        var s when s.Contains("comment") => "💬",
        var s when s.Contains("document") => "📄",
        _ => "⚙️"
    };

    private string GetEventColor(ProcessEventType eventType) => eventType.ToString().ToLowerInvariant() switch
    {
        var s when s.Contains("approved") => "#22c55e",
        var s when s.Contains("rejected") => "#dc2626",
        var s when s.Contains("document") => "#14b8a6",
        var s when s.Contains("comment") => "#6b7280",
        _ => "#a855f7"
    };

    private string GetCosigningIcon(CosigningStepStatus status) => status switch
    {
        CosigningStepStatus.Approved => "✅",
        CosigningStepStatus.Rejected => "❌",
        CosigningStepStatus.Skipped => "⏭️",
        _ => "🔄"
    };

    private string GetCosigningColor(CosigningStepStatus status) => status switch
    {
        CosigningStepStatus.Approved => "#22c55e",
        CosigningStepStatus.Rejected => "#dc2626",
        CosigningStepStatus.Skipped => "#6b7280",
        _ => "#6366f1"
    };

    private TimelineAggregationResult BuildAggregationResult(
        List<TimelineItem> allItems,
        List<TimelineItem> filteredItems)
    {
        var result = new TimelineAggregationResult
        {
            Items = filteredItems,
            TotalCount = allItems.Count,
            FilteredCount = filteredItems.Count
        };

        if (filteredItems.Any())
        {
            result.OldestDate = filteredItems.Min(i => i.Date);
            result.NewestDate = filteredItems.Max(i => i.Date);

            result.CountByType = filteredItems
                .GroupBy(i => i.ObjectType)
                .ToDictionary(g => g.Key, g => g.Count());

            result.CountByPriority = filteredItems
                .GroupBy(i => i.Priority)
                .ToDictionary(g => g.Key, g => g.Count());

            result.CountByStatus = filteredItems
                .GroupBy(i => i.Status)
                .ToDictionary(g => g.Key, g => g.Count());
        }

        return result;
    }
}
