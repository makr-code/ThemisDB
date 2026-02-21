/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AddToFavoritesCommand.cs                           ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;

namespace Themis.DocumentManager.Application.Favorites.Commands.AddToFavorites;

/// <summary>
/// Command to add an entity to favorites
/// Every entity in the DMS can be favorited with a badge
/// </summary>
public record AddToFavoritesCommand : IRequest<bool>
{
    public string EntityId { get; init; } = string.Empty;
    public EntityType EntityType { get; init; }
    public string EntityName { get; init; } = string.Empty;
    public string UserId { get; init; } = string.Empty;
    public string? Category { get; init; } // Optional: User-defined category
    public string? Tags { get; init; }     // Optional: User-defined tags
    public string? Notes { get; init; }    // Optional: User notes
}
