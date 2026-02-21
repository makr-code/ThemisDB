/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            OutlookTaskModels.cs                               ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:37:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
    • 94f72fb1c  2025-12-07  Add seamless messenger integration (WhatsApp, Jabber, Tea... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

public class OutlookTask
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Subject { get; set; } = string.Empty;
    public string Body { get; set; } = string.Empty;
    public DateTime? DueDate { get; set; }
    public DateTime? StartDate { get; set; }
    public OutlookTaskStatus Status { get; set; }
    public OutlookTaskPriority Priority { get; set; }
    public int PercentComplete { get; set; }
    public string? ProcessId { get; set; }
    public string? AssignedTo { get; set; }
    public string? Owner { get; set; }
    public List<string> Categories { get; set; } = new();
    public List<TaskReminder> Reminders { get; set; } = new();
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime? CompletedAt { get; set; }
}

public enum OutlookTaskStatus
{
    NotStarted,
    InProgress,
    Completed,
    WaitingOnOthers,
    Deferred
}

public enum OutlookTaskPriority
{
    Low = 0,
    Normal = 1,
    High = 2
}

public class TaskReminder
{
    public DateTime ReminderTime { get; set; }
    public bool IsSet { get; set; }
}

public class TaskCategory
{
    public string Name { get; set; } = string.Empty;
    public string Color { get; set; } = string.Empty;
}

public class TaskDelegation
{
    public string TaskId { get; set; } = string.Empty;
    public string DelegatedBy { get; set; } = string.Empty;
    public string DelegatedTo { get; set; } = string.Empty;
    public DateTime DelegatedAt { get; set; }
    public bool Accepted { get; set; }
}

public class TaskSyncSettings
{
    public bool AutoCreateTasksForProcessSteps { get; set; } = true;
    public bool SyncCompletionStatus { get; set; } = true;
    public bool SyncPriority { get; set; } = true;
    public bool SyncCategories { get; set; } = true;
}
