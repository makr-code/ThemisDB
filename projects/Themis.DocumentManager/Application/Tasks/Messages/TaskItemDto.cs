/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TaskItemDto.cs                                     ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     59                                             ║
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

using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Messages;

/// <summary>
/// Task DTO for data transfer
/// </summary>
public record TaskItemDto : BaseEntityDto
{
    public string Subject { get; init; } = string.Empty;
    public string Body { get; init; } = string.Empty;
    public DateTime? DueDate { get; init; }
    public DateTime? StartDate { get; init; }
    public OutlookTaskStatus Status { get; init; }
    public OutlookTaskPriority Priority { get; init; }
    public int PercentComplete { get; init; }
    public string? ProcessId { get; init; }
    public string? AssignedTo { get; init; }
    public string? Owner { get; init; }
    public List<string> Categories { get; init; } = new();
    public DateTime? CompletedAt { get; init; }
    public string? LinkedEntityId { get; init; }
    public LinkedEntityType? LinkedEntityType { get; init; }
}

public enum LinkedEntityType
{
    Document,
    Process,
    Inbox,
    Reminder
}
