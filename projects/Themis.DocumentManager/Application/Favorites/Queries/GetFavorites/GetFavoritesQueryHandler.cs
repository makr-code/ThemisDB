/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetFavoritesQueryHandler.cs                        ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     118                                            ║
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
using Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;

namespace Themis.DocumentManager.Application.Favorites.Queries.GetFavorites;

/// <summary>
/// Handler for GetFavoritesQuery
/// Returns user's favorites with filtering and grouping
/// </summary>
public class GetFavoritesQueryHandler : IRequestHandler<GetFavoritesQuery, FavoritesResult>
{
    private static readonly List<FavoriteEntity> _favorites = 
        AddToFavoritesCommandHandler._favorites ?? new();

    public async Task<FavoritesResult> Handle(GetFavoritesQuery request, CancellationToken cancellationToken)
    {
        var result = new FavoritesResult();

        // Get user's favorites
        var userFavorites = _favorites.Where(f => f.UserId == request.UserId).ToList();

        // Apply filters
        if (request.EntityTypeFilter.HasValue)
        {
            userFavorites = userFavorites
                .Where(f => f.EntityType == request.EntityTypeFilter.Value)
                .ToList();
        }

        if (!string.IsNullOrEmpty(request.CategoryFilter))
        {
            userFavorites = userFavorites
                .Where(f => f.Category == request.CategoryFilter)
                .ToList();
        }

        // Apply sorting
        userFavorites = request.SortBy switch
        {
            FavoritesSortBy.RecentlyAdded => userFavorites.OrderByDescending(f => f.CreatedAt).ToList(),
            FavoritesSortBy.RecentlyAccessed => userFavorites.OrderByDescending(f => f.LastAccessedAt ?? DateTime.MinValue).ToList(),
            FavoritesSortBy.MostAccessed => userFavorites.OrderByDescending(f => f.AccessCount).ToList(),
            FavoritesSortBy.Alphabetical => userFavorites.OrderBy(f => f.EntityName).ToList(),
            FavoritesSortBy.EntityType => userFavorites.OrderBy(f => f.EntityType).ToList(),
            _ => userFavorites
        };

        // Convert to DTOs
        result.Items = userFavorites.Select(f => new FavoriteItemDto
        {
            Id = f.Id,
            EntityId = f.EntityId,
            EntityType = f.EntityType,
            EntityName = f.EntityName,
            Icon = GetIconForEntityType(f.EntityType),
            Category = f.Category,
            Tags = f.Tags,
            Notes = f.Notes,
            CreatedAt = f.CreatedAt,
            LastAccessedAt = f.LastAccessedAt,
            AccessCount = f.AccessCount
        }).ToList();

        // Group by type if requested
        if (request.GroupByType)
        {
            result.GroupedByType = result.Items
                .GroupBy(f => f.EntityType)
                .ToDictionary(g => g.Key, g => g.ToList());
        }

        // Group by category
        result.GroupedByCategory = result.Items
            .Where(f => !string.IsNullOrEmpty(f.Category))
            .GroupBy(f => f.Category!)
            .ToDictionary(g => g.Key, g => g.ToList());

        await Task.CompletedTask;
        return result;
    }

    private string GetIconForEntityType(EntityType type) => type switch
    {
        EntityType.Authority => "🏛️",
        EntityType.Repository => "📁",
        EntityType.File => "📂",
        EntityType.Process => "📋",
        EntityType.Document => "📄",
        _ => "⭐"
    };
}
