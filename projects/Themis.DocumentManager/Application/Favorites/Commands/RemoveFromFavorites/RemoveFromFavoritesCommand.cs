using MediatR;

namespace Themis.DocumentManager.Application.Favorites.Commands.RemoveFromFavorites;

/// <summary>
/// Command to remove an entity from favorites
/// </summary>
public record RemoveFromFavoritesCommand : IRequest<bool>
{
    public string EntityId { get; init; } = string.Empty;
    public string UserId { get; init; } = string.Empty;
}
