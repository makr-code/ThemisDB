/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCosigningCommandValidator.cs                 ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;
using Themis.DocumentManager.Application.Cosigning.Messages;

namespace Themis.DocumentManager.Application.Cosigning.Commands.UpdateCosigning;

public class UpdateCosigningCommandValidator : AbstractValidator<UpdateCosigningCommand>
{
    public UpdateCosigningCommandValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty().WithMessage("Cosigning ID ist erforderlich");

        RuleFor(x => x.RejectionReason)
            .NotEmpty().When(x => x.Status == CosigningStatus.Rejected)
            .WithMessage("Ablehnungsgrund ist erforderlich bei Ablehnung");

        RuleFor(x => x.SignatureData)
            .NotEmpty().When(x => x.Status == CosigningStatus.Signed)
            .WithMessage("Signaturdaten sind erforderlich bei Unterzeichnung");

        RuleFor(x => x.SignOrder)
            .GreaterThan(0).When(x => x.SignOrder.HasValue)
            .WithMessage("Unterschriftsreihenfolge muss größer als 0 sein");
    }
}
