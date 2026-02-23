/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateInboxItemCommandValidator.cs                 ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
