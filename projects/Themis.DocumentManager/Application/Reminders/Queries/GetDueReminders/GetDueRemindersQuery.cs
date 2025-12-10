using MediatR;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Reminders.Queries.GetDueReminders;

/// <summary>
/// Query to get due reminders (Fällige Wiedervorlagen)
/// Based on PDV VIS Fristenmanagement requirements
/// </summary>
public record GetDueRemindersQuery : IRequest<List<Reminder>>
{
    public DateTime? UpToDate { get; init; }
}
