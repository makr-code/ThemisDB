/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateTaskCommandValidator.cs                      ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
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
