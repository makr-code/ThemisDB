/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateReminderCommandValidator.cs                  ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     56                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
