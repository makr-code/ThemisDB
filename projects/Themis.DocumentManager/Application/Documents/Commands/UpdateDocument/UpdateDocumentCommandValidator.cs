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
