using MediatR;

namespace Themis.DocumentManager.Application.Favorites.Queries.IsFavorite;

/// <summary>
/// Query to check if an entity is favorited
/// Used to show/hide favorite badge
/// </summary>
public record IsFavoriteQuery : IRequest<bool>
{
    public string EntityId { get; init; } = string.Empty;
    public string UserId { get; init; } = string.Empty;
}
