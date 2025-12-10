using FluentValidation;

namespace Themis.DocumentManager.Application.Cosigning.Commands.ApproveCosigningStep;

/// <summary>
/// Validator for ApproveCosigningStepCommand
/// Ensures VIS Mitzeichnung compliance
/// </summary>
public class ApproveCosigningStepCommandValidator : AbstractValidator<ApproveCosigningStepCommand>
{
    public ApproveCosigningStepCommandValidator()
    {
        RuleFor(v => v.CosigningId)
            .NotEmpty().WithMessage("Mitzeichnungs-ID ist erforderlich.");

        RuleFor(v => v.CosignerId)
            .NotEmpty().WithMessage("Mitzeichner-ID ist erforderlich.");

        RuleFor(v => v.Comment)
            .MaximumLength(1000)
            .When(v => !string.IsNullOrEmpty(v.Comment))
            .WithMessage("Kommentar darf maximal 1000 Zeichen lang sein.");
    }
}
