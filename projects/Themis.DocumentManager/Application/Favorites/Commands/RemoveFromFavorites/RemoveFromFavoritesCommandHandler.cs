using MediatR;
using Themis.DocumentManager.Application.Favorites.Commands.AddToFavorites;

namespace Themis.DocumentManager.Application.Favorites.Commands.RemoveFromFavorites;

/// <summary>
/// Handler for RemoveFromFavoritesCommand
/// </summary>
public class RemoveFromFavoritesCommandHandler : IRequestHandler<RemoveFromFavoritesCommand, bool>
{
    // Access to shared favorites list (in real app, use repository)
    private static readonly List<FavoriteEntity> _favorites = 
        AddToFavoritesCommandHandler._favorites ?? new();

    public async Task<bool> Handle(RemoveFromFavoritesCommand request, CancellationToken cancellationToken)
    {
        var favorite = _favorites.FirstOrDefault(f => 
            f.EntityId == request.EntityId && 
            f.UserId == request.UserId);

        if (favorite == null)
            return false;

        _favorites.Remove(favorite);

        await Task.CompletedTask;
        return true;
    }
}
