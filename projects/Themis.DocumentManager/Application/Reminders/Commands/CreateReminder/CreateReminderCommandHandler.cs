/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateReminderCommandHandler.cs                    ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
