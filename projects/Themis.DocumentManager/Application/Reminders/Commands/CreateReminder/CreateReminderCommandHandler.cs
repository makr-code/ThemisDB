/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateReminderCommandHandler.cs                    ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Application.Reminders.Commands.CreateReminder;

/// <summary>
/// Handler for CreateReminderCommand
/// Implements VIS Wiedervorlage/Fristenmanagement functionality
/// </summary>
public class CreateReminderCommandHandler : IRequestHandler<CreateReminderCommand, string>
{
    private readonly IReminderService _reminderService;

    public CreateReminderCommandHandler(IReminderService reminderService)
    {
        _reminderService = reminderService;
    }

    public async Task<string> Handle(CreateReminderCommand request, CancellationToken cancellationToken)
    {
        var reminder = new Reminder
        {
            Id = Guid.NewGuid().ToString(),
            ProcessId = request.ProcessId,
            DueDate = request.DueDate,
            ReminderDate = request.ReminderDate,
            Type = request.Type,
            AssignedTo = request.AssignedTo,
            Description = request.Description ?? string.Empty,
            Status = ReminderStatus.Active,
            CreatedAt = DateTime.UtcNow,
            EscalationLevels = request.EscalationLevels?.Select(e => new EscalationLevel
            {
                Level = e.Level,
                DaysBeforeDue = e.DaysBeforeDue,
                EscalateTo = e.EscalateTo,
                Triggered = false
            }).ToList() ?? new List<EscalationLevel>()
        };

        var createdReminder = await _reminderService.CreateReminderAsync(reminder);
        
        return createdReminder.Id;
    }
}
