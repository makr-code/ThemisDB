/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            OutboxModels.cs                                    ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     286                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

/// <summary>
/// Outbox item (Postausgang) - represents outgoing correspondence/documents
/// </summary>
public class OutboxItem
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string URN { get; set; } = string.Empty; // urn:themis:outbox:{id}
    
    // Basic information
    public string Subject { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string DocumentId { get; set; } = string.Empty; // Link to document being sent
    public string FileReference { get; set; } = string.Empty; // Aktenzeichen
    
    // Sender information
    public string SentBy { get; set; } = string.Empty; // User ID who sent
    public string SenderName { get; set; } = string.Empty;
    public string SenderDepartment { get; set; } = string.Empty;
    public string SenderEmail { get; set; } = string.Empty;
    
    // Recipient information
    public List<OutboxRecipient> Recipients { get; set; } = new();
    public List<OutboxRecipient> CarbonCopy { get; set; } = new(); // CC
    public List<OutboxRecipient> BlindCarbonCopy { get; set; } = new(); // BCC
    
    // Status tracking
    public OutboxStatus Status { get; set; } = OutboxStatus.Draft;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public DateTime? SentAt { get; set; }
    public DateTime? DeliveredAt { get; set; }
    public DateTime? ReadAt { get; set; } // When recipient first read it
    
    // Delivery method
    public DeliveryMethod DeliveryMethod { get; set; } = DeliveryMethod.Email;
    public bool RequireDeliveryConfirmation { get; set; }
    public bool RequireReadReceipt { get; set; }
    public bool IsUrgent { get; set; }
    public OutboxPriority Priority { get; set; } = OutboxPriority.Normal;
    
    // Process/File links
    public string? RelatedProcessId { get; set; }
    public string? RelatedFileId { get; set; }
    public string? RelatedInboxItemId { get; set; } // If this is a reply to an inbox item
    
    // Attachments
    public List<OutboxAttachment> Attachments { get; set; } = new();
    
    // Tracking & audit
    public List<OutboxDeliveryAttempt> DeliveryAttempts { get; set; } = new();
    public List<OutboxStatusChange> StatusHistory { get; set; } = new();
    public string? ErrorMessage { get; set; }
    
    // Security
    public SecurityClassification SecurityLevel { get; set; } = SecurityClassification.Public;
    public bool RequireEncryption { get; set; }
    public bool RequireSignature { get; set; }
    
    // Metadata
    public Dictionary<string, object> Metadata { get; set; } = new();
    public List<string> Tags { get; set; } = new();
}

/// <summary>
/// Outbox recipient
/// </summary>
public class OutboxRecipient
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public RecipientType Type { get; set; } = RecipientType.External;
    public string Name { get; set; } = string.Empty;
    public string Email { get; set; } = string.Empty;
    public string? OrganizationName { get; set; }
    public string? Department { get; set; }
    public string? Address { get; set; } // Physical address for postal delivery
    
    // Delivery tracking per recipient
    public RecipientDeliveryStatus DeliveryStatus { get; set; } = RecipientDeliveryStatus.Pending;
    public DateTime? DeliveredAt { get; set; }
    public DateTime? ReadAt { get; set; }
    public bool DeliveryConfirmed { get; set; }
    public string? DeliveryConfirmationId { get; set; }
    public string? ErrorMessage { get; set; }
}

/// <summary>
/// Recipient type
/// </summary>
public enum RecipientType
{
    Internal,  // Same organization
    External,  // Different organization
    Authority, // Government authority
    Citizen,   // Private citizen
    Company    // Private company
}

/// <summary>
/// Recipient delivery status
/// </summary>
public enum RecipientDeliveryStatus
{
    Pending,
    Sent,
    Delivered,
    Read,
    Failed,
    Bounced
}

/// <summary>
/// Outbox status
/// </summary>
public enum OutboxStatus
{
    Draft,        // Being composed
    Scheduled,    // Scheduled for sending
    Sending,      // Currently being sent
    Sent,         // Sent but not yet delivered
    Delivered,    // Delivered to all recipients
    PartiallyDelivered, // Delivered to some recipients
    Failed,       // Failed to send
    Cancelled,    // Cancelled before sending
    Archived      // Archived after successful delivery
}

/// <summary>
/// Delivery method
/// </summary>
public enum DeliveryMethod
{
    Email,         // Electronic mail
    Post,          // Physical mail (postal service)
    Courier,       // Courier service
    Fax,           // Facsimile
    EDelivery,     // E-delivery (eIDAS compliant)
    Portal,        // Web portal (recipient downloads)
    API,           // Direct API integration
    Print,         // Print locally (pickup by recipient)
    Internal       // Internal routing system
}

/// <summary>
/// Outbox priority
/// </summary>
public enum OutboxPriority
{
    Low,
    Normal,
    High,
    Urgent
}

/// <summary>
/// Outbox attachment
/// </summary>
public class OutboxAttachment
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Filename { get; set; } = string.Empty;
    public string ContentType { get; set; } = string.Empty;
    public long SizeBytes { get; set; }
    public string DocumentId { get; set; } = string.Empty; // Reference to document in ThemisDB
    public string? FilePath { get; set; }
    public bool IsEncrypted { get; set; }
    public string? EncryptionMethod { get; set; }
}

/// <summary>
/// Delivery attempt tracking
/// </summary>
public class OutboxDeliveryAttempt
{
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public bool Success { get; set; }
    public string? ErrorMessage { get; set; }
    public string? ErrorCode { get; set; }
    public DeliveryMethod Method { get; set; }
    public int AttemptNumber { get; set; }
    public Dictionary<string, string> Details { get; set; } = new();
}

/// <summary>
/// Status change history
/// </summary>
public class OutboxStatusChange
{
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public OutboxStatus FromStatus { get; set; }
    public OutboxStatus ToStatus { get; set; }
    public string ChangedBy { get; set; } = string.Empty;
    public string? Reason { get; set; }
}

/// <summary>
/// Outbox filter for querying
/// </summary>
public class OutboxFilter
{
    public string? SentBy { get; set; }
    public string? Department { get; set; }
    public OutboxStatus? Status { get; set; }
    public OutboxPriority? Priority { get; set; }
    public DateTime? SentAfter { get; set; }
    public DateTime? SentBefore { get; set; }
    public DeliveryMethod? DeliveryMethod { get; set; }
    public string? SearchText { get; set; }
    public string? RecipientName { get; set; }
    public string? RelatedProcessId { get; set; }
    public List<string>? Tags { get; set; }
}

/// <summary>
/// Outbox statistics
/// </summary>
public class OutboxStatistics
{
    public int TotalSent { get; set; }
    public int TotalDelivered { get; set; }
    public int TotalFailed { get; set; }
    public int PendingDelivery { get; set; }
    public Dictionary<DeliveryMethod, int> ByDeliveryMethod { get; set; } = new();
    public Dictionary<OutboxPriority, int> ByPriority { get; set; } = new();
    public Dictionary<string, int> ByDepartment { get; set; } = new();
    public double AverageDeliveryTimeMinutes { get; set; }
    public int AwaitingReadReceipt { get; set; }
}

/// <summary>
/// Scheduled outbox item (for delayed sending)
/// </summary>
public class ScheduledOutboxItem
{
    public string OutboxItemId { get; set; } = string.Empty;
    public DateTime ScheduledSendTime { get; set; }
    public bool RecurringSend { get; set; }
    public RecurrencePattern? RecurrencePattern { get; set; }
}

/// <summary>
/// Outbox template (for frequently sent communications)
/// </summary>
public class OutboxTemplate
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string SubjectTemplate { get; set; } = string.Empty;
    public string BodyTemplate { get; set; } = string.Empty;
    public DeliveryMethod DefaultDeliveryMethod { get; set; } = DeliveryMethod.Email;
    public OutboxPriority DefaultPriority { get; set; } = OutboxPriority.Normal;
    public List<string> DefaultTags { get; set; } = new();
    public List<OutboxAttachment> DefaultAttachments { get; set; } = new();
    public string CreatedBy { get; set; } = string.Empty;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}
