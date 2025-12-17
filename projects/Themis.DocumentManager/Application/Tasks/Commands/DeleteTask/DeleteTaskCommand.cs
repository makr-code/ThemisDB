using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;

namespace Themis.DocumentManager.Application.Tasks.Commands.DeleteTask;

/// <summary>
/// Command to delete a task
/// </summary>
public record DeleteTaskCommand : IDeleteCommand
{
    public string Id { get; init; } = string.Empty;
}
