using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Classification.Messages;

public record ClassificationDto : BaseEntityDto
{
    public string Name { get; init; } = string.Empty;
    public string? Description { get; init; }
    public string? Code { get; init; }
    public ClassificationLevel Level { get; init; }
    public string? Color { get; init; }
    public int SortOrder { get; init; }
    public bool IsActive { get; init; }
    public string? ParentId { get; init; }
    public List<string> AllowedRoles { get; init; } = new();
    public Dictionary<string, object> Metadata { get; init; } = new();
}

public enum ClassificationLevel
{
    Public,
    Internal,
    Confidential,
    Secret,
    TopSecret
}
