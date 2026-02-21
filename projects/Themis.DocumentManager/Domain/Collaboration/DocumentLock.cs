/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentLock.cs                                    ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;

namespace Themis.DocumentManager.Domain.Collaboration;

/// <summary>
/// Repräsentiert eine Sperre auf einem Dokument für Collaboration-Zwecke.
/// Unterstützt Check-in/Check-out Mechanismen.
/// </summary>
public class DocumentLock
{
    /// <summary>
    /// Eindeutige ID der Sperre
    /// </summary>
    public string Id { get; set; } = Guid.NewGuid().ToString();

    /// <summary>
    /// ID des gesperrten Dokuments
    /// </summary>
    public string DocumentId { get; set; } = string.Empty;

    /// <summary>
    /// ID des Benutzers, der die Sperre hält
    /// </summary>
    public string UserId { get; set; } = string.Empty;

    /// <summary>
    /// Benutzername des Sperrenden (für UI-Anzeige)
    /// </summary>
    public string UserName { get; set; } = string.Empty;

    /// <summary>
    /// Zeitpunkt der Sperre
    /// </summary>
    public DateTime LockedAt { get; set; } = DateTime.UtcNow;

    /// <summary>
    /// Ablaufzeit der Sperre (null = kein Timeout)
    /// </summary>
    public DateTime? ExpiresAt { get; set; }

    /// <summary>
    /// Typ der Sperre (Read/Write)
    /// </summary>
    public LockType Type { get; set; } = LockType.Write;

    /// <summary>
    /// Grund für die Sperre (optional)
    /// </summary>
    public string? Reason { get; set; }

    /// <summary>
    /// Maschinen-/Host-Name des Sperrenden
    /// </summary>
    public string? MachineName { get; set; }

    /// <summary>
    /// Prüft, ob die Sperre abgelaufen ist
    /// </summary>
    public bool IsExpired()
    {
        return ExpiresAt.HasValue && DateTime.UtcNow > ExpiresAt.Value;
    }

    /// <summary>
    /// Prüft, ob die Sperre aktiv ist
    /// </summary>
    public bool IsActive()
    {
        return !IsExpired();
    }
}

/// <summary>
/// Typ der Dokumentensperre
/// </summary>
public enum LockType
{
    /// <summary>
    /// Lesesperre - Andere können lesen, aber nicht ändern
    /// </summary>
    Read,

    /// <summary>
    /// Schreibsperre - Exklusiver Zugriff, andere können weder lesen noch ändern
    /// </summary>
    Write,

    /// <summary>
    /// Optimistische Sperre - Andere können ändern, Konflikte werden beim Check-in gelöst
    /// </summary>
    Optimistic
}
