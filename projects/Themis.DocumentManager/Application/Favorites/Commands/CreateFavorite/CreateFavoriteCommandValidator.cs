/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateFavoriteCommandValidator.cs                  ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
