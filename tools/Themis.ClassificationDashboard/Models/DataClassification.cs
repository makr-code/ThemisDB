/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DataClassification.cs                              ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:35:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace Themis.ClassificationDashboard.Models
{
    public class DataClassification
    {
        public string EntityId { get; set; } = string.Empty;
        public string EntityType { get; set; } = string.Empty;
        public string ClassificationLevel { get; set; } = string.Empty;
        public string IsEncrypted { get; set; } = string.Empty;
        public string Owner { get; set; } = string.Empty;
        public DateTimeOffset CreatedAt { get; set; }
        public DateTimeOffset? LastReview { get; set; }
        public string ComplianceStatus { get; set; } = string.Empty;
        public string Notes { get; set; } = string.Empty;
    }
}
