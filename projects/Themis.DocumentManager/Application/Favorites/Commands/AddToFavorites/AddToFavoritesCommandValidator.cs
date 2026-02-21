/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AddToFavoritesCommandValidator.cs                  ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     61                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
