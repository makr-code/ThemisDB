using MediatR;
using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Common.Queries;

/// <summary>
/// Generic interface for GetById queries
/// </summary>
/// <typeparam name="TDto">The DTO type to return</typeparam>
public interface IGetByIdQuery<TDto> : IRequest<Result<TDto>>
    where TDto : IEntityDto
{
    /// <summary>
    /// ID of the entity to retrieve
    /// </summary>
    string Id { get; init; }
}
