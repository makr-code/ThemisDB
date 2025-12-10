using MediatR;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Reminders.Commands.CreateReminder;

/// <summary>
/// Command to create a reminder (Wiedervorlage)
/// Based on PDV VIS Fristenmanagement requirements
/// </summary>
public record CreateReminderCommand : IRequest<string>
{
    public string ProcessId { get; init; } = string.Empty;
    public DateTime DueDate { get; init; }
    public DateTime? ReminderDate { get; init; }
    public ReminderType Type { get; init; }
    public string AssignedTo { get; init; } = string.Empty;
    public string? Description { get; init; }
    public List<EscalationLevelDto>? EscalationLevels { get; init; }
}

public record EscalationLevelDto
{
    public int Level { get; init; }
    public int DaysBeforeDue { get; init; }
    public string EscalateTo { get; init; } = string.Empty;
}
