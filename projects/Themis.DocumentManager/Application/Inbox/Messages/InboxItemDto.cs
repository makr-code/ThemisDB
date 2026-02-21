/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            InboxItemDto.cs                                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
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
