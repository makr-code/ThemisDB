using FluentValidation;
using Themis.DocumentManager.Application.Reminders.Commands.UpdateReminder;

namespace Themis.DocumentManager.Application.Reminders.Commands.UpdateReminder;

public class UpdateReminderCommandValidator : AbstractValidator<UpdateReminderCommand>
{
    public UpdateReminderCommandValidator()
    {
        RuleFor(x => x.Id)
            .NotEmpty().WithMessage("Id is required");

        RuleFor(x => x.Subject)
            .MaximumLength(255).WithMessage("Subject must not exceed 255 characters")
            .When(x => !string.IsNullOrEmpty(x.Subject));

        RuleFor(x => x.DueDate)
            .GreaterThan(DateTime.Now).WithMessage("DueDate must be in the future")
            .When(x => x.DueDate.HasValue);
    }
}
