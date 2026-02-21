/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetMyTasksQueryHandler.cs                          ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     257                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a89e05f8c  2026-01-06  Add Tasks tab to right sidebar with drag&drop, search, an... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Application.Tasks.Queries.GetMyTasks;

/// <summary>
/// Handler for GetMyTasksQuery
/// Retrieves and filters tasks based on query parameters
/// </summary>
public class GetMyTasksQueryHandler : IRequestHandler<GetMyTasksQuery, List<TaskItem>>
{
    private readonly IInboxService _inboxService;
    private readonly IReminderService _reminderService;
    private readonly ICosigningService _cosigningService;

    public GetMyTasksQueryHandler(
        IInboxService inboxService,
        IReminderService reminderService,
        ICosigningService cosigningService)
    {
        _inboxService = inboxService;
        _reminderService = reminderService;
        _cosigningService = cosigningService;
    }

    public async Task<List<TaskItem>> Handle(GetMyTasksQuery request, CancellationToken cancellationToken)
    {
        var tasks = new List<TaskItem>();

        // Get inbox items
        var inboxItems = await _inboxService.GetMyInboxItemsAsync(request.UserId);
        tasks.AddRange(inboxItems.Select(item => new TaskItem
        {
            Id = item.Id,
            Title = item.Subject,
            Description = item.Notes,
            DueDate = null, // Inbox items don't have due dates
            Priority = MapInboxPriority(item.Priority),
            Status = MapInboxStatus(item.Status),
            Category = "Posteingang",
            Type = TaskType.Inbox,
            SourceId = item.Id,
            AssignedTo = item.AssignedTo,
            CreatedAt = item.ReceivedAt,
            EntityType = null,
            EntityId = null,
            ProcessId = null
        }));

        // Get reminders
        var reminders = await _reminderService.GetRemindersByUserAsync(request.UserId);
        tasks.AddRange(reminders.Select(reminder => new TaskItem
        {
            Id = reminder.Id,
            Title = $"Frist: {reminder.Description}",
            Description = reminder.Description,
            DueDate = reminder.DueDate,
            Priority = TaskPriority.Normal,
            Status = MapReminderStatus(reminder.Status),
            Category = "Wiedervorlage",
            Type = TaskType.Reminder,
            SourceId = reminder.Id,
            AssignedTo = reminder.AssignedTo,
            CreatedAt = reminder.CreatedAt,
            EntityType = null,
            EntityId = null,
            ProcessId = null
        }));

        // Get cosigning tasks
        var cosignings = await _cosigningService.GetPendingCosigningsForUserAsync(request.UserId);
        tasks.AddRange(cosignings.Select(cosigning => new TaskItem
        {
            Id = cosigning.Id,
            Title = "Mitzeichnung erforderlich",
            Description = $"Prozess: {cosigning.ProcessId}",
            DueDate = null,
            Priority = TaskPriority.High,
            Status = TaskStatus.Pending,
            Category = "Mitzeichnung",
            Type = TaskType.Cosigning,
            SourceId = cosigning.Id,
            AssignedTo = request.UserId,
            CreatedAt = DateTime.UtcNow, // Would need actual creation date
            EntityType = LinkedEntityType.Process,
            EntityId = cosigning.ProcessId,
            ProcessId = cosigning.ProcessId
        }));

        // Apply filters
        if (request.StatusFilter.HasValue)
        {
            tasks = tasks.Where(t => t.Status == request.StatusFilter.Value).ToList();
        }

        if (request.PriorityFilter.HasValue)
        {
            tasks = tasks.Where(t => t.Priority == request.PriorityFilter.Value).ToList();
        }

        if (!string.IsNullOrEmpty(request.CategoryFilter))
        {
            tasks = tasks.Where(t => t.Category == request.CategoryFilter).ToList();
        }

        // Filter by entity / process
        if (!string.IsNullOrEmpty(request.EntityId))
        {
            tasks = tasks.Where(t => string.Equals(t.EntityId, request.EntityId, StringComparison.OrdinalIgnoreCase)
                                   || string.Equals(t.ProcessId, request.EntityId, StringComparison.OrdinalIgnoreCase))
                         .ToList();
        }

        if (request.EntityType.HasValue)
        {
            tasks = tasks.Where(t => t.EntityType == request.EntityType.Value).ToList();
        }

        if (!string.IsNullOrEmpty(request.ProcessId))
        {
            tasks = tasks.Where(t => string.Equals(t.ProcessId, request.ProcessId, StringComparison.OrdinalIgnoreCase)).ToList();
        }

        // Apply sorting
        tasks = request.SortBy switch
        {
            TaskSortBy.DueDate => request.SortDescending 
                ? tasks.OrderByDescending(t => t.DueDate).ToList()
                : tasks.OrderBy(t => t.DueDate).ToList(),
            TaskSortBy.Priority => request.SortDescending
                ? tasks.OrderByDescending(t => t.Priority).ToList()
                : tasks.OrderBy(t => t.Priority).ToList(),
            TaskSortBy.CreatedDate => request.SortDescending
                ? tasks.OrderByDescending(t => t.CreatedAt).ToList()
                : tasks.OrderBy(t => t.CreatedAt).ToList(),
            TaskSortBy.Title => request.SortDescending
                ? tasks.OrderByDescending(t => t.Title).ToList()
                : tasks.OrderBy(t => t.Title).ToList(),
            TaskSortBy.Category => request.SortDescending
                ? tasks.OrderByDescending(t => t.Category).ToList()
                : tasks.OrderBy(t => t.Category).ToList(),
            _ => tasks
        };

        return tasks;
    }

    private TaskPriority MapInboxPriority(InboxPriority priority) => priority switch
    {
        InboxPriority.Low => TaskPriority.Low,
        InboxPriority.Normal => TaskPriority.Normal,
        InboxPriority.High => TaskPriority.High,
        InboxPriority.Urgent => TaskPriority.Urgent,
        _ => TaskPriority.Normal
    };

    private TaskStatus MapInboxStatus(InboxStatus status) => status switch
    {
        InboxStatus.New => TaskStatus.Pending,
        InboxStatus.Assigned => TaskStatus.InProgress,
        InboxStatus.InProgress => TaskStatus.InProgress,
        InboxStatus.Completed => TaskStatus.Completed,
        InboxStatus.Archived => TaskStatus.Completed,
        _ => TaskStatus.Pending
    };

    private TaskStatus MapReminderStatus(ReminderStatus status) => status switch
    {
        ReminderStatus.Active => TaskStatus.Pending,
        ReminderStatus.Completed => TaskStatus.Completed,
        ReminderStatus.Cancelled => TaskStatus.Cancelled,
        ReminderStatus.Overdue => TaskStatus.Overdue,
        _ => TaskStatus.Pending
    };
}

// Task item model for unified task view
public class TaskItem
{
    public string Id { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public DateTime? DueDate { get; set; }
    public TaskPriority Priority { get; set; }
    public TaskStatus Status { get; set; }
    public string Category { get; set; } = string.Empty;
    public TaskType Type { get; set; }
    public string SourceId { get; set; } = string.Empty;
    public string AssignedTo { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
    public LinkedEntityType? EntityType { get; set; }
    public string? EntityId { get; set; }
    public string? ProcessId { get; set; }
    public bool IsOverdue => DueDate.HasValue && DueDate.Value < DateTime.Now && Status != TaskStatus.Completed;
    
    /// <summary>
    /// Indicates if this task is related to the currently selected entity (for highlighting)
    /// </summary>
    public bool IsRelatedToCurrentEntity { get; set; }
    
    /// <summary>
    /// ID of the linked entity (Document, Case, File, etc.)
    /// </summary>
    public string? LinkedEntityId => EntityId;
}

public enum TaskType
{
    Inbox,
    Reminder,
    Cosigning,
    General
}

public enum TaskStatus
{
    Pending,
    InProgress,
    Completed,
    Cancelled,
    Overdue
}

public enum TaskPriority
{
    Low,
    Normal,
    High,
    Urgent
}
