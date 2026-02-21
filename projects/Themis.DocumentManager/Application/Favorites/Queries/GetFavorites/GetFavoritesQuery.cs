/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetFavoritesQuery.cs                               ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     75                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Favorites.Commands.AddToFavorites;
using Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;

namespace Themis.DocumentManager.Application.Favorites.Queries.GetFavorites;

/// <summary>
/// Query to get all favorites for a user
/// Supports filtering, sorting, and grouping
/// </summary>
public record GetFavoritesQuery : IRequest<FavoritesResult>
{
    public string UserId { get; init; } = string.Empty;
    public EntityType? EntityTypeFilter { get; init; }
    public string? CategoryFilter { get; init; }
    public FavoritesSortBy SortBy { get; init; } = FavoritesSortBy.RecentlyAdded;
    public bool GroupByType { get; init; } = true;
}

public enum FavoritesSortBy
{
    RecentlyAdded,
    RecentlyAccessed,
    MostAccessed,
    Alphabetical,
    EntityType
}

public class FavoritesResult
{
    public List<FavoriteItemDto> Items { get; set; } = new();
    public Dictionary<EntityType, List<FavoriteItemDto>> GroupedByType { get; set; } = new();
    public Dictionary<string, List<FavoriteItemDto>> GroupedByCategory { get; set; } = new();
}

public class FavoriteItemDto
{
    public string Id { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public EntityType EntityType { get; set; }
    public string EntityName { get; set; } = string.Empty;
    public string Icon { get; set; } = string.Empty;
    public string? Category { get; set; }
    public string? Tags { get; set; }
    public string? Notes { get; set; }
    public DateTime CreatedAt { get; set; }
    public DateTime? LastAccessedAt { get; set; }
    public int AccessCount { get; set; }
}
