/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ProcessWatchModels.cs                              ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     191                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Represents a process watch (Überwachen-Aufgabe) - monitoring without intervention
/// </summary>
public class ProcessWatch
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string URN { get; set; } = string.Empty; // urn:themis:process-watch:{id}
    public string ProcessId { get; set; } = string.Empty;
    public string ProcessNumber { get; set; } = string.Empty;
    public string WatchedBy { get; set; } = string.Empty; // User ID of watcher
    public string WatcherName { get; set; } = string.Empty;
    public string Department { get; set; } = string.Empty;
    public string Reason { get; set; } = string.Empty; // Why is this being watched?
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime? EndedAt { get; set; }
    public bool IsActive { get; set; } = true;
    
    // Notification settings
    public ProcessWatchNotificationSettings NotificationSettings { get; set; } = new();
    
    // What to watch for
    public List<ProcessEventType> WatchedEventTypes { get; set; } = new();
    public bool WatchAllEvents { get; set; } = true;
    public bool NotifyOnStatusChange { get; set; } = true;
    public bool NotifyOnDeadlineApproaching { get; set; } = true;
    public bool NotifyOnDocumentAdded { get; set; } = true;
    public bool NotifyOnCommentAdded { get; set; } = true;
    public bool NotifyOnApproval { get; set; } = true;
    public bool NotifyOnRejection { get; set; } = true;
    public bool NotifyOnCompletion { get; set; } = true;
    
    // Watch scope
    public bool WatchMainProcessOnly { get; set; } = false; // Or include sub-processes
    public bool WatchRelatedDocuments { get; set; } = true;
    public bool WatchWorkflowSteps { get; set; } = true;
    
    // Notification history
    public List<ProcessWatchNotification> Notifications { get; set; } = new();
    
    // Statistics
    public int TotalNotifications { get; set; }
    public DateTime? LastNotificationAt { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Notification settings for process watch
/// </summary>
public class ProcessWatchNotificationSettings
{
    public bool EnableEmail { get; set; } = true;
    public bool EnableDesktopNotification { get; set; } = true;
    public bool EnableInAppNotification { get; set; } = true;
    public string? EmailAddress { get; set; }
    
    // Notification frequency
    public ProcessWatchNotificationFrequency Frequency { get; set; } = ProcessWatchNotificationFrequency.Immediate;
    public bool DigestMode { get; set; } = false; // Send summary instead of individual notifications
    public TimeSpan? DigestInterval { get; set; } // e.g., daily digest at 9 AM
    
    // Notification priority filter
    public bool OnlyHighPriority { get; set; } = false;
    public bool OnlyUrgent { get; set; } = false;
    
    // Quiet hours
    public bool EnableQuietHours { get; set; } = false;
    public TimeSpan QuietHoursStart { get; set; } = TimeSpan.FromHours(18); // 6 PM
    public TimeSpan QuietHoursEnd { get; set; } = TimeSpan.FromHours(8); // 8 AM
    public bool QueueDuringQuietHours { get; set; } = true; // Queue and send after quiet hours
}

/// <summary>
/// Notification frequency for process watch
/// </summary>
public enum ProcessWatchNotificationFrequency
{
    Immediate,    // Send immediately when event occurs
    Hourly,       // Batch notifications every hour
    Daily,        // Daily digest
    Weekly,       // Weekly summary
    OnDemand      // Only when user requests
}

/// <summary>
/// Individual notification generated by process watch
/// </summary>
public class ProcessWatchNotification
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string ProcessWatchId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public ProcessEventType EventType { get; set; }
    public string EventDescription { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public string Actor { get; set; } = string.Empty; // Who triggered the event
    public ProcessWatchNotificationPriority Priority { get; set; } = ProcessWatchNotificationPriority.Normal;
    public bool IsRead { get; set; }
    public DateTime? ReadAt { get; set; }
    public bool WasSent { get; set; }
    public DateTime? SentAt { get; set; }
    public List<string> SentChannels { get; set; } = new(); // Email, Desktop, InApp
    public Dictionary<string, object> EventData { get; set; } = new(); // Additional event information
}

/// <summary>
/// Priority levels for watch notifications
/// </summary>
public enum ProcessWatchNotificationPriority
{
    Low,
    Normal,
    High,
    Urgent
}

/// <summary>
/// Watch status summary
/// </summary>
public class ProcessWatchSummary
{
    public string ProcessWatchId { get; set; } = string.Empty;
    public string ProcessNumber { get; set; } = string.Empty;
    public string ProcessTitle { get; set; } = string.Empty;
    public DateTime WatchStartedAt { get; set; }
    public int TotalEvents { get; set; }
    public int UnreadNotifications { get; set; }
    public DateTime? LastActivityAt { get; set; }
    public ProcessStatus CurrentStatus { get; set; }
    public Dictionary<ProcessEventType, int> EventCounts { get; set; } = new();
    public List<string> RecentActors { get; set; } = new();
    public bool HasCriticalEvents { get; set; }
    public int DaysWatched => (DateTime.UtcNow - WatchStartedAt).Days;
}

/// <summary>
/// Bulk watch operations
/// </summary>
public class BulkProcessWatch
{
    public List<string> ProcessIds { get; set; } = new();
    public string WatchedBy { get; set; } = string.Empty;
    public string Reason { get; set; } = string.Empty;
    public ProcessWatchNotificationSettings NotificationSettings { get; set; } = new();
    public List<ProcessEventType> WatchedEventTypes { get; set; } = new();
}

/// <summary>
/// Watch filter for querying watches
/// </summary>
public class ProcessWatchFilter
{
    public string? WatchedBy { get; set; }
    public string? Department { get; set; }
    public bool? IsActive { get; set; }
    public DateTime? CreatedAfter { get; set; }
    public DateTime? CreatedBefore { get; set; }
    public bool? HasUnreadNotifications { get; set; }
    public List<ProcessEventType>? EventTypes { get; set; }
    public string? SearchText { get; set; }
}
