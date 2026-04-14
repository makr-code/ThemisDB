/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeysModels.cs                                      ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     46                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
