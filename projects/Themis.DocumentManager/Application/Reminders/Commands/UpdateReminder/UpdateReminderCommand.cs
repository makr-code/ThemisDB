/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateReminderCommand.cs                           ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     37                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;

namespace Themis.DocumentManager.Application.Reminders.Commands.UpdateReminder;

/// <summary>
/// Command to update an existing reminder
/// </summary>
public record UpdateReminderCommand : IUpdateCommand
{
    public string Id { get; init; } = string.Empty;
    public string? Subject { get; init; }
    public string? Description { get; init; }
    public DateTime? DueDate { get; init; }
    public DateTime? ReminderDate { get; init; }
    public DateTime? CompletedAt { get; init; }
    public Dictionary<string, object>? Metadata { get; init; }
}
