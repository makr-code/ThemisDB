/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UsbDetectionService.cs                             ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:49:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     186                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 684f1ae3bf  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.IO;
using System.Management;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text;
using System.Text.Json;
using Themis.USBAdminTool.Models;

namespace Themis.USBAdminTool.Services;

/// <summary>
/// Detects removable USB drives and reads their serial numbers and volume
/// information.  On Windows, uses <c>GetVolumeInformation</c> (P/Invoke) for
/// the volume serial and WMI <c>Win32_DiskDrive</c> for the hardware serial.
/// </summary>
[SupportedOSPlatform("windows")]
public sealed class UsbDetectionService
{
    private const string LicenseFileName = "themis_admin.lic";
    private const string HardeningFileName = ".themis_hardening.json";

    private readonly HashService _hash;

    public UsbDetectionService(HashService hash)
    {
        _hash = hash;
    }

    // ── Public API ────────────────────────────────────────────────────────────

    /// <summary>
    /// Return all removable drives currently attached to the system, enriched
    /// with license file and hardening record information if present.
    /// </summary>
    public async Task<List<UsbDriveInfo>> GetRemovableDrivesAsync()
    {
        var result = new List<UsbDriveInfo>();

        foreach (var di in DriveInfo.GetDrives())
        {
            if (di.DriveType != DriveType.Removable) continue;

            // May throw if drive was just ejected — skip gracefully.
            try
            {
                if (!di.IsReady) continue;
            }
            catch
            {
                continue;
            }

            var root = di.RootDirectory.FullName;
            var volumeSerial = GetVolumeSerial(root);
            var hardwareSerial = GetHardwareSerialForDrive(root);

            var info = new UsbDriveInfo
            {
                RootPath = root,
                VolumeLabel = TrySafe(() => di.VolumeLabel, string.Empty),
                VolumeSerial = volumeSerial,
                HardwareSerial = hardwareSerial,
                TotalBytes = TrySafe(() => di.TotalSize, 0L),
                FreeBytes = TrySafe(() => di.AvailableFreeSpace, 0L),
                FileSystem = TrySafe(() => di.DriveFormat, string.Empty),
            };

            // Try to load the license file if present.
            var licPath = Path.Combine(root, LicenseFileName);
            if (File.Exists(licPath))
            {
                info.LicenseFilePath = licPath;
                info.LicenseFileHash = await _hash.ComputeFileHashAsync(licPath);
                try
                {
                    var json = await File.ReadAllTextAsync(licPath);
                    info.License = JsonSerializer.Deserialize<UsbAdminLicense>(json);
                }
                catch
                {
                    // Malformed JSON — license remains null, shown as invalid.
                }
            }

            result.Add(info);
        }

        return result;
    }

    // ── Windows P/Invoke: GetVolumeInformation ────────────────────────────────

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool GetVolumeInformation(
        string lpRootPathName,
        StringBuilder? lpVolumeNameBuffer,
        int nVolumeNameSize,
        out uint lpVolumeSerialNumber,
        out uint lpMaximumComponentLength,
        out uint lpFileSystemFlags,
        StringBuilder? lpFileSystemNameBuffer,
        int nFileSystemNameSize);

    /// <summary>
    /// Returns the hex-encoded 32-bit volume serial number, e.g. "A1B2C3D4".
    /// This is the value compared against <c>USBAdminConfig::expected_usb_serial</c>
    /// on Windows in the C++ authenticator.
    /// </summary>
    private static string GetVolumeSerial(string rootPath)
    {
        if (GetVolumeInformation(rootPath, null, 0,
            out var serial, out _, out _, null, 0))
        {
            return serial.ToString("X8");
        }
        return string.Empty;
    }

    // ── WMI: Win32_DiskDrive hardware serial ──────────────────────────────────

    /// <summary>
    /// Attempts to read the physical disk serial number via WMI.
    /// Returns empty string on failure or when WMI is unavailable.
    /// </summary>
    private static string GetHardwareSerialForDrive(string rootPath)
    {
        try
        {
            // Map drive letter to physical disk number via Win32_LogicalDiskToPartition
            // then Win32_DiskDrive.  This chain works for most USB drives.
            var driveLetter = rootPath.TrimEnd('\\', '/');
            using var diskQuery = new ManagementObjectSearcher(
                $"ASSOCIATORS OF {{Win32_LogicalDisk.DeviceID='{driveLetter}'}} " +
                "WHERE AssocClass=Win32_LogicalDiskToPartition");

            foreach (ManagementObject partition in diskQuery.Get())
            {
                using var diskQ2 = new ManagementObjectSearcher(
                    $"ASSOCIATORS OF {{{partition.Path.RelativePath}}} " +
                    "WHERE AssocClass=Win32_DiskDriveToDiskPartition");

                foreach (ManagementObject disk in diskQ2.Get())
                {
                    var serial = disk["SerialNumber"]?.ToString()?.Trim();
                    if (!string.IsNullOrEmpty(serial))
                        return serial;
                }
            }
        }
        catch
        {
            // WMI may be unavailable or the query may fail on some systems.
        }
        return string.Empty;
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    private static T TrySafe<T>(Func<T> fn, T fallback)
    {
        try { return fn(); }
        catch { return fallback; }
    }
}
