/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationModels.cs                            ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:49:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
