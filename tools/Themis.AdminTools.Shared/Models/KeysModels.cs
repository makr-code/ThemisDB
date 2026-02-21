/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeysModels.cs                                      ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
