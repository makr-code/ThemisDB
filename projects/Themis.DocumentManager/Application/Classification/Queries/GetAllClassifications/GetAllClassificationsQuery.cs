using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Classification.Messages;

namespace Themis.DocumentManager.Application.Classification.Queries.GetAllClassifications;

public record GetAllClassificationsQuery : IGetAllQuery<ClassificationDto>
{
    public int PageNumber { get; init; } = 1;
    public int PageSize { get; init; } = 50;
    public string? SearchTerm { get; init; }
    public Dictionary<string, object>? Filters { get; init; }
    
    // Specific filters
    public ClassificationLevel? Level { get; init; }
    public bool? IsActive { get; init; }
    public string? ParentId { get; init; }
}
