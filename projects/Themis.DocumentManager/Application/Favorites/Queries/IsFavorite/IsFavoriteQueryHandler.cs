using MediatR;
using Themis.DocumentManager.Application.Favorites.Commands.AddToFavorites;

namespace Themis.DocumentManager.Application.Favorites.Queries.IsFavorite;

/// <summary>
/// Handler for IsFavoriteQuery
/// </summary>
public class IsFavoriteQueryHandler : IRequestHandler<IsFavoriteQuery, bool>
{
    private static readonly List<FavoriteEntity> _favorites = 
        AddToFavoritesCommandHandler._favorites ?? new();

    public async Task<bool> Handle(IsFavoriteQuery request, CancellationToken cancellationToken)
    {
        var isFavorite = _favorites.Any(f => 
            f.EntityId == request.EntityId && 
            f.UserId == request.UserId);

        await Task.CompletedTask;
        return isFavorite;
    }
}
