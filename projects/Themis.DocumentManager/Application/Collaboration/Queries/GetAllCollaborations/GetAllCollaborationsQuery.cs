/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllCollaborationsQuery.cs                       ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     40                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Collaboration.Messages;

namespace Themis.DocumentManager.Application.Collaboration.Queries.GetAllCollaborations;

public record GetAllCollaborationsQuery : IGetAllQuery<CollaborationDto>
{
    public int PageNumber { get; init; } = 1;
    public int PageSize { get; init; } = 50;
    public string? SearchTerm { get; init; }
    public Dictionary<string, object>? Filters { get; init; }
    
    // Specific filters
    public string? EntityId { get; init; }
    public CollaborationEntityType? EntityType { get; init; }
    public string? UserId { get; init; }
    public CollaborationRole? Role { get; init; }
    public bool? IsActive { get; init; }
}
