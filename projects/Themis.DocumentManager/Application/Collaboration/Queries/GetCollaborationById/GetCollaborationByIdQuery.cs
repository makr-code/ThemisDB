using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Collaboration.Messages;

namespace Themis.DocumentManager.Application.Collaboration.Queries.GetCollaborationById;

public record GetCollaborationByIdQuery : IGetByIdQuery<CollaborationDto>
{
    public string Id { get; init; } = string.Empty;
}
