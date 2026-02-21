/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FavoriteDto.cs                                     ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     49                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Favorites.Messages;

public record FavoriteDto : BaseEntityDto
{
    public string EntityId { get; init; } = string.Empty;
    public FavoriteEntityType EntityType { get; init; }
    public string EntityTitle { get; init; } = string.Empty;
    public string? EntityDescription { get; init; }
    public string UserId { get; init; } = string.Empty;
    public int SortOrder { get; init; }
    public Dictionary<string, object> Metadata { get; init; } = new();
}

public enum FavoriteEntityType
{
    Document,
    Process,
    Folder,
    Search,
    Report
}
