/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ReportsModels.cs                                   ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     47                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
