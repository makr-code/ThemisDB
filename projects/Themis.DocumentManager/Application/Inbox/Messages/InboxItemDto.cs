/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            InboxItemDto.cs                                    ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
