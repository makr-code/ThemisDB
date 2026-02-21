/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetNavigationPathQuery.cs                          ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     62                                             ║
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

using MediatR;

namespace Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;

/// <summary>
/// Query to get current navigation path (breadcrumb)
/// Example: Behörde > Ablage > Akte > Vorgang > Dokument
/// </summary>
public record GetNavigationPathQuery : IRequest<NavigationPath>
{
    public string EntityId { get; init; } = string.Empty;
    public EntityType EntityType { get; init; }
}

public enum EntityType
{
    Authority,      // Behörde
    Repository,     // Ablage
    File,           // Akte
    Process,        // Vorgang
    Document        // Dokument
}

public class NavigationPath
{
    public List<NavigationPathItem> Items { get; set; } = new();
}

public class NavigationPathItem
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public EntityType Type { get; set; }
    public int Level { get; set; }
    public bool IsCurrentItem { get; set; }
}
