/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            NotificationModels.cs                              ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-03-02 03:56:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     46                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
