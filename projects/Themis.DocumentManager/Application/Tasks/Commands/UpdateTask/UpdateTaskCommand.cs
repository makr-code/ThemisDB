using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;
using Themis.DocumentManager.Application.Tasks.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Commands.UpdateTask;

/// <summary>
/// Command to update an existing task
/// </summary>
public record UpdateTaskCommand : IUpdateCommand
{
    public string Id { get; init; } = string.Empty;
    public string? Subject { get; init; }
    public string? Body { get; init; }
    public DateTime? DueDate { get; init; }
    public DateTime? StartDate { get; init; }
    public OutlookTaskStatus? Status { get; init; }
    public OutlookTaskPriority? Priority { get; init; }
    public int? PercentComplete { get; init; }
    public string? ProcessId { get; init; }
    public string? AssignedTo { get; init; }
    public string? Owner { get; init; }
    public List<string>? Categories { get; init; }
    public string? LinkedEntityId { get; init; }
    public string? LinkedEntityType { get; init; }
}
