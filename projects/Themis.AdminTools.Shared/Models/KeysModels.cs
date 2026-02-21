/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeysModels.cs                                      ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
