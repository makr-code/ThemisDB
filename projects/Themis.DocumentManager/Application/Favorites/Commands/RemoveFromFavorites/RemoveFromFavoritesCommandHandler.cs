/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RemoveFromFavoritesCommandHandler.cs               ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
