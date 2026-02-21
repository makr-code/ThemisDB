/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeyRotationInfo.cs                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     16                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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