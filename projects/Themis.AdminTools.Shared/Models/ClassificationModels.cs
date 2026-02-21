/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationModels.cs                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.AdminTools.Shared.Models;

public record ClassificationStat
{
    public string Classification { get; init; } = string.Empty; // PUBLIC/INTERNAL/CONFIDENTIAL/RESTRICTED
    public long Count { get; init; }
}

public record ClassificationDetail
{
    public string EntityId { get; init; } = string.Empty;
    public string EntityType { get; init; } = string.Empty;
    public string Classification { get; init; } = string.Empty;
    public bool IsEncrypted { get; init; }
    public string? Owner { get; init; }
    public DateTime CreatedAt { get; init; }
    public DateTime? LastReview { get; init; }
    public bool IsCompliant { get; init; }
}

public record ClassificationStatsResponse
{
    public List<ClassificationStat> Stats { get; init; } = new();
    public List<ClassificationDetail> Items { get; init; } = new();
    public long Total { get; init; }
}
