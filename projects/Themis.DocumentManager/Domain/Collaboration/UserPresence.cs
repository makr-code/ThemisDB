/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UserPresence.cs                                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     126                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7e8faa1d8  2025-12-10  Implement Phase 2 Sprint 5-6: Collaboration features (Che... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace Themis.DocumentManager.Domain.Collaboration;

/// <summary>
/// Repräsentiert die Präsenz eines Benutzers in einem Dokument (für Real-time Collaboration).
/// </summary>
public class UserPresence
{
    /// <summary>
    /// ID des Benutzers
    /// </summary>
    public string UserId { get; set; } = string.Empty;

    /// <summary>
    /// Benutzername
    /// </summary>
    public string UserName { get; set; } = string.Empty;

    /// <summary>
    /// ID des Dokuments, in dem der Benutzer aktiv ist
    /// </summary>
    public string DocumentId { get; set; } = string.Empty;

    /// <summary>
    /// Status des Benutzers
    /// </summary>
    public PresenceStatus Status { get; set; } = PresenceStatus.Viewing;

    /// <summary>
    /// Zeitpunkt der letzten Aktivität
    /// </summary>
    public DateTime LastActivityAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Zeitpunkt, an dem der Benutzer das Dokument geöffnet hat
    /// </summary>
    public DateTime JoinedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Cursor-Position im Dokument (optional)
    /// </summary>
    public DocumentPosition? CursorPosition { get; set; }

    /// <summary>
    /// Ausgewählter Bereich im Dokument (optional)
    /// </summary>
    public DocumentPosition? Selection { get; set; }

    /// <summary>
    /// Farbe für die Anzeige des Benutzers (Cursor, Markierungen)
    /// </summary>
    public string? Color { get; set; }

    /// <summary>
    /// Verbindungs-ID (SignalR Connection ID)
    /// </summary>
    public string? ConnectionId { get; set; }

    /// <summary>
    /// Ist der Benutzer noch aktiv? (basierend auf LastActivityAt)
    /// </summary>
    public bool IsActive(TimeSpan timeout)
    {
        return DateTime.UtcNow - LastActivityAt < timeout;
    }

    /// <summary>
    /// Aktualisiert die letzte Aktivitätszeit
    /// </summary>
    public void UpdateActivity()
    {
        LastActivityAt = DateTime.UtcNow;
    }
}

/// <summary>
/// Status der Benutzer-Präsenz
/// </summary>
public enum PresenceStatus
{
    /// <summary>
    /// Benutzer betrachtet das Dokument
    /// </summary>
    Viewing,

    /// <summary>
    /// Benutzer bearbeitet das Dokument
    /// </summary>
    Editing,

    /// <summary>
    /// Benutzer ist inaktiv/away
    /// </summary>
    Away,

    /// <summary>
    /// Benutzer hat das Dokument verlassen
    /// </summary>
    Left
}
