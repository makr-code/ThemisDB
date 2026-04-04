/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            RetentionModels.cs                                 ║
  Version:         0.0.36                                             ║
  Last Modified:   2026-03-30 04:13:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.AdminTools.Shared.Models;

public record RetentionPolicy
{
    public string Name { get; init; } = string.Empty;
    public bool Active { get; init; }
    public List<string> Collections { get; init; } = new();
    public int RetentionDays { get; init; }
    public DateTime? LastRun { get; init; }
}

public record RetentionPolicyListResponse
{
    public List<RetentionPolicy> Items { get; init; } = new();
    public int Total { get; init; }
}
