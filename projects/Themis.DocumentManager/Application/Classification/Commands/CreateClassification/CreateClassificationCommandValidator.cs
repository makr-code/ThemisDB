using FluentValidation;

namespace Themis.DocumentManager.Application.Classification.Commands.CreateClassification;

public class CreateClassificationCommandValidator : AbstractValidator<CreateClassificationCommand>
{
    public CreateClassificationCommandValidator()
    {
        RuleFor(x => x.Name)
            .NotEmpty()
            .WithMessage("Name ist erforderlich")
            .MaximumLength(200)
            .WithMessage("Name darf maximal 200 Zeichen haben");

        When(x => !string.IsNullOrEmpty(x.Code), () =>
        {
            RuleFor(x => x.Code)
                .MaximumLength(50)
                .WithMessage("Code darf maximal 50 Zeichen haben");
        });

        When(x => !string.IsNullOrEmpty(x.Description), () =>
        {
            RuleFor(x => x.Description)
                .MaximumLength(1000)
                .WithMessage("Description darf maximal 1000 Zeichen haben");
        });

        RuleFor(x => x.SortOrder)
            .GreaterThanOrEqualTo(0)
            .WithMessage("SortOrder muss >= 0 sein");
    }
}
