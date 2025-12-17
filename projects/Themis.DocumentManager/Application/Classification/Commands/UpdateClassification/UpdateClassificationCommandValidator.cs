using FluentValidation;

namespace Themis.DocumentManager.Application.Classification.Commands.UpdateClassification;

public class UpdateClassificationCommandValidator : AbstractValidator<UpdateClassificationCommand>
{
    public UpdateClassificationCommandValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty()
            .WithMessage("Classification ID ist erforderlich");

        When(x => !string.IsNullOrEmpty(x.Name), () =>
        {
            RuleFor(x => x.Name)
                .MaximumLength(200)
                .WithMessage("Name darf maximal 200 Zeichen haben");
        });

        When(x => !string.IsNullOrEmpty(x.Code), () =>
        {
            RuleFor(x => x.Code)
                .MaximumLength(50)
                .WithMessage("Code darf maximal 50 Zeichen haben");
        });

        When(x => x.SortOrder.HasValue, () =>
        {
            RuleFor(x => x.SortOrder)
                .GreaterThanOrEqualTo(0)
                .WithMessage("SortOrder muss >= 0 sein");
        });
    }
}
