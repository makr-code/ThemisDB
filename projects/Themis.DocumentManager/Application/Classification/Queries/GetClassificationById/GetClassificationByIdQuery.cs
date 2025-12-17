using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Classification.Messages;

namespace Themis.DocumentManager.Application.Classification.Queries.GetClassificationById;

public record GetClassificationByIdQuery : IGetByIdQuery<ClassificationDto>
{
    public string Id { get; init; } = string.Empty;
}
