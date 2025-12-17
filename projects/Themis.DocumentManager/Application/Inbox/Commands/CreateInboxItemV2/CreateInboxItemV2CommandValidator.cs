using FluentValidation;

namespace Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2;

public class CreateInboxItemV2CommandValidator : AbstractValidator<CreateInboxItemV2Command>
{
    public CreateInboxItemV2CommandValidator()
    {
        RuleFor(x => x.Subject)
            .NotEmpty().WithMessage("Betreff ist erforderlich")
            .MaximumLength(200).WithMessage("Betreff darf maximal 200 Zeichen lang sein");

        RuleFor(x => x.Sender)
            .NotEmpty().WithMessage("Absender ist erforderlich")
            .MaximumLength(100).WithMessage("Absender darf maximal 100 Zeichen lang sein");

        RuleFor(x => x.SenderEmail)
            .EmailAddress().When(x => !string.IsNullOrEmpty(x.SenderEmail))
            .WithMessage("Ungültige E-Mail-Adresse");
    }
}
