/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NotificationModels.cs                              ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     53                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

#nullable enable

/// <summary>
/// Supported notification delivery channels.
/// </summary>
public enum NotificationChannel
{
    InApp,
    Desktop,
    Email,
    SMS
}

/// <summary>
/// Extended notification payload used by orchestrator and notification service.
/// </summary>
public class EnhancedNotification
{
    public NotificationType Type { get; set; }
    public NotificationPriority Priority { get; set; }
    public IEnumerable<NotificationChannel> Channels { get; set; } = new List<NotificationChannel>();
    public Dictionary<string, object> TemplateData { get; set; } = new();
}
