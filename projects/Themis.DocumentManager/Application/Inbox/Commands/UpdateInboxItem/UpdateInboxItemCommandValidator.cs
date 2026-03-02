/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateInboxItemCommandValidator.cs                 ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-03-02 03:55:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     31                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentValidation;

namespace Themis.DocumentManager.Application.Inbox.Commands.UpdateInboxItem;

public class UpdateInboxItemCommandValidator : AbstractValidator<UpdateInboxItemCommand>
{
    public UpdateInboxItemCommandValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty().WithMessage("Inbox-ID ist erforderlich");
    }
}
