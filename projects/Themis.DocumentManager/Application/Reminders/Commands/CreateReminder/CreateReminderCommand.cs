/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateReminderCommand.cs                           ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Reminders.Commands.CreateReminder;

/// <summary>
/// Command to create a reminder (Wiedervorlage)
/// Based on PDV VIS Fristenmanagement requirements
/// </summary>
public record CreateReminderCommand : IRequest<string>
{
    public string ProcessId { get; init; } = string.Empty;
    public DateTime DueDate { get; init; }
    public DateTime? ReminderDate { get; init; }
    public ReminderType Type { get; init; }
    public string AssignedTo { get; init; } = string.Empty;
    public string? Description { get; init; }
    public List<EscalationLevelDto>? EscalationLevels { get; init; }
}

public record EscalationLevelDto
{
    public int Level { get; init; }
    public int DaysBeforeDue { get; init; }
    public string EscalateTo { get; init; } = string.Empty;
}
