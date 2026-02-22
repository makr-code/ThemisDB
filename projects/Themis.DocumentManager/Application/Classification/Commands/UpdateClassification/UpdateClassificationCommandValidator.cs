/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateClassificationCommandValidator.cs            ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Classification.Commands.UpdateClassification;

public class UpdateClassificationCommandValidator : AbstractValidator<UpdateClassificationCommand>
{
    public UpdateClassificationCommandValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty()
            .WithMessage("Classification ID ist erforderlich");

        When(x => !string.IsNullOrEmpty(x.Name), () =>
        {
            RuleFor(x => x.Name)
                .MaximumLength(200)
                .WithMessage("Name darf maximal 200 Zeichen haben");
        });

        When(x => !string.IsNullOrEmpty(x.Code), () =>
        {
            RuleFor(x => x.Code)
                .MaximumLength(50)
                .WithMessage("Code darf maximal 50 Zeichen haben");
        });

        When(x => x.SortOrder.HasValue, () =>
        {
            RuleFor(x => x.SortOrder)
                .GreaterThanOrEqualTo(0)
                .WithMessage("SortOrder muss >= 0 sein");
        });
    }
}
