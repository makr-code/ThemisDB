/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetInboxItemsQuery.cs                              ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:56:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     33                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Inbox.Queries.GetInboxItems;

/// <summary>
/// Query to get inbox items (Posteingang)
/// Based on PDV VIS requirements
/// </summary>
public record GetInboxItemsQuery : IRequest<List<InboxItem>>
{
    public InboxStatus? Status { get; init; }
    public string? AssignedTo { get; init; }
}
