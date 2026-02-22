/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateDocumentCommandValidator.cs                  ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
