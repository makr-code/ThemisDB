/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SignalRConfiguration.cs                            ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     67                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace Themis.DocumentManager.Infrastructure.SignalR;

/// <summary>
/// Konfiguration für SignalR Client.
/// Ermöglicht umgebungsspezifische Einstellungen.
/// </summary>
public class SignalRConfiguration
{
    /// <summary>
    /// Hub URL (z.B. "https://themisdb.local/documenthub")
    /// </summary>
    public string HubUrl { get; set; } = string.Empty;

    /// <summary>
    /// Reconnection Intervalle in Millisekunden.
    /// Default: [0ms, 2s, 10s] - exponential backoff
    /// </summary>
    public TimeSpan[] ReconnectionIntervals { get; set; } = new[]
    {
        TimeSpan.Zero,
        TimeSpan.FromSeconds(2),
        TimeSpan.FromSeconds(10)
    };

    /// <summary>
    /// Timeout für User Presence (Standard: 5 Minuten)
    /// </summary>
    public TimeSpan PresenceTimeout { get; set; } = TimeSpan.FromMinutes(5);

    /// <summary>
    /// Automatisch reconnecten?
    /// </summary>
    public bool AutoReconnect { get; set; } = true;

    /// <summary>
    /// Logging Level (Debug, Information, Warning, Error)
    /// </summary>
    public string LogLevel { get; set; } = "Information";
}
