/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateReminderCommandValidator.cs                  ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 5de9cc853  2025-12-10  Integrate VIS features (Inbox, Reminders, Cosigning) into... ║
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
