/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ReminderDto.cs                                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     61                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
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
