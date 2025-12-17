using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Reminders.Messages;

namespace Themis.DocumentManager.Application.Reminders.Queries.GetReminderById;

/// <summary>
/// Query to get a reminder by ID
/// </summary>
public record GetReminderByIdQuery : IGetByIdQuery<ReminderDto>
{
    public string Id { get; init; } = string.Empty;
}
