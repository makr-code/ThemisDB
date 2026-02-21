/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ReportsModels.cs                                   ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:04:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     46                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 20c4e9c84  2025-11-02  feat: Complete feature set - Auth, Governance, Compliance... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.AdminTools.Shared.Models;

public record ComplianceReport
{
    public string Id { get; init; } = string.Empty;
    public string Type { get; init; } = string.Empty; // DSGVO/SOX/HIPAA/ISO27001/PCI-DSS
    public DateTime CreatedAt { get; init; }
    public DateTime GeneratedAt { get; init; }
    public string? CreatedBy { get; init; }
    public DateTime PeriodStart { get; init; }
    public DateTime PeriodEnd { get; init; }
    public string Status { get; init; } = "Ready";
    public int RecordCount { get; init; }
    public string? Description { get; init; }
}

public record ComplianceReportListResponse
{
    public List<ComplianceReport> Items { get; init; } = new();
    public int Total { get; init; }
}
