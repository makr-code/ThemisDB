/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ClassificationModels.cs                            ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:29:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
