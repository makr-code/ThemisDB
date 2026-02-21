/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateCosigningCommandValidator.cs                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     30                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Cosigning.Commands.CreateCosigning;

public class CreateCosigningCommandValidator : AbstractValidator<CreateCosigningCommand>
{
    public CreateCosigningCommandValidator()
    {
        RuleFor(x => x.DocumentId)
            .NotEmpty().WithMessage("Document ID ist erforderlich");

        RuleFor(x => x.DocumentName)
            .NotEmpty().WithMessage("Dokumentname ist erforderlich")
            .MaximumLength(200).WithMessage("Dokumentname darf maximal 200 Zeichen lang sein");

        RuleFor(x => x.SignerId)
            .NotEmpty().WithMessage("Unterzeichner ID ist erforderlich");

        RuleFor(x => x.SignerName)
            .NotEmpty().WithMessage("Unterzeichner-Name ist erforderlich")
            .MaximumLength(100).WithMessage("Unterzeichner-Name darf maximal 100 Zeichen lang sein");

        RuleFor(x => x.SignerEmail)
            .EmailAddress().When(x => !string.IsNullOrEmpty(x.SignerEmail))
            .WithMessage("Ungültige E-Mail-Adresse");

        RuleFor(x => x.SignOrder)
            .GreaterThan(0).WithMessage("Unterschriftsreihenfolge muss größer als 0 sein");
    }
}
