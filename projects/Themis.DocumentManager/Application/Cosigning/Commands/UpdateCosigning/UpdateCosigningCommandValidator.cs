/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCosigningCommandValidator.cs                 ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
