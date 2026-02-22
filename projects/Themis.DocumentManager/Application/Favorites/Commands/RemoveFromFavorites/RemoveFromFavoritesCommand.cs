/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RemoveFromFavoritesCommand.cs                      ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
