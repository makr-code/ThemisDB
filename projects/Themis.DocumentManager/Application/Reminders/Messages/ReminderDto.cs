/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ReminderDto.cs                                     ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Reminders.Messages;

/// <summary>
/// Escalation Level DTO
/// </summary>
public record EscalationLevelDto
{
    public int Level { get; init; }
    public int DaysBeforeDue { get; init; }
    public string EscalateTo { get; init; } = string.Empty;
    public string EscalateToRole { get; init; } = string.Empty;
}

/// <summary>
/// Reminder DTO for data transfer
/// </summary>
public record ReminderDto : BaseEntityDto
{
    public string ProcessId { get; init; } = string.Empty;
    public string FileId { get; init; } = string.Empty;
    public string DocumentId { get; init; } = string.Empty;
    public DateTime DueDate { get; init; }
    public DateTime? ReminderDate { get; init; }
    public DateTime? CompletedAt { get; init; }
    public ReminderType Type { get; init; }
    public ReminderStatus Status { get; init; }
    public string AssignedTo { get; init; } = string.Empty;
    public string Subject { get; init; } = string.Empty;
    public string Description { get; init; } = string.Empty;
    public List<EscalationLevelDto> EscalationLevels { get; init; } = new();
    public Dictionary<string, object> Metadata { get; init; } = new();
    public bool IsCompleted { get; init; }
    public bool IsOverdue { get; init; }
}
