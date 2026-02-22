/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IsFavoriteQueryHandler.cs                          ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     42                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
