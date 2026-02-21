/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetRelatedEntitiesQuery.cs                         ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     66                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;

namespace Themis.DocumentManager.Application.Navigation.Queries.GetRelatedEntities;

/// <summary>
/// Query to get related entities for intelligent navigation suggestions
/// Uses user history and AI predictions
/// </summary>
public record GetRelatedEntitiesQuery : IRequest<RelatedEntitiesResult>
{
    public string EntityId { get; init; } = string.Empty;
    public EntityType EntityType { get; init; }
    public string UserId { get; init; } = string.Empty;
}

public class RelatedEntitiesResult
{
    public List<RelatedEntityGroup> Groups { get; set; } = new();
}

public class RelatedEntityGroup
{
    public string GroupName { get; set; } = string.Empty;
    public EntityType TargetType { get; set; }
    public List<RelatedEntity> Entities { get; set; } = new();
}

public class RelatedEntity
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public EntityType Type { get; set; }
    public double RelevanceScore { get; set; } // AI-predicted relevance (0-1)
    public string RelevanceReason { get; set; } = string.Empty; // Why this is suggested
    public DateTime? LastAccessedAt { get; set; }
    public int AccessCount { get; set; }
    public bool IsFrequentlyAccessed { get; set; }
    public bool IsSimilar { get; set; }
}
