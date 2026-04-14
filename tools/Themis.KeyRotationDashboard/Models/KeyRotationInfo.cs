/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeyRotationInfo.cs                                 ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:54:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace Themis.KeyRotationDashboard.Models
{
    public class KeyRotationInfo
    {
        public string KeyId { get; set; } = string.Empty;
        public string KeyType { get; set; } = string.Empty;
        public int Version { get; set; }
        public DateTimeOffset CreatedAt { get; set; }
        public DateTimeOffset? LastRotation { get; set; }
        public DateTimeOffset? NextRotation { get; set; }
        public string Status { get; set; } = string.Empty;
        public string RotationInterval { get; set; } = string.Empty;
    }
}