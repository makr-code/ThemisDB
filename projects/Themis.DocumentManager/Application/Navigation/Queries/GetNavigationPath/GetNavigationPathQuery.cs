/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetNavigationPathQuery.cs                          ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     61                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b01a2e3c3  2025-12-10  Add intelligent breadcrumb navigation and configurable fa... ║
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
