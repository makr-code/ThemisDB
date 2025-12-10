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
