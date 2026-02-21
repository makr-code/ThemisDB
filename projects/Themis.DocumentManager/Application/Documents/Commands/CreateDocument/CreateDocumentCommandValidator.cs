/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateDocumentCommandValidator.cs                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Documents.Commands.CreateDocument;

/// <summary>
/// Validator for CreateDocumentCommand
/// </summary>
public class CreateDocumentCommandValidator : AbstractValidator<CreateDocumentCommand>
{
    public CreateDocumentCommandValidator()
    {
        RuleFor(v => v.Title)
            .NotEmpty().WithMessage("Title ist erforderlich.")
            .MaximumLength(200).WithMessage("Title darf maximal 200 Zeichen lang sein.");

        RuleFor(v => v.Filename)
            .NotEmpty().WithMessage("Filename ist erforderlich.")
            .MaximumLength(255).WithMessage("Filename darf maximal 255 Zeichen lang sein.");

        RuleFor(v => v.Author)
            .NotEmpty().WithMessage("Author ist erforderlich.")
            .MaximumLength(100).WithMessage("Author darf maximal 100 Zeichen lang sein.");

        RuleFor(v => v.MimeType)
            .NotEmpty().WithMessage("MimeType ist erforderlich.")
            .Matches(@"^[\w.-]+/[\w.-]+$").WithMessage("MimeType muss ein gültiges Format haben (z.B. application/pdf).");

        RuleFor(v => v.SizeBytes)
            .GreaterThan(0).WithMessage("SizeBytes muss größer als 0 sein.");
    }
}
