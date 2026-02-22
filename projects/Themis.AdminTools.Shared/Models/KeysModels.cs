/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeysModels.cs                                      ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
