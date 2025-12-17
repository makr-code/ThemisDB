using FluentValidation;

namespace Themis.DocumentManager.Application.Collaboration.Commands.CreateCollaboration;

public class CreateCollaborationCommandValidator : AbstractValidator<CreateCollaborationCommand>
{
    public CreateCollaborationCommandValidator()
    {
        RuleFor(x => x.EntityId)
            .NotEmpty()
            .WithMessage("EntityId ist erforderlich");

        RuleFor(x => x.UserId)
            .NotEmpty()
            .WithMessage("UserId ist erforderlich");

        RuleFor(x => x.UserName)
            .NotEmpty()
            .WithMessage("UserName ist erforderlich")
            .MaximumLength(200)
            .WithMessage("UserName darf maximal 200 Zeichen haben");

        When(x => !string.IsNullOrEmpty(x.UserEmail), () =>
        {
            RuleFor(x => x.UserEmail)
                .EmailAddress()
                .WithMessage("Ungültige E-Mail-Adresse");
        });

        When(x => x.AccessExpiresAt.HasValue, () =>
        {
            RuleFor(x => x.AccessExpiresAt)
                .GreaterThan(DateTime.UtcNow)
                .WithMessage("AccessExpiresAt muss in der Zukunft liegen");
        });
    }
}
