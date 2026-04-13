/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            HardeningRecord.cs                                 ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 20:54:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     64                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 684f1ae3bf  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Text.Json.Serialization;

namespace Themis.USBAdminTool.Models;

/// <summary>
/// Serialised as <c>.themis_hardening.json</c> on the USB stick root and also
/// stored server-side.  Contains the values that must be fed into
/// <c>USBAdminConfig::expected_volume_hash</c> and
/// <c>USBAdminConfig::expected_usb_serial</c> in the ThemisDB C++ server
/// configuration to activate the three hardening layers.
/// </summary>
public sealed class HardeningRecord
{
    [JsonPropertyName("license_file")]
    public string LicenseFile { get; set; } = "themis_admin.lic";

    /// <summary>
    /// SHA-256 hex digest of the license file bytes at provisioning time.
    /// Feed into <c>USBAdminConfig::expected_volume_hash</c>.
    /// </summary>
    [JsonPropertyName("expected_volume_hash")]
    public string ExpectedVolumeHash { get; set; } = string.Empty;

    /// <summary>
    /// Volume serial number (hex) of the provisioned USB stick.
    /// Feed into <c>USBAdminConfig::expected_usb_serial</c>.
    /// </summary>
    [JsonPropertyName("expected_usb_serial")]
    public string ExpectedUsbSerial { get; set; } = string.Empty;

    /// <summary>UTC timestamp when this record was created.</summary>
    [JsonPropertyName("provisioned_at")]
    public string ProvisionedAt { get; set; } = DateTime.UtcNow.ToString("o");

    /// <summary>Organization field from the license.</summary>
    [JsonPropertyName("organization")]
    public string Organization { get; set; } = string.Empty;

    /// <summary>License key from the license.</summary>
    [JsonPropertyName("license_key")]
    public string LicenseKey { get; set; } = string.Empty;
}
