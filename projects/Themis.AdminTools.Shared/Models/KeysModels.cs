/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeysModels.cs                                      ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     49                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 60d127110  2025-12-09  feat: Add comprehensive test report for ThemisDB Document... ║
    • 20c4e9c84  2025-11-02  feat: Complete feature set - Auth, Governance, Compliance... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.AdminTools.Shared.Models;

public record KeyInfo
{
    public string Id { get; init; } = string.Empty;
    public int Version { get; init; }
    public string Status { get; init; } = "Active"; // Active/Deprecated/Expired
    public DateTime Created { get; init; }
    public DateTime? Expires { get; init; }
}

public record KeyListResponse
{
    public List<KeyInfo> Items { get; init; } = new();
    public int Total { get; init; }
}

public record RotationResult
{
    public string KeyId { get; init; } = string.Empty;
    public int NewVersion { get; init; }
    public bool Success { get; init; }
    public string? Message { get; init; }
}
