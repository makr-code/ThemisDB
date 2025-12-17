using FluentValidation;
using Themis.DocumentManager.Application.Reminders.Queries.GetReminderById;

namespace Themis.DocumentManager.Application.Reminders.Queries.GetReminderById;

public class GetReminderByIdQueryValidator : AbstractValidator<GetReminderByIdQuery>
{
    public GetReminderByIdQueryValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty().WithMessage("Id is required");
    }
}
