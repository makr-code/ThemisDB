/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateInboxItemV2CommandValidator.cs               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     21                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2;

public class CreateInboxItemV2CommandValidator : AbstractValidator<CreateInboxItemV2Command>
{
    public CreateInboxItemV2CommandValidator()
    {
        RuleFor(x => x.Subject)
            .NotEmpty().WithMessage("Betreff ist erforderlich")
            .MaximumLength(200).WithMessage("Betreff darf maximal 200 Zeichen lang sein");

        RuleFor(x => x.Sender)
            .NotEmpty().WithMessage("Absender ist erforderlich")
            .MaximumLength(100).WithMessage("Absender darf maximal 100 Zeichen lang sein");

        RuleFor(x => x.SenderEmail)
            .EmailAddress().When(x => !string.IsNullOrEmpty(x.SenderEmail))
            .WithMessage("Ungültige E-Mail-Adresse");
    }
}
