/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateDocumentCommandValidator.cs                  ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
