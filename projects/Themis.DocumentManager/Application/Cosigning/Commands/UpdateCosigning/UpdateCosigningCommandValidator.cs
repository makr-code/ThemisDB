/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCosigningCommandValidator.cs                 ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
