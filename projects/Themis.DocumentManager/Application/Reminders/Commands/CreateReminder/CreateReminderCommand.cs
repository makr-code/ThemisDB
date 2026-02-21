/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateReminderCommand.cs                           ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
