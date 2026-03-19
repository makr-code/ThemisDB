/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FavoriteDto.cs                                     ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
