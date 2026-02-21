/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateReminderCommandValidator.cs                  ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     56                                             ║
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
