/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RemoveFromFavoritesCommandHandler.cs               ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
