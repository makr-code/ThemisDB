/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateTaskCommandValidator.cs                      ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Tasks.Commands.CreateTask;

/// <summary>
/// Validator for CreateTaskCommand
/// </summary>
public class CreateTaskCommandValidator : AbstractValidator<CreateTaskCommand>
{
    public CreateTaskCommandValidator()
    {
        RuleFor(x => x.Subject)
            .NotEmpty().WithMessage("Betreff darf nicht leer sein")
            .MaximumLength(500).WithMessage("Betreff darf maximal 500 Zeichen lang sein");

        When(x => x.DueDate.HasValue && x.StartDate.HasValue, () =>
        {
            RuleFor(x => x.DueDate)
                .GreaterThanOrEqualTo(x => x.StartDate)
                .WithMessage("Fälligkeitsdatum muss nach oder gleich dem Startdatum sein");
        });

        When(x => !string.IsNullOrEmpty(x.AssignedTo), () =>
        {
            RuleFor(x => x.Owner)
                .NotEmpty().WithMessage("Owner muss angegeben werden wenn eine Zuweisung erfolgt");
        });
    }
}
