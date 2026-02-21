/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AddToFavoritesCommandValidator.cs                  ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     61                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
