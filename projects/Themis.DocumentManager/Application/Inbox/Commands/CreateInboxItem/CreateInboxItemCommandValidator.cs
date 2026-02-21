/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateInboxItemCommandValidator.cs                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 5de9cc853  2025-12-10  Integrate VIS features (Inbox, Reminders, Cosigning) into... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItem;

/// <summary>
/// Validator for CreateInboxItemCommand
/// Ensures VIS Posteingang compliance
/// </summary>
public class CreateInboxItemCommandValidator : AbstractValidator<CreateInboxItemCommand>
{
    public CreateInboxItemCommandValidator()
    {
        RuleFor(v => v.Subject)
            .NotEmpty().WithMessage("Betreff ist erforderlich.")
            .MaximumLength(500).WithMessage("Betreff darf maximal 500 Zeichen lang sein.");

        RuleFor(v => v.Sender)
            .NotEmpty().WithMessage("Absender ist erforderlich.")
            .MaximumLength(200).WithMessage("Absender darf maximal 200 Zeichen lang sein.");

        RuleFor(v => v.Priority)
            .IsInEnum().WithMessage("Ungültige Priorität.");
    }
}
