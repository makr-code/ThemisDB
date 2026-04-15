/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            VerifyResult.cs                                    ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 04:33:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     78                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 684f1ae3bf  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.USBAdminTool.Models;

/// <summary>
/// Result returned by <see cref="Services.UsbProvisioningService.VerifyAsync"/>.
/// Each boolean maps to one of the three hardening layers implemented in
/// <c>src/security/usb_volume_hardening.cpp</c>.
/// </summary>
public sealed class VerifyResult
{
    public bool LicenseFileFound { get; init; }
    public bool LicenseJsonValid { get; init; }
    public bool LicenseNotExpired { get; init; }

    /// <summary>
    /// Volume hash check: current SHA-256 matches the pinned value in
    /// <c>.themis_hardening.json</c> (Layer 1).
    /// Null when no pinned hash is available.
    /// </summary>
    public bool? VolumeHashOk { get; init; }

    /// <summary>Current SHA-256 of the license file (for display).</summary>
    public string CurrentHash { get; init; } = string.Empty;

    /// <summary>Pinned hash from hardening record (for comparison display).</summary>
    public string PinnedHash { get; init; } = string.Empty;

    /// <summary>
    /// Serial binding check: current volume serial matches pinned value (Layer 3).
    /// Null when no pinned serial is available.
    /// </summary>
    public bool? SerialOk { get; init; }

    public string CurrentSerial { get; init; } = string.Empty;
    public string PinnedSerial { get; init; } = string.Empty;

    public bool OverallOk =>
        LicenseFileFound &&
        LicenseJsonValid &&
        LicenseNotExpired &&
        (VolumeHashOk ?? true) &&
        (SerialOk ?? true);

    public string Summary
    {
        get
        {
            var issues = new List<string>();
            if (!LicenseFileFound) issues.Add("Keine Lizenzdatei");
            if (!LicenseJsonValid) issues.Add("Ungültiges JSON");
            if (!LicenseNotExpired) issues.Add("Lizenz abgelaufen");
            if (VolumeHashOk == false) issues.Add("Hash-Mismatch (FAT-Manipulation?)");
            if (SerialOk == false) issues.Add("Seriennummer-Mismatch (geklonter Stick?)");
            return issues.Count == 0 ? "Alle Prüfungen bestanden ✓" : string.Join("; ", issues);
        }
    }
}
