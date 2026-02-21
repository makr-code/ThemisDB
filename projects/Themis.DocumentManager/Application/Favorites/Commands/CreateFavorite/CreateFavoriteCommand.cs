/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateFavoriteCommand.cs                           ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     42                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;
using Themis.DocumentManager.Application.Favorites.Messages;

namespace Themis.DocumentManager.Application.Favorites.Commands.CreateFavorite;

public record CreateFavoriteCommand : ICreateCommand<FavoriteDto>
{
    public string EntityId { get; init; } = string.Empty;
    public FavoriteEntityType EntityType { get; init; }
    public string EntityTitle { get; init; } = string.Empty;
    public string? EntityDescription { get; init; }
    public string UserId { get; init; } = string.Empty;
    public int SortOrder { get; init; }
    public Dictionary<string, object> Metadata { get; init; } = new();
}
