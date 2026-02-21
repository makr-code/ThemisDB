/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCosigningCommandValidator.cs                 ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
