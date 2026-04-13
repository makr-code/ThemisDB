/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ProvisionConfig.cs                                 ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:49:28                                ║
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
/// Parameters collected in the "Provisionieren" tab before writing a new
/// license to the USB stick.
/// </summary>
public sealed class ProvisionConfig
{
    /// <summary>Root path of the target USB drive (e.g. "E:\").</summary>
    public string DrivePath { get; set; } = string.Empty;

    /// <summary>Organization name embedded in the license.</summary>
    public string Organization { get; set; } = string.Empty;

    /// <summary>
    /// Hardware ID of the server host that will consume this USB.
    /// On Linux: content of <c>/etc/machine-id</c>.
    /// On Windows: registry <c>HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid</c>.
    /// </summary>
    public string HardwareId { get; set; } = string.Empty;

    /// <summary>Expiry date for the generated license.</summary>
    public DateTime ExpiryDate { get; set; } = DateTime.UtcNow.AddYears(1);

    /// <summary>
    /// Admin scopes to embed in the license.
    /// Defaults mirror <c>USBAdminConfig::usb_protected_scopes</c>.
    /// </summary>
    public List<string> AdminScopes { get; set; } = new()
    {
        "admin",
        "config:write",
        "cdc:admin",
        "admin:backup",
        "admin:restore",
        "admin:topology",
        "admin:rebalance"
    };

    /// <summary>
    /// Path to the RSA private key PEM file used to sign the license.
    /// If empty, a self-signed test key is generated (not suitable for
    /// production).
    /// </summary>
    public string PrivateKeyPath { get; set; } = string.Empty;

    /// <summary>
    /// Name of the license file written to the USB root.
    /// Must match <c>USBAdminConfig::license_file</c>.
    /// </summary>
    public string LicenseFileName { get; set; } = "themis_admin.lic";

    // ── Hardening record (written to <drive>\.themis_hardening.json) ─────────

    /// <summary>
    /// When true, the tool records the SHA-256 hash of the written license
    /// file and the USB volume serial in a side-channel hardening config
    /// (<c>.themis_hardening.json</c>) that can be fed back into
    /// <c>USBAdminConfig::expected_volume_hash</c> and
    /// <c>USBAdminConfig::expected_usb_serial</c>.
    /// </summary>
    public bool WriteHardeningRecord { get; set; } = true;
}
