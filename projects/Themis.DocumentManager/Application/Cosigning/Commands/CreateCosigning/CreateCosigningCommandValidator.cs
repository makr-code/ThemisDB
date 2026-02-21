/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateCosigningCommandValidator.cs                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     55                                             ║
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
