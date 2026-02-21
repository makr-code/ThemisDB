/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NotificationModels.cs                              ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 60d127110  2025-12-09  feat: Add comprehensive test report for ThemisDB Document... ║
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
