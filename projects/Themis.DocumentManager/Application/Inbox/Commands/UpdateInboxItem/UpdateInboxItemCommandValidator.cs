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
