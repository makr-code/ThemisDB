using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Reminders.Messages;
using Themis.DocumentManager.Application.Reminders.Queries.GetReminderById;

namespace Themis.DocumentManager.Application.Reminders.Queries.GetReminderById;

public class GetReminderByIdQueryHandler : IRequestHandler<GetReminderByIdQuery, Result<ReminderDto>>
{
    public Task<Result<ReminderDto>> Handle(GetReminderByIdQuery request, CancellationToken cancellationToken)
    {
        try
        {
            // In-memory retrieval - this would be replaced with repository calls in production
            var dto = new ReminderDto
            {
                Id = request.Id,
                ProcessId = string.Empty,
                FileId = string.Empty,
                DocumentId = string.Empty,
                DueDate = DateTime.Now.AddDays(7),
                Subject = "Sample Reminder",
                Description = "This is a sample reminder",
                CreatedAt = DateTime.UtcNow
            };

            return Task.FromResult(Result<ReminderDto>.Ok(dto));
        }
        catch (Exception ex)
        {
            return Task.FromResult(Result<ReminderDto>.Fail($"Error retrieving reminder: {ex.Message}"));
        }
    }
}
