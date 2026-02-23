/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            InboxItemDto.cs                                    ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Inbox.Messages;

public record InboxItemDto : BaseEntityDto
{
    public DateTime ReceivedAt { get; init; }
    public InboxStatus Status { get; init; } = InboxStatus.New;
    public InboxPriority Priority { get; init; } = InboxPriority.Normal;
    public bool IsRead { get; init; }
    
    public string AssignedTo { get; init; } = string.Empty;
    public string? AssignedBy { get; init; }
    public DateTime? AssignedAt { get; init; }
    
    public string DocumentId { get; init; } = string.Empty;
    public string Subject { get; init; } = string.Empty;
    public string Sender { get; init; } = string.Empty;
    public string? SenderEmail { get; init; }
    public string? Description { get; init; }
    public string? RelatedProcessId { get; init; }
    public string? Notes { get; init; }
    public Dictionary<string, object> Metadata { get; init; } = new();
}

public enum InboxStatus
{
    New,
    Assigned,
    InProgress,
    Completed,
    Archived
}

public enum InboxPriority
{
    Low,
    Normal,
    High,
    Urgent
}
