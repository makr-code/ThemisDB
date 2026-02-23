/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ApproveCosigningStepCommandValidator.cs            ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Cosigning.Commands.ApproveCosigningStep;

/// <summary>
/// Validator for ApproveCosigningStepCommand
/// Ensures VIS Mitzeichnung compliance
/// </summary>
public class ApproveCosigningStepCommandValidator : AbstractValidator<ApproveCosigningStepCommand>
{
    public ApproveCosigningStepCommandValidator()
    {
        RuleFor(v => v.CosigningId)
            .NotEmpty().WithMessage("Mitzeichnungs-ID ist erforderlich.");

        RuleFor(v => v.CosignerId)
            .NotEmpty().WithMessage("Mitzeichner-ID ist erforderlich.");

        RuleFor(v => v.Comment)
            .MaximumLength(1000)
            .When(v => !string.IsNullOrEmpty(v.Comment))
            .WithMessage("Kommentar darf maximal 1000 Zeichen lang sein.");
    }
}
