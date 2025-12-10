using FluentValidation;

namespace Themis.DocumentManager.Application.Favorites.Commands.AddToFavorites;

/// <summary>
/// Validator for AddToFavoritesCommand
/// </summary>
public class AddToFavoritesCommandValidator : AbstractValidator<AddToFavoritesCommand>
{
    public AddToFavoritesCommandValidator()
    {
        RuleFor(v => v.EntityId)
            .NotEmpty().WithMessage("Entity ID ist erforderlich.");

        RuleFor(v => v.EntityName)
            .NotEmpty().WithMessage("Entity Name ist erforderlich.")
            .MaximumLength(500).WithMessage("Entity Name darf maximal 500 Zeichen lang sein.");

        RuleFor(v => v.UserId)
            .NotEmpty().WithMessage("User ID ist erforderlich.");

        RuleFor(v => v.EntityType)
            .IsInEnum().WithMessage("Ungültiger Entity-Typ.");

        RuleFor(v => v.Category)
            .MaximumLength(100)
            .When(v => !string.IsNullOrEmpty(v.Category))
            .WithMessage("Kategorie darf maximal 100 Zeichen lang sein.");

        RuleFor(v => v.Notes)
            .MaximumLength(1000)
            .When(v => !string.IsNullOrEmpty(v.Notes))
            .WithMessage("Notizen dürfen maximal 1000 Zeichen lang sein.");
    }
}
