using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Reminders.Commands.DeleteReminder;

namespace Themis.DocumentManager.Application.Reminders.Commands.DeleteReminder;

public class DeleteReminderCommandHandler : IRequestHandler<DeleteReminderCommand, Result>
{
    public Task<Result> Handle(DeleteReminderCommand request, CancellationToken cancellationToken)
    {
        try
        {
            // In-memory deletion - this would be replaced with repository calls in production
            return Task.FromResult(Result.Ok());
        }
        catch (Exception ex)
        {
            return Task.FromResult(Result.Fail($"Error deleting reminder: {ex.Message}"));
        }
    }
}
