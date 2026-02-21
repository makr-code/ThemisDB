/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Phase1Services.cs                                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     144                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
/// Phase 1 VIS-Features: Service Interfaces
/// </summary>

#region Posteingang (Inbox) Service

public interface IInboxService
{
    Task<InboxItem> CreateInboxItemAsync(InboxItem item);
    Task<InboxItem?> GetInboxItemByIdAsync(string id);
    Task<InboxItem?> GetInboxItemAsync(string id);
    Task<IEnumerable<InboxItem>> GetInboxItemsAsync(InboxStatus? status = null, string? assignedTo = null);
    Task<IEnumerable<InboxItem>> GetAllInboxItemsAsync();
    Task<IEnumerable<InboxItem>> GetMyInboxItemsAsync(string userId);
    Task<bool> AssignInboxItemAsync(string itemId, string assignedTo, string assignedBy);
    Task<bool> UpdateInboxStatusAsync(string itemId, InboxStatus status);
    Task<bool> MarkAsReadAsync(string itemId);
    Task<bool> UpdateInboxItemStatusAsync(string itemId, InboxStatus status);
    Task<bool> DeleteInboxItemAsync(string itemId);
    Task<bool> UpdateInboxPriorityAsync(string itemId, InboxPriority priority);
    Task<int> GetUnreadCountAsync(string userId);
}

#endregion

#region Wiedervorlage/Fristen (Reminder) Service

public interface IReminderService
{
    Task<Reminder> CreateReminderAsync(Reminder reminder);
    Task<Reminder?> GetReminderByIdAsync(string id);
    Task<IEnumerable<Reminder>> GetAllRemindersAsync();
    Task<IEnumerable<Reminder>> GetRemindersByProcessAsync(string processId);
    Task<IEnumerable<Reminder>> GetRemindersByUserAsync(string userId, ReminderStatus? status = null);
    Task<IEnumerable<Reminder>> GetDueRemindersAsync(DateTime? upToDate = null);
    Task<IEnumerable<Reminder>> GetOverdueRemindersAsync();
    Task<bool> CompleteReminderAsync(string reminderId);
    Task<bool> CancelReminderAsync(string reminderId);
    Task<bool> TriggerEscalationAsync(string reminderId, int level);
    Task<IEnumerable<Reminder>> CheckAndEscalateAsync();
}

#endregion

#region Mitzeichnung (Cosigning) Service

public interface ICosigningService
{
    Task<Cosigning> CreateCosigningAsync(Cosigning cosigning);
    Task<Cosigning?> GetCosigningByIdAsync(string id);
    Task<IEnumerable<Cosigning>> GetAllCosigningsAsync();
    Task<IEnumerable<Cosigning>> GetCosigningsByProcessAsync(string processId);
    Task<IEnumerable<Cosigning>> GetPendingCosigningsForUserAsync(string userId);
    Task<bool> ApproveCosigningStepAsync(string cosigningId, string cosignerId, string comment);
    Task<bool> RejectCosigningStepAsync(string cosigningId, string cosignerId, string reason);
    Task<bool> SkipCosigningStepAsync(string cosigningId, string cosignerId, string reason);
    Task<CosigningStatus> GetCosigningStatusAsync(string cosigningId);
}

#endregion

#region Vorgangslaufzettel (Process Log) Service

public interface IProcessLogService
{
    Task<ProcessLog> GetProcessLogAsync(string processId);
    Task<ProcessLogEntry> AddProcessLogEntryAsync(string processId, ProcessLogEntry entry);
    Task<byte[]> ExportProcessLogToPdfAsync(string processId);
}

#endregion

#region Aktenplan (Filing Plan) Service

public interface IFilingPlanService
{
    Task<FilingPlan> CreateFilingPlanAsync(FilingPlan plan);
    Task<FilingPlan?> GetFilingPlanByIdAsync(string id);
    Task<IEnumerable<FilingPlan>> GetFilingPlansByAuthorityAsync(string authorityId);
    Task<FilingPlan?> GetActiveFilingPlanAsync(string authorityId);
    
    Task<FilingPlanNode> CreateNodeAsync(FilingPlanNode node);
    Task<FilingPlanNode?> GetNodeByIdAsync(string planId, string nodeId);
    Task<IEnumerable<FilingPlanNode>> GetNodesByPlanAsync(string planId);
    Task<IEnumerable<FilingPlanNode>> GetChildNodesAsync(string planId, string parentId);
    Task<string> GenerateFileNumberAsync(string planId, string nodeId, int year);
}

#endregion

#region Benachrichtigungen (Notification) Service

public interface INotificationService
{
    Task<Notification> CreateNotificationAsync(Notification notification);
    Task<IEnumerable<Notification>> GetNotificationsByUserAsync(string userId, bool includeRead = false);
    Task<IEnumerable<Notification>> GetUnreadNotificationsAsync(string userId);
    Task<bool> MarkAsReadAsync(string notificationId);
    Task<bool> MarkAllAsReadAsync(string userId);
    Task<bool> DismissNotificationAsync(string notificationId);
    Task<int> GetUnreadCountAsync(string userId);
    Task ShowNotificationAsync(Notification notification, System.Threading.CancellationToken cancellationToken = default);
    
    // Helper methods für automatische Benachrichtigungen
    Task SendDeadlineReminderAsync(string userId, Reminder reminder);
    Task SendTaskAssignedAsync(string userId, string processId, string assignedBy);
    Task SendCosigningRequestAsync(string userId, Cosigning cosigning);
}

#endregion
