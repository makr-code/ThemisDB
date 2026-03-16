/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateReminderCommandValidator.cs                  ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Reminders.Commands.CreateReminder;

/// <summary>
/// Validator for CreateReminderCommand
/// Ensures VIS Fristenmanagement compliance
/// </summary>
public class CreateReminderCommandValidator : AbstractValidator<CreateReminderCommand>
{
    public CreateReminderCommandValidator()
    {
        RuleFor(v => v.ProcessId)
            .NotEmpty().WithMessage("Vorgang-ID ist erforderlich.");

        RuleFor(v => v.DueDate)
            .GreaterThan(DateTime.UtcNow).WithMessage("Fälligkeitsdatum muss in der Zukunft liegen.");

        RuleFor(v => v.AssignedTo)
            .NotEmpty().WithMessage("Zuständiger Sachbearbeiter ist erforderlich.");

        RuleFor(v => v.ReminderDate)
            .LessThan(v => v.DueDate)
            .When(v => v.ReminderDate.HasValue)
            .WithMessage("Wiedervorlagedatum muss vor dem Fälligkeitsdatum liegen.");

        RuleFor(v => v.Type)
            .IsInEnum().WithMessage("Ungültiger Wiedervorlagetyp.");
    }
}
