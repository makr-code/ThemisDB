/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateFavoriteCommandValidator.cs                  ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     41                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
