using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;
using Themis.DocumentManager.Application.Favorites.Messages;

namespace Themis.DocumentManager.Application.Favorites.Commands.CreateFavorite;

public record CreateFavoriteCommand : ICreateCommand<FavoriteDto>
{
    public string EntityId { get; init; } = string.Empty;
    public FavoriteEntityType EntityType { get; init; }
    public string EntityTitle { get; init; } = string.Empty;
    public string? EntityDescription { get; init; }
    public string UserId { get; init; } = string.Empty;
    public int SortOrder { get; init; }
    public Dictionary<string, object> Metadata { get; init; } = new();
}
