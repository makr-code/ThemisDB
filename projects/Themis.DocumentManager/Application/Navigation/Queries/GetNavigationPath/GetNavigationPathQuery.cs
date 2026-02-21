/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetNavigationPathQuery.cs                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     36                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
