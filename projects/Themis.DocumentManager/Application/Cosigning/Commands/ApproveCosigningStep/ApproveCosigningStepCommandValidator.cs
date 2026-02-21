/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ApproveCosigningStepCommandValidator.cs            ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
