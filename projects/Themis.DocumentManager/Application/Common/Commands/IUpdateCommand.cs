using MediatR;

namespace Themis.DocumentManager.Application.Common.Commands;

/// <summary>
/// Generic interface for Update commands
/// </summary>
public interface IUpdateCommand : IRequest<Result>
{
    /// <summary>
    /// ID of the entity to update
    /// </summary>
    string Id { get; init; }
}
