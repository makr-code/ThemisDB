/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateDocumentCommandValidator.cs                  ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Documents.Commands.UpdateDocument;

/// <summary>
/// Validator for UpdateDocumentCommand
/// </summary>
public class UpdateDocumentCommandValidator : AbstractValidator<UpdateDocumentCommand>
{
    public UpdateDocumentCommandValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty().WithMessage("Dokument-ID darf nicht leer sein");

        When(x => x.Title != null, () =>
        {
            RuleFor(x => x.Title)
                .NotEmpty().WithMessage("Titel darf nicht leer sein")
                .MaximumLength(500).WithMessage("Titel darf maximal 500 Zeichen lang sein");
        });
    }
}
