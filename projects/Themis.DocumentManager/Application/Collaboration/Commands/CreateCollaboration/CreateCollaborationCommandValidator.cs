/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateCollaborationCommandValidator.cs             ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
