/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateReminderCommandValidator.cs                  ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     40                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;
using Themis.DocumentManager.Application.Reminders.Commands.UpdateReminder;

namespace Themis.DocumentManager.Application.Reminders.Commands.UpdateReminder;

public class UpdateReminderCommandValidator : AbstractValidator<UpdateReminderCommand>
{
    public UpdateReminderCommandValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty().WithMessage("Id is required");

        RuleFor(x => x.Subject)
            .MaximumLength(255).WithMessage("Subject must not exceed 255 characters")
            .When(x => !string.IsNullOrEmpty(x.Subject));

        RuleFor(x => x.DueDate)
            .GreaterThan(DateTime.Now).WithMessage("DueDate must be in the future")
            .When(x => x.DueDate.HasValue);
    }
}
