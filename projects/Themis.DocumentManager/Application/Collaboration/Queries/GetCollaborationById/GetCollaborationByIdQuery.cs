/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetCollaborationByIdQuery.cs                       ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-03-02 03:55:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     29                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Collaboration.Messages;

namespace Themis.DocumentManager.Application.Collaboration.Queries.GetCollaborationById;

public record GetCollaborationByIdQuery : IGetByIdQuery<CollaborationDto>
{
    public string Id { get; init; } = string.Empty;
}
