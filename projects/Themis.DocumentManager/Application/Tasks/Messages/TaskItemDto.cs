using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Messages;

/// <summary>
/// Task DTO for data transfer
/// </summary>
public record TaskItemDto : BaseEntityDto
{
    public string Subject { get; init; } = string.Empty;
    public string Body { get; init; } = string.Empty;
    public DateTime? DueDate { get; init; }
    public DateTime? StartDate { get; init; }
    public OutlookTaskStatus Status { get; init; }
    public OutlookTaskPriority Priority { get; init; }
    public int PercentComplete { get; init; }
    public string? ProcessId { get; init; }
    public string? AssignedTo { get; init; }
    public string? Owner { get; init; }
    public List<string> Categories { get; init; } = new();
    public DateTime? CompletedAt { get; init; }
    public string? LinkedEntityId { get; init; }
    public LinkedEntityType? LinkedEntityType { get; init; }
}

public enum LinkedEntityType
{
    Document,
    Process,
    Inbox,
    Reminder
}
