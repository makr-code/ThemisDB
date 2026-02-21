/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Phase1Models.cs                                    ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     377                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Phase 1 VIS-Features: Models für Posteingang, Fristen, Mitzeichnung, Aktenplan
/// </summary>

#region Posteingang (Inbox)

/// <summary>
/// Posteingangs-Eintrag
/// URN: urn:themis:inbox:{id}
/// </summary>
public class InboxItem
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:inbox:{Id}";
    
    public DateTime ReceivedAt { get; set; }
    public InboxStatus Status { get; set; } = InboxStatus.New;
    public InboxPriority Priority { get; set; } = InboxPriority.Normal;
    public bool IsRead { get; set; } = false;
    
    public string AssignedTo { get; set; } = string.Empty;
    public string AssignedBy { get; set; } = string.Empty;
    public DateTime? AssignedAt { get; set; }
    
    public string DocumentId { get; set; } = string.Empty;
    public string Subject { get; set; } = string.Empty;
    public string Sender { get; set; } = string.Empty;
    public string SenderEmail { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string RelatedProcessId { get; set; } = string.Empty;
    
    public string Notes { get; set; } = string.Empty;
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public enum InboxStatus
{
    New,            // Neu eingetroffen
    Assigned,       // Zugewiesen
    InProgress,     // In Bearbeitung
    Completed,      // Erledigt
    Archived        // Archiviert
}

public enum InboxPriority
{
    Low,            // Niedrig
    Normal,         // Normal
    High,           // Hoch
    Urgent          // Dringend
}

#endregion

#region Wiedervorlage & Fristen (Reminders & Deadlines)

/// <summary>
/// Wiedervorlage/Frist
/// URN: urn:themis:reminder:{id}
/// </summary>
public class Reminder
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:reminder:{Id}";
    
    public string ProcessId { get; set; } = string.Empty;
    public string FileId { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty;
    
    public DateTime DueDate { get; set; }
    public DateTime? ReminderDate { get; set; }
    public DateTime? CompletedAt { get; set; }
    public bool IsCompleted => CompletedAt.HasValue;
    public bool IsOverdue => DueDate < DateTime.UtcNow && !IsCompleted;
    
    public ReminderType Type { get; set; }
    public ReminderStatus Status { get; set; } = ReminderStatus.Active;
    
    public string AssignedTo { get; set; } = string.Empty;
    public string CreatedBy { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
    
    public string Subject { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    public List<EscalationLevel> EscalationLevels { get; set; } = new();
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public class EscalationLevel
{
    public int Level { get; set; }
    public int DaysBeforeDue { get; set; }
    public string EscalateTo { get; set; } = string.Empty;
    public string EscalateToRole { get; set; } = string.Empty;
    public bool Triggered { get; set; }
    public DateTime? TriggeredAt { get; set; }
}

public enum ReminderType
{
    Deadline,       // Frist
    Reminder,       // Wiedervorlage
    Review,         // Prüfung
    Approval        // Genehmigung
}

public enum ReminderStatus
{
    Active,         // Aktiv
    Completed,      // Erledigt
    Cancelled,      // Abgebrochen
    Overdue,        // Überfällig
    Escalated       // Eskaliert
}

#endregion

#region Mitzeichnung (Co-signing)

/// <summary>
/// Mitzeichnungsverfahren
/// URN: urn:themis:cosigning:{id}
/// </summary>
public class Cosigning
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:cosigning:{Id}";
    
    public string ProcessId { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty;
    
    public CosigningType Type { get; set; } = CosigningType.Serial;
    public CosigningStatus Status { get; set; } = CosigningStatus.Pending;
    
    public DateTime CreatedAt { get; set; }
    public string CreatedBy { get; set; } = string.Empty;
    public DateTime? CompletedAt { get; set; }
    
    public string Subject { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    public List<CosigningStep> Steps { get; set; } = new();
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public class CosigningStep
{
    public string Id { get; set; } = string.Empty;
    public int Order { get; set; }
    public int StepNumber { get; set; }
    public DateTime? CompletedAt { get; set; }
    
    public string CosignerId { get; set; } = string.Empty;
    public string CosignerName { get; set; } = string.Empty;
    public string CosignerRole { get; set; } = string.Empty;
    
    public CosigningStepStatus Status { get; set; } = CosigningStepStatus.Pending;
    
    public DateTime? SignedAt { get; set; }
    public string Comment { get; set; } = string.Empty;
    public bool IsRejected { get; set; }
    public string RejectionReason { get; set; } = string.Empty;
}

public enum CosigningType
{
    Serial,         // Seriell (nacheinander)
    Parallel        // Parallel (gleichzeitig)
}

public enum CosigningStatus
{
    Pending,        // Ausstehend
    InProgress,     // In Bearbeitung
    Completed,      // Abgeschlossen
    Rejected,       // Abgelehnt
    Cancelled       // Abgebrochen
}

public enum CosigningStepStatus
{
    Pending,        // Ausstehend
    Approved,       // Genehmigt
    Rejected,       // Abgelehnt
    Skipped         // Übersprungen
}

#endregion

#region Vorgangslaufzettel (Process Log)

/// <summary>
/// Vorgangslaufzettel - Digitaler Laufzettel
/// Wird aus ProcessTimelineEvents generiert
/// </summary>
public class ProcessLog
{
    public string ProcessId { get; set; } = string.Empty;
    public string FileNumber { get; set; } = string.Empty;
    public string Subject { get; set; } = string.Empty;
    
    public DateTime CreatedAt { get; set; }
    public DateTime? CompletedAt { get; set; }
    
    public List<ProcessLogEntry> Entries { get; set; } = new();
}

public class ProcessLogEntry
{
    public string Id { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; }
    
    public string Action { get; set; } = string.Empty;
    public string ActionType { get; set; } = string.Empty;
    
    public string Actor { get; set; } = string.Empty;
    public string ActorRole { get; set; } = string.Empty;
    
    public string FromStation { get; set; } = string.Empty;
    public string ToStation { get; set; } = string.Empty;
    
    public string Comment { get; set; } = string.Empty;
    public Dictionary<string, object> Changes { get; set; } = new();
}

#endregion

#region Aktenplan (Filing Plan)

/// <summary>
/// Aktenplan
/// URN: urn:themis:filingplan:{id}
/// </summary>
public class FilingPlan
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:filingplan:{Id}";
    
    public string Name { get; set; } = string.Empty;
    public string Type { get; set; } = string.Empty; // "KGSt", "Custom", "Standard"
    public string Description { get; set; } = string.Empty;
    
    public string AuthorityId { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; }
    public DateTime? ValidFrom { get; set; }
    public DateTime? ValidUntil { get; set; }
    
    public bool IsActive { get; set; } = true;
    public List<FilingPlanNode> Nodes { get; set; } = new();
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Aktenplan-Knoten
/// URN: urn:themis:filingplan:{planId}:node:{id}
/// </summary>
public class FilingPlanNode
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:filingplan:{FilingPlanId}:node:{Id}";
    
    public string FilingPlanId { get; set; } = string.Empty;
    public string ParentId { get; set; } = string.Empty;
    
    public string Code { get; set; } = string.Empty; // z.B. "1.2.3" oder "IV C 5"
    public string Title { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    
    public int RetentionYears { get; set; } = 10;
    public bool IsArchivable { get; set; } = true;
    public bool RequiresApproval { get; set; }
    
    public List<string> AllowedRoles { get; set; } = new();
    public List<string> ResponsibleOfficers { get; set; } = new();
    
    public Dictionary<string, object> Metadata { get; set; } = new();
}

#endregion

#region Benachrichtigungen (Notifications)

/// <summary>
/// Benachrichtigung
/// URN: urn:themis:notification:{id}
/// </summary>
public class Notification
{
    public string Id { get; set; } = string.Empty;
    public string Urn => $"urn:themis:notification:{Id}";
    
    public string RecipientId { get; set; } = string.Empty;
    public string RecipientName { get; set; } = string.Empty;
    
    public NotificationType Type { get; set; }
    public NotificationPriority Priority { get; set; } = NotificationPriority.Normal;
    
    public string Title { get; set; } = string.Empty;
    public string Message { get; set; } = string.Empty;
    
    public DateTime CreatedAt { get; set; }
    public DateTime? ReadAt { get; set; }
    public DateTime? DismissedAt { get; set; }
    
    public bool IsRead { get; set; }
    public bool IsDismissed { get; set; }
    
    // Verknüpfungen
    public string ProcessId { get; set; } = string.Empty;
    public string FileId { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty;
    public string ReminderId { get; set; } = string.Empty;
    public string RecipientUserId { get; set; } = string.Empty;
    
    public Dictionary<string, object> Metadata { get; set; } = new();
}

public enum NotificationType
{
    Info,               // Information
    Warning,            // Warnung
    Error,              // Fehler
    Success,            // Erfolgreich
    DeadlineReminder,   // Frist-Erinnerung
    Deadline,           // Frist
    DeadlineOverdue,    // Frist überfällig
    TaskAssigned,       // Aufgabe zugewiesen
    Task,               // Aufgabe
    CosigningRequest,   // Mitzeichnung angefordert
    Cosigning,          // Mitzeichnung
    ProcessCompleted,   // Vorgang abgeschlossen
    DocumentReceived,   // Dokument eingegangen
    Escalation,         // Eskalation
    System              // System
}

public enum NotificationPriority
{
    Low,
    Normal,
    High,
    Urgent
}

#endregion
