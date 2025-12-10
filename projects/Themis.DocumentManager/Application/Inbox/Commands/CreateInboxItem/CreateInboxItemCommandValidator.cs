using FluentValidation;

namespace Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItem;

/// <summary>
/// Validator for CreateInboxItemCommand
/// Ensures VIS Posteingang compliance
/// </summary>
public class CreateInboxItemCommandValidator : AbstractValidator<CreateInboxItemCommand>
{
    public CreateInboxItemCommandValidator()
    {
        RuleFor(v => v.Subject)
            .NotEmpty().WithMessage("Betreff ist erforderlich.")
            .MaximumLength(500).WithMessage("Betreff darf maximal 500 Zeichen lang sein.");

        RuleFor(v => v.Sender)
            .NotEmpty().WithMessage("Absender ist erforderlich.")
            .MaximumLength(200).WithMessage("Absender darf maximal 200 Zeichen lang sein.");

        RuleFor(v => v.Priority)
            .IsInEnum().WithMessage("Ungültige Priorität.");
    }
}
