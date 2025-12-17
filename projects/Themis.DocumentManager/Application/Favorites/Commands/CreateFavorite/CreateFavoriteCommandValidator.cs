using FluentValidation;

namespace Themis.DocumentManager.Application.Favorites.Commands.CreateFavorite;

public class CreateFavoriteCommandValidator : AbstractValidator<CreateFavoriteCommand>
{
    public CreateFavoriteCommandValidator()
    {
        RuleFor(x => x.EntityId)
            .NotEmpty().WithMessage("Entitäts-ID darf nicht leer sein");

        RuleFor(x => x.EntityTitle)
            .NotEmpty().WithMessage("Entitätstitel darf nicht leer sein")
            .MaximumLength(200).WithMessage("Entitätstitel darf maximal 200 Zeichen lang sein");

        RuleFor(x => x.UserId)
            .NotEmpty().WithMessage("Benutzer-ID darf nicht leer sein");
    }
}
