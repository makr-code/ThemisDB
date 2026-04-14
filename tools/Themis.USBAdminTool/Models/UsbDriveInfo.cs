/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UsbDriveInfo.cs                                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 11:54:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     85                                             ║
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
/// Runtime information about a removable USB drive detected on the system.
/// </summary>
public sealed class UsbDriveInfo
{
    /// <summary>Drive root path, e.g. "E:\".</summary>
    public string RootPath { get; init; } = string.Empty;

    /// <summary>Volume label reported by the OS.</summary>
    public string VolumeLabel { get; init; } = string.Empty;

    /// <summary>
    /// Hex-encoded 32-bit volume serial number from <c>GetVolumeInformation</c>
    /// (e.g. "A1B2C3D4").  This is the value compared against
    /// <c>USBAdminConfig::expected_usb_serial</c> in the C++ authenticator on
    /// Windows.
    /// </summary>
    public string VolumeSerial { get; init; } = string.Empty;

    /// <summary>
    /// Hardware serial number from WMI <c>Win32_DiskDrive.SerialNumber</c>, if
    /// available (requires elevated privileges on some systems).
    /// </summary>
    public string HardwareSerial { get; init; } = string.Empty;

    /// <summary>Total capacity in bytes.</summary>
    public long TotalBytes { get; init; }

    /// <summary>Available free space in bytes.</summary>
    public long FreeBytes { get; init; }

    /// <summary>Filesystem type, e.g. "FAT32", "exFAT".</summary>
    public string FileSystem { get; init; } = string.Empty;

    /// <summary>
    /// Path to the license file if found on this drive.
    /// Null when no <c>themis_admin.lic</c> is present.
    /// </summary>
    public string? LicenseFilePath { get; set; }

    /// <summary>Parsed license from <see cref="LicenseFilePath"/>, or null.</summary>
    public UsbAdminLicense? License { get; set; }

    /// <summary>SHA-256 hex digest of the license file at last scan.</summary>
    public string? LicenseFileHash { get; set; }

    // ── Display helpers ───────────────────────────────────────────────────────

    public string DisplayName =>
        string.IsNullOrWhiteSpace(VolumeLabel)
            ? RootPath
            : $"{VolumeLabel} ({RootPath})";

    public string CapacityDisplay =>
        TotalBytes > 0
            ? $"{FreeBytes / 1_048_576:N0} MB frei von {TotalBytes / 1_073_741_824.0:F1} GB"
            : "Unbekannt";

    public string LicenseStatus =>
        License is null ? "Keine Lizenzdatei" : License.StatusLabel;
}
