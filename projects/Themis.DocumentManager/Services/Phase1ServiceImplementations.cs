/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Phase1ServiceImplementations.cs                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     777                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 60d127110  2025-12-09  feat: Add comprehensive test report for ThemisDB Document... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Phase 1 VIS-Features: Service Implementations
/// </summary>

#region Inbox Service Implementation

public class InboxService : IInboxService
{
    private readonly IThemisApiClient _apiClient;
    private readonly INotificationService _notificationService;

    public InboxService(IThemisApiClient apiClient, INotificationService notificationService)
    {
        _apiClient = apiClient;
        _notificationService = notificationService;
    }

    public async Task<InboxItem> CreateInboxItemAsync(InboxItem item)
    {
        item.Id = item.Id == string.Empty ? Guid.NewGuid().ToString() : item.Id;
        item.ReceivedAt = DateTime.UtcNow;
        item.Status = InboxStatus.New;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{item.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(item) }
        );

        return item;
    }

    public async Task<InboxItem?> GetInboxItemByIdAsync(string id)
    {
        var urn = $"urn:themis:inbox:{id}";
        return await _apiClient.GetAsync<InboxItem>($"/entities/{urn}");
    }

    public async Task<InboxItem?> GetInboxItemAsync(string id)
    {
        return await GetInboxItemByIdAsync(id);
    }

    public async Task<IEnumerable<InboxItem>> GetAllInboxItemsAsync()
    {
        var query = "FOR item IN inbox_items SORT item.receivedAt DESC RETURN item";
        var response = await _apiClient.PostAsync<object, QueryResponse<InboxItem>>(
            "/query/aql",
            new { query, bindVars = new { } }
        );
        return response?.Results ?? Enumerable.Empty<InboxItem>();
    }

    public async Task<IEnumerable<InboxItem>> GetInboxItemsAsync(InboxStatus? status = null, string? assignedTo = null)
    {
        var query = "FOR item IN inbox_items";
        var filters = new List<string>();
        var bindVars = new Dictionary<string, object>();

        if (status.HasValue)
        {
            filters.Add("item.status == @status");
            bindVars["status"] = status.Value.ToString();
        }

        if (!string.IsNullOrEmpty(assignedTo))
        {
            filters.Add("item.assignedTo == @assignedTo");
            bindVars["assignedTo"] = assignedTo;
        }

        if (filters.Any())
            query += " FILTER " + string.Join(" AND ", filters);

        query += " SORT item.receivedAt DESC RETURN item";

        var response = await _apiClient.PostAsync<object, QueryResponse<InboxItem>>(
            "/query/aql",
            new { query, bindVars }
        );

        return response?.Results ?? Enumerable.Empty<InboxItem>();
    }

    public async Task<IEnumerable<InboxItem>> GetMyInboxItemsAsync(string userId)
    {
        return await GetInboxItemsAsync(assignedTo: userId);
    }

    public async Task<bool> AssignInboxItemAsync(string itemId, string assignedTo, string assignedBy)
    {
        var item = await GetInboxItemByIdAsync(itemId);
        if (item == null) return false;

        item.AssignedTo = assignedTo;
        item.AssignedBy = assignedBy;
        item.AssignedAt = DateTime.UtcNow;
        item.Status = InboxStatus.Assigned;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{item.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(item) }
        );

        // Send notification
        await _notificationService.SendTaskAssignedAsync(assignedTo, item.DocumentId, assignedBy);

        return true;
    }

    public async Task<bool> UpdateInboxStatusAsync(string itemId, InboxStatus status)
    {
        var item = await GetInboxItemByIdAsync(itemId);
        if (item == null) return false;

        item.Status = status;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{item.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(item) }
        );

        return true;
    }

    public async Task<bool> MarkAsReadAsync(string itemId)
    {
        var item = await GetInboxItemByIdAsync(itemId);
        if (item == null) return false;

        item.IsRead = true;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{item.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(item) }
        );

        return true;
    }

    public async Task<bool> UpdateInboxItemStatusAsync(string itemId, InboxStatus status)
    {
        return await UpdateInboxStatusAsync(itemId, status);
    }

    public async Task<bool> DeleteInboxItemAsync(string itemId)
    {
        var urn = $"urn:themis:inbox:{itemId}";
        return await _apiClient.DeleteAsync($"/entities/{urn}");
    }

    public async Task<bool> UpdateInboxPriorityAsync(string itemId, InboxPriority priority)
    {
        var item = await GetInboxItemByIdAsync(itemId);
        if (item == null) return false;

        item.Priority = priority;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{item.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(item) }
        );

        return true;
    }

    public async Task<int> GetUnreadCountAsync(string userId)
    {
        var items = await GetInboxItemsAsync(InboxStatus.New, userId);
        return items.Count();
    }

    private class QueryResponse<T>
    {
        public List<T> Results { get; set; } = new();
    }
}

#endregion

#region Reminder Service Implementation

public class ReminderService : IReminderService
{
    private readonly IThemisApiClient _apiClient;
    private readonly INotificationService _notificationService;
    private readonly IProcessTimelineService _timelineService;

    public ReminderService(
        IThemisApiClient apiClient,
        INotificationService notificationService,
        IProcessTimelineService timelineService)
    {
        _apiClient = apiClient;
        _notificationService = notificationService;
        _timelineService = timelineService;
    }

    public async Task<Reminder> CreateReminderAsync(Reminder reminder)
    {
        reminder.Id = reminder.Id == string.Empty ? Guid.NewGuid().ToString() : reminder.Id;
        reminder.CreatedAt = DateTime.UtcNow;
        reminder.Status = ReminderStatus.Active;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{reminder.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(reminder) }
        );

        // Create timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            ProcessId = reminder.ProcessId,
            FileId = reminder.FileId,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.DeadlineChanged,
            Description = $"Frist gesetzt: {reminder.Subject} (Fällig: {reminder.DueDate:yyyy-MM-dd})",
            Actor = reminder.CreatedBy
        });

        return reminder;
    }

    public async Task<Reminder?> GetReminderByIdAsync(string id)
    {
        var urn = $"urn:themis:reminder:{id}";
        return await _apiClient.GetAsync<Reminder>($"/entities/{urn}");
    }

    public async Task<IEnumerable<Reminder>> GetAllRemindersAsync()
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<Reminder>>(
            "/query/aql",
            new
            {
                query = "FOR reminder IN reminders SORT reminder.dueDate ASC RETURN reminder",
                bindVars = new { }
            }
        );

        return response?.Results ?? Enumerable.Empty<Reminder>();
    }

    public async Task<IEnumerable<Reminder>> GetRemindersByProcessAsync(string processId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<Reminder>>(
            "/query/aql",
            new
            {
                query = "FOR reminder IN reminders FILTER reminder.processId == @processId SORT reminder.dueDate ASC RETURN reminder",
                bindVars = new { processId }
            }
        );

        return response?.Results ?? Enumerable.Empty<Reminder>();
    }

    public async Task<IEnumerable<Reminder>> GetRemindersByUserAsync(string userId, ReminderStatus? status = null)
    {
        var query = "FOR reminder IN reminders FILTER reminder.assignedTo == @userId";
        var bindVars = new Dictionary<string, object> { ["userId"] = userId };

        if (status.HasValue)
        {
            query += " AND reminder.status == @status";
            bindVars["status"] = status.Value.ToString();
        }

        query += " SORT reminder.dueDate ASC RETURN reminder";

        var response = await _apiClient.PostAsync<object, QueryResponse<Reminder>>(
            "/query/aql",
            new { query, bindVars }
        );

        return response?.Results ?? Enumerable.Empty<Reminder>();
    }

    public async Task<IEnumerable<Reminder>> GetDueRemindersAsync(DateTime? upToDate = null)
    {
        var targetDate = upToDate ?? DateTime.UtcNow.AddDays(7);

        var response = await _apiClient.PostAsync<object, QueryResponse<Reminder>>(
            "/query/aql",
            new
            {
                query = @"FOR reminder IN reminders 
                         FILTER reminder.status == 'Active' 
                         AND reminder.dueDate <= @targetDate 
                         SORT reminder.dueDate ASC 
                         RETURN reminder",
                bindVars = new { targetDate = targetDate.ToString("yyyy-MM-ddTHH:mm:ssZ") }
            }
        );

        return response?.Results ?? Enumerable.Empty<Reminder>();
    }

    public async Task<IEnumerable<Reminder>> GetOverdueRemindersAsync()
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<Reminder>>(
            "/query/aql",
            new
            {
                query = @"FOR reminder IN reminders 
                         FILTER reminder.status == 'Active' 
                         AND reminder.dueDate < @now 
                         SORT reminder.dueDate ASC 
                         RETURN reminder",
                bindVars = new { now = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ") }
            }
        );

        return response?.Results ?? Enumerable.Empty<Reminder>();
    }

    public async Task<bool> CompleteReminderAsync(string reminderId)
    {
        var reminder = await GetReminderByIdAsync(reminderId);
        if (reminder == null) return false;

        reminder.Status = ReminderStatus.Completed;
        reminder.CompletedAt = DateTime.UtcNow;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{reminder.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(reminder) }
        );

        return true;
    }

    public async Task<bool> CancelReminderAsync(string reminderId)
    {
        var reminder = await GetReminderByIdAsync(reminderId);
        if (reminder == null) return false;

        reminder.Status = ReminderStatus.Cancelled;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{reminder.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(reminder) }
        );

        return true;
    }

    public async Task<bool> TriggerEscalationAsync(string reminderId, int level)
    {
        var reminder = await GetReminderByIdAsync(reminderId);
        if (reminder == null) return false;

        var escalationLevel = reminder.EscalationLevels.FirstOrDefault(e => e.Level == level);
        if (escalationLevel == null) return false;

        escalationLevel.Triggered = true;
        escalationLevel.TriggeredAt = DateTime.UtcNow;
        reminder.Status = ReminderStatus.Escalated;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{reminder.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(reminder) }
        );

        // Send escalation notification
        await _notificationService.CreateNotificationAsync(new Notification
        {
            RecipientId = escalationLevel.EscalateTo,
            Type = NotificationType.Escalation,
            Priority = NotificationPriority.Urgent,
            Title = $"Eskalation: {reminder.Subject}",
            Message = $"Frist überschritten. Eskalationsstufe {level} erreicht.",
            ReminderId = reminderId,
            ProcessId = reminder.ProcessId
        });

        return true;
    }

    public async Task<IEnumerable<Reminder>> CheckAndEscalateAsync()
    {
        var reminders = await GetRemindersByUserAsync("", ReminderStatus.Active);
        var escalated = new List<Reminder>();

        foreach (var reminder in reminders)
        {
            foreach (var level in reminder.EscalationLevels.Where(e => !e.Triggered))
            {
                var escalationDate = reminder.DueDate.AddDays(-level.DaysBeforeDue);
                if (DateTime.UtcNow >= escalationDate)
                {
                    await TriggerEscalationAsync(reminder.Id, level.Level);
                    escalated.Add(reminder);
                }
            }
        }

        return escalated;
    }

    private class QueryResponse<T>
    {
        public List<T> Results { get; set; } = new();
    }
}

#endregion

#region Cosigning Service Implementation

public class CosigningService : ICosigningService
{
    private readonly IThemisApiClient _apiClient;
    private readonly INotificationService _notificationService;
    private readonly IProcessTimelineService _timelineService;

    public CosigningService(
        IThemisApiClient apiClient,
        INotificationService notificationService,
        IProcessTimelineService timelineService)
    {
        _apiClient = apiClient;
        _notificationService = notificationService;
        _timelineService = timelineService;
    }

    public async Task<Cosigning> CreateCosigningAsync(Cosigning cosigning)
    {
        cosigning.Id = cosigning.Id == string.Empty ? Guid.NewGuid().ToString() : cosigning.Id;
        cosigning.CreatedAt = DateTime.UtcNow;
        cosigning.Status = CosigningStatus.Pending;

        // Set step IDs and initial status
        for (int i = 0; i < cosigning.Steps.Count; i++)
        {
            cosigning.Steps[i].Id = Guid.NewGuid().ToString();
            cosigning.Steps[i].Order = i + 1;
            cosigning.Steps[i].Status = CosigningStepStatus.Pending;
        }

        await _apiClient.PutAsync<object, object>(
            $"/entities/{cosigning.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(cosigning) }
        );

        // Create timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            ProcessId = cosigning.ProcessId,
            DocumentId = cosigning.DocumentId,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.WorkflowStateChanged,
            Description = $"Mitzeichnung gestartet: {cosigning.Subject}",
            Actor = cosigning.CreatedBy
        });

        // Notify first cosigner(s)
        if (cosigning.Type == CosigningType.Serial)
        {
            var firstStep = cosigning.Steps.FirstOrDefault();
            if (firstStep != null)
            {
                await _notificationService.SendCosigningRequestAsync(firstStep.CosignerId, cosigning);
            }
        }
        else // Parallel
        {
            foreach (var step in cosigning.Steps)
            {
                await _notificationService.SendCosigningRequestAsync(step.CosignerId, cosigning);
            }
        }

        return cosigning;
    }

    public async Task<Cosigning?> GetCosigningByIdAsync(string id)
    {
        var urn = $"urn:themis:cosigning:{id}";
        return await _apiClient.GetAsync<Cosigning>($"/entities/{urn}");
    }

    public async Task<IEnumerable<Cosigning>> GetAllCosigningsAsync()
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<Cosigning>>(
            "/query/aql",
            new
            {
                query = "FOR cs IN cosignings RETURN cs",
                bindVars = new { }
            }
        );

        return response?.Results ?? Enumerable.Empty<Cosigning>();
    }

    public async Task<IEnumerable<Cosigning>> GetCosigningsByProcessAsync(string processId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<Cosigning>>(
            "/query/aql",
            new
            {
                query = "FOR cs IN cosignings FILTER cs.processId == @processId RETURN cs",
                bindVars = new { processId }
            }
        );

        return response?.Results ?? Enumerable.Empty<Cosigning>();
    }

    public async Task<IEnumerable<Cosigning>> GetPendingCosigningsForUserAsync(string userId)
    {
        var response = await _apiClient.PostAsync<object, QueryResponse<Cosigning>>(
            "/query/aql",
            new
            {
                query = @"FOR cs IN cosignings 
                         FILTER cs.status IN ['Pending', 'InProgress']
                         FOR step IN cs.steps
                         FILTER step.cosignerId == @userId AND step.status == 'Pending'
                         RETURN cs",
                bindVars = new { userId }
            }
        );

        return response?.Results ?? Enumerable.Empty<Cosigning>();
    }

    public async Task<bool> ApproveCosigningStepAsync(string cosigningId, string cosignerId, string comment)
    {
        var cosigning = await GetCosigningByIdAsync(cosigningId);
        if (cosigning == null) return false;

        var step = cosigning.Steps.FirstOrDefault(s => s.CosignerId == cosignerId && s.Status == CosigningStepStatus.Pending);
        if (step == null) return false;

        step.Status = CosigningStepStatus.Approved;
        step.SignedAt = DateTime.UtcNow;
        step.Comment = comment;

        // Update overall status
        await UpdateCosigningStatusAsync(cosigning);

        await _apiClient.PutAsync<object, object>(
            $"/entities/{cosigning.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(cosigning) }
        );

        // Create timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            ProcessId = cosigning.ProcessId,
            DocumentId = cosigning.DocumentId,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.DocumentApproved,
            Description = $"Mitzeichnung genehmigt von {step.CosignerName}",
            Actor = cosignerId
        });

        // Notify next cosigner if serial
        if (cosigning.Type == CosigningType.Serial && cosigning.Status == CosigningStatus.InProgress)
        {
            var nextStep = cosigning.Steps
                .Where(s => s.Status == CosigningStepStatus.Pending)
                .OrderBy(s => s.Order)
                .FirstOrDefault();

            if (nextStep != null)
            {
                await _notificationService.SendCosigningRequestAsync(nextStep.CosignerId, cosigning);
            }
        }

        return true;
    }

    public async Task<bool> RejectCosigningStepAsync(string cosigningId, string cosignerId, string reason)
    {
        var cosigning = await GetCosigningByIdAsync(cosigningId);
        if (cosigning == null) return false;

        var step = cosigning.Steps.FirstOrDefault(s => s.CosignerId == cosignerId && s.Status == CosigningStepStatus.Pending);
        if (step == null) return false;

        step.Status = CosigningStepStatus.Rejected;
        step.SignedAt = DateTime.UtcNow;
        step.IsRejected = true;
        step.RejectionReason = reason;

        cosigning.Status = CosigningStatus.Rejected;

        await _apiClient.PutAsync<object, object>(
            $"/entities/{cosigning.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(cosigning) }
        );

        // Create timeline event
        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = Guid.NewGuid().ToString(),
            ProcessId = cosigning.ProcessId,
            DocumentId = cosigning.DocumentId,
            Timestamp = DateTime.UtcNow,
            EventType = ProcessEventType.DocumentRejected,
            Description = $"Mitzeichnung abgelehnt von {step.CosignerName}: {reason}",
            Actor = cosignerId
        });

        return true;
    }

    public async Task<bool> SkipCosigningStepAsync(string cosigningId, string cosignerId, string reason)
    {
        var cosigning = await GetCosigningByIdAsync(cosigningId);
        if (cosigning == null) return false;

        var step = cosigning.Steps.FirstOrDefault(s => s.CosignerId == cosignerId && s.Status == CosigningStepStatus.Pending);
        if (step == null) return false;

        step.Status = CosigningStepStatus.Skipped;
        step.SignedAt = DateTime.UtcNow;
        step.Comment = reason;

        await UpdateCosigningStatusAsync(cosigning);

        await _apiClient.PutAsync<object, object>(
            $"/entities/{cosigning.Urn}",
            new { blob = System.Text.Json.JsonSerializer.Serialize(cosigning) }
        );

        return true;
    }

    public async Task<CosigningStatus> GetCosigningStatusAsync(string cosigningId)
    {
        var cosigning = await GetCosigningByIdAsync(cosigningId);
        return cosigning?.Status ?? CosigningStatus.Cancelled;
    }

    private async Task UpdateCosigningStatusAsync(Cosigning cosigning)
    {
        if (cosigning.Steps.Any(s => s.Status == CosigningStepStatus.Rejected))
        {
            cosigning.Status = CosigningStatus.Rejected;
        }
        else if (cosigning.Steps.All(s => s.Status == CosigningStepStatus.Approved || s.Status == CosigningStepStatus.Skipped))
        {
            cosigning.Status = CosigningStatus.Completed;
            cosigning.CompletedAt = DateTime.UtcNow;
        }
        else if (cosigning.Steps.Any(s => s.Status == CosigningStepStatus.Approved))
        {
            cosigning.Status = CosigningStatus.InProgress;
        }

        await Task.CompletedTask;
    }

    private class QueryResponse<T>
    {
        public List<T> Results { get; set; } = new();
    }
}

#endregion

#region Process Log Service Implementation

public class ProcessLogService : IProcessLogService
{
    private readonly IProcessTimelineService _timelineService;
    private readonly IAdministrativeStructureService _adminService;

    public ProcessLogService(
        IProcessTimelineService timelineService,
        IAdministrativeStructureService adminService)
    {
        _timelineService = timelineService;
        _adminService = adminService;
    }

    public async Task<ProcessLog> GetProcessLogAsync(string processId)
    {
        var process = await _adminService.GetProcessByIdAsync(processId);
        var events = await _timelineService.GetEventsByProcessAsync(processId);

        var log = new ProcessLog
        {
            ProcessId = processId,
            Subject = process?.Subject ?? "",
            CreatedAt = process?.CreatedAt ?? DateTime.MinValue,
            CompletedAt = process?.CompletedAt,
            Entries = events.Select(e => new ProcessLogEntry
            {
                Id = e.Id,
                Timestamp = e.Timestamp,
                Action = e.Description,
                ActionType = e.EventType.ToString(),
                Actor = e.Actor,
                ActorRole = e.ActorRole,
                Comment = e.Comment,
                Changes = e.ChangedFields
            }).ToList()
        };

        return log;
    }

    public async Task<ProcessLogEntry> AddProcessLogEntryAsync(string processId, ProcessLogEntry entry)
    {
        entry.Id = Guid.NewGuid().ToString();
        entry.Timestamp = DateTime.UtcNow;

        await _timelineService.CreateEventAsync(new ProcessTimelineEvent
        {
            Id = entry.Id,
            ProcessId = processId,
            Timestamp = entry.Timestamp,
            EventType = ProcessEventType.CommentAdded,
            Description = entry.Action,
            Actor = entry.Actor,
            ActorRole = entry.ActorRole,
            Comment = entry.Comment,
            ChangedFields = entry.Changes
        });

        return entry;
    }

    public async Task<byte[]> ExportProcessLogToPdfAsync(string processId)
    {
        // TODO: Implement PDF generation
        // For now, return empty byte array
        await Task.CompletedTask;
        return Array.Empty<byte>();
    }
}

#endregion

// Remaining implementations continue in next file...
