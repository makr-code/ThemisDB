/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateTaskCommandValidator.cs                      ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Tasks.Commands.UpdateTask;

public class UpdateTaskCommandValidator : AbstractValidator<UpdateTaskCommand>
{
    public UpdateTaskCommandValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty()
            .WithMessage("Task ID ist erforderlich");

        When(x => x.DueDate.HasValue && x.StartDate.HasValue, () =>
        {
            RuleFor(x => x.DueDate)
                .GreaterThanOrEqualTo(x => x.StartDate)
                .WithMessage("Fälligkeitsdatum muss nach oder am Startdatum liegen");
        });

        When(x => !string.IsNullOrEmpty(x.AssignedTo), () =>
        {
            RuleFor(x => x.Owner)
                .NotEmpty()
                .WithMessage("Owner ist erforderlich wenn Task zugewiesen ist");
        });

        When(x => x.PercentComplete.HasValue, () =>
        {
            RuleFor(x => x.PercentComplete)
                .InclusiveBetween(0, 100)
                .WithMessage("PercentComplete muss zwischen 0 und 100 liegen");
        });
    }
}
