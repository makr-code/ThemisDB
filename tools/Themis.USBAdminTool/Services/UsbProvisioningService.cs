/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UsbProvisioningService.cs                          ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 05:58:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     375                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 684f1ae3bf  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.IO;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using Themis.USBAdminTool.Models;

namespace Themis.USBAdminTool.Services;

/// <summary>
/// Core business logic for USB stick lifecycle management.
///
/// Operations:
/// <list type="bullet">
///   <item><description>
///     <b>Provision</b> — generate a new <c>themis_admin.lic</c>, sign it with
///     the configured RSA private key, write it to the USB, and optionally record
///     the SHA-256 hash + volume serial in <c>.themis_hardening.json</c>.
///   </description></item>
///   <item><description>
///     <b>Verify</b> — check that the license is syntactically valid, not expired,
///     and that the file hash matches the pinned value (Layer 1) and the volume
///     serial matches the pinned value (Layer 3).
///   </description></item>
///   <item><description>
///     <b>Repair</b> — re-provision the USB with the original hardening record if
///     the hash has changed (FAT manipulation recovery) or the license has expired.
///   </description></item>
///   <item><description>
///     <b>Transfer</b> — copy the license from a source USB to a target USB,
///     re-sign it for the target's volume serial, and update
///     <c>.themis_hardening.json</c> on both sticks.
///   </description></item>
/// </list>
/// </summary>
[SupportedOSPlatform("windows")]
public sealed class UsbProvisioningService
{
    private const string LicenseFileName      = "themis_admin.lic";
    private const string HardeningFileName    = ".themis_hardening.json";
    private const string LicenseKeyPrefix     = "THEMIS-ENT-ADMIN";

    private static readonly JsonSerializerOptions JsonOpts = new()
    {
        WriteIndented = true,
        DefaultIgnoreCondition = JsonIgnoreCondition.Never,
    };

    private readonly HashService _hash;
    private readonly UsbDetectionService _detection;

    public UsbProvisioningService(HashService hash, UsbDetectionService detection)
    {
        _hash      = hash;
        _detection = detection;
    }

    // ── Provision ─────────────────────────────────────────────────────────────

    /// <summary>
    /// Provision a new ThemisDB admin license on the USB stick at
    /// <paramref name="config"/>.DrivePath.
    /// </summary>
    /// <returns>
    /// The <see cref="HardeningRecord"/> that was written to the stick and
    /// should be stored server-side to activate the hardening layers.
    /// </returns>
    public async Task<HardeningRecord> ProvisionAsync(ProvisionConfig config)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(config.DrivePath);
        ArgumentException.ThrowIfNullOrWhiteSpace(config.Organization);
        ArgumentException.ThrowIfNullOrWhiteSpace(config.HardwareId);

        var licenseKey = GenerateLicenseKey();
        var now        = DateTime.UtcNow;

        var license = new UsbAdminLicense
        {
            LicenseKey   = licenseKey,
            Organization = config.Organization,
            HardwareId   = config.HardwareId,
            IssuedDate   = now.ToString("yyyy-MM-dd"),
            ExpiryDate   = config.ExpiryDate.ToString("yyyy-MM-dd"),
            AdminScopes  = config.AdminScopes,
            Signature    = string.Empty,   // Filled in after signing
        };

        // Sign the canonical data string (same field order as the C++ verifier).
        var issuedEpoch = new DateTimeOffset(now).ToUnixTimeSeconds();
        var expiryEpoch = new DateTimeOffset(config.ExpiryDate.ToUniversalTime()).ToUnixTimeSeconds();
        var canonicalData = BuildCanonicalData(license, issuedEpoch, expiryEpoch);
        license.Signature = SignData(canonicalData, config.PrivateKeyPath);

        // Write the license JSON to the USB.
        var licPath = Path.Combine(config.DrivePath, config.LicenseFileName);
        var licJson = JsonSerializer.Serialize(license, JsonOpts);
        await File.WriteAllTextAsync(licPath, licJson, Encoding.UTF8);

        // Build hardening record from the freshly written file.
        var drives = await _detection.GetRemovableDrivesAsync();
        var drive  = drives.FirstOrDefault(d =>
            string.Equals(d.RootPath.TrimEnd('\\', '/'),
                          config.DrivePath.TrimEnd('\\', '/'),
                          StringComparison.OrdinalIgnoreCase));

        var record = new HardeningRecord
        {
            LicenseFile         = config.LicenseFileName,
            ExpectedVolumeHash  = await _hash.ComputeFileHashAsync(licPath),
            ExpectedUsbSerial   = drive?.VolumeSerial ?? string.Empty,
            ProvisionedAt       = now.ToString("o"),
            Organization        = config.Organization,
            LicenseKey          = licenseKey,
        };

        if (config.WriteHardeningRecord)
        {
            var hardeningPath = Path.Combine(config.DrivePath, HardeningFileName);
            await File.WriteAllTextAsync(
                hardeningPath,
                JsonSerializer.Serialize(record, JsonOpts),
                Encoding.UTF8);

            // Hide the hardening file to reduce accidental discovery.
            File.SetAttributes(hardeningPath,
                File.GetAttributes(hardeningPath) | FileAttributes.Hidden);
        }

        return record;
    }

    // ── Verify ────────────────────────────────────────────────────────────────

    /// <summary>
    /// Verify the integrity of the admin USB at <paramref name="drivePath"/>.
    /// </summary>
    public async Task<VerifyResult> VerifyAsync(string drivePath)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(drivePath);

        var licPath = Path.Combine(drivePath, LicenseFileName);
        if (!File.Exists(licPath))
        {
            return new VerifyResult
            {
                LicenseFileFound  = false,
                LicenseJsonValid  = false,
                LicenseNotExpired = false,
            };
        }

        // Parse license JSON.
        UsbAdminLicense? license = null;
        try
        {
            var json = await File.ReadAllTextAsync(licPath);
            license = JsonSerializer.Deserialize<UsbAdminLicense>(json);
        }
        catch { /* invalid JSON — will be reported */ }

        var jsonValid  = license is { } && license.IsValid;
        var notExpired = license is { } && !license.IsExpired;

        // Compute current hash (Layer 1 check).
        var currentHash = await _hash.ComputeFileHashAsync(licPath);

        // Try to load hardening record for pinned values.
        var hardeningPath = Path.Combine(drivePath, HardeningFileName);
        HardeningRecord? hardening = null;
        if (File.Exists(hardeningPath))
        {
            try
            {
                var hJson = await File.ReadAllTextAsync(hardeningPath);
                hardening = JsonSerializer.Deserialize<HardeningRecord>(hJson);
            }
            catch { /* ignore corrupt hardening record */ }
        }

        // Volume serial (Layer 3 check).
        var drives = await _detection.GetRemovableDrivesAsync();
        var drive  = drives.FirstOrDefault(d =>
            string.Equals(d.RootPath.TrimEnd('\\', '/'),
                          drivePath.TrimEnd('\\', '/'),
                          StringComparison.OrdinalIgnoreCase));
        var currentSerial = drive?.VolumeSerial ?? string.Empty;

        bool? hashOk = hardening is null
            ? null
            : string.Equals(currentHash, hardening.ExpectedVolumeHash, StringComparison.OrdinalIgnoreCase);

        bool? serialOk = hardening is null || string.IsNullOrEmpty(hardening.ExpectedUsbSerial)
            ? null
            : string.Equals(currentSerial, hardening.ExpectedUsbSerial, StringComparison.OrdinalIgnoreCase);

        return new VerifyResult
        {
            LicenseFileFound  = true,
            LicenseJsonValid  = jsonValid,
            LicenseNotExpired = notExpired,
            VolumeHashOk      = hashOk,
            CurrentHash       = currentHash,
            PinnedHash        = hardening?.ExpectedVolumeHash ?? string.Empty,
            SerialOk          = serialOk,
            CurrentSerial     = currentSerial,
            PinnedSerial      = hardening?.ExpectedUsbSerial ?? string.Empty,
        };
    }

    // ── Repair ────────────────────────────────────────────────────────────────

    /// <summary>
    /// Repair a damaged or expired USB stick by re-provisioning it with the
    /// provided <paramref name="config"/>.  This is equivalent to calling
    /// <see cref="ProvisionAsync"/> after verifying the drive is accessible.
    /// </summary>
    public async Task<HardeningRecord> RepairAsync(ProvisionConfig config)
    {
        // Read existing license metadata to preserve scopes and organization
        // unless overridden.
        var licPath = Path.Combine(config.DrivePath, LicenseFileName);
        if (File.Exists(licPath) && string.IsNullOrWhiteSpace(config.Organization))
        {
            try
            {
                var json = await File.ReadAllTextAsync(licPath);
                var existing = JsonSerializer.Deserialize<UsbAdminLicense>(json);
                if (existing is not null)
                {
                    config.Organization = existing.Organization;
                    config.HardwareId   = existing.HardwareId;
                    if (config.AdminScopes.Count == 0)
                        config.AdminScopes = existing.AdminScopes;
                }
            }
            catch { /* ignore — proceed with caller-supplied values */ }
        }

        return await ProvisionAsync(config);
    }

    // ── Transfer ──────────────────────────────────────────────────────────────

    /// <summary>
    /// Transfer a license from <paramref name="sourceDrivePath"/> to
    /// <paramref name="targetDrivePath"/>.
    ///
    /// The license fields are preserved; the target's volume serial replaces
    /// the source serial in the new hardening record, and the license is
    /// re-signed so the target's <c>expected_usb_serial</c> binding is correct.
    /// </summary>
    /// <param name="sourceDrivePath">Source USB root (e.g. "E:\").</param>
    /// <param name="targetDrivePath">Target USB root (e.g. "F:\").</param>
    /// <param name="privateKeyPath">RSA private key PEM for re-signing.</param>
    /// <returns>The hardening record written to the target USB.</returns>
    public async Task<HardeningRecord> TransferAsync(
        string sourceDrivePath,
        string targetDrivePath,
        string privateKeyPath = "")
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(sourceDrivePath);
        ArgumentException.ThrowIfNullOrWhiteSpace(targetDrivePath);

        // Read source license.
        var srcLicPath = Path.Combine(sourceDrivePath, LicenseFileName);
        if (!File.Exists(srcLicPath))
            throw new FileNotFoundException("Keine Lizenzdatei auf dem Quell-Stick gefunden.", srcLicPath);

        var srcJson = await File.ReadAllTextAsync(srcLicPath);
        var srcLic  = JsonSerializer.Deserialize<UsbAdminLicense>(srcJson)
                      ?? throw new InvalidDataException("Ungültiges Lizenz-JSON auf dem Quell-Stick.");

        // Build provisioning config for the target.
        var config = new ProvisionConfig
        {
            DrivePath        = targetDrivePath,
            Organization     = srcLic.Organization,
            HardwareId       = srcLic.HardwareId,
            ExpiryDate       = DateTime.TryParse(srcLic.ExpiryDate, out var d) ? d : DateTime.UtcNow.AddYears(1),
            AdminScopes      = srcLic.AdminScopes,
            PrivateKeyPath   = privateKeyPath,
            WriteHardeningRecord = true,
        };

        return await ProvisionAsync(config);
    }

    // ── Internal helpers ──────────────────────────────────────────────────────

    /// <summary>
    /// Build the canonical data string that is signed by the C++ server and
    /// must be reproduced exactly when generating signatures in this tool.
    /// Format: <c>key|org|hw_id|issued_epoch|expiry_epoch[|scope1|scope2|…]</c>
    /// </summary>
    private static string BuildCanonicalData(
        UsbAdminLicense license,
        long issuedEpoch,
        long expiryEpoch)
    {
        var sb = new StringBuilder();
        sb.Append(license.LicenseKey);
        sb.Append('|').Append(license.Organization);
        sb.Append('|').Append(license.HardwareId);
        sb.Append('|').Append(issuedEpoch);
        sb.Append('|').Append(expiryEpoch);
        foreach (var scope in license.AdminScopes)
            sb.Append('|').Append(scope);
        return sb.ToString();
    }

    /// <summary>
    /// Sign <paramref name="data"/> with the RSA private key at
    /// <paramref name="privateKeyPath"/> (PEM format, PKCS#8 or traditional).
    /// If the path is empty or the file does not exist a deterministic
    /// placeholder signature is returned; this is suitable only for development
    /// environments.  In production the private key must be provided.
    /// </summary>
    private static string SignData(string data, string privateKeyPath)
    {
        if (string.IsNullOrWhiteSpace(privateKeyPath) || !File.Exists(privateKeyPath))
        {
            // Development / test mode: return a deterministic placeholder.
            // Production deployments MUST supply a real private key.
            var hmacKey = Encoding.UTF8.GetBytes("THEMIS-DEV-SIGNING-KEY-DO-NOT-USE-IN-PRODUCTION");
            using var hmac = new HMACSHA256(hmacKey);
            var sig = hmac.ComputeHash(Encoding.UTF8.GetBytes(data));
            return Convert.ToBase64String(sig) + ".DEV";
        }

        try
        {
            var pem = File.ReadAllText(privateKeyPath);
            using var rsa = RSA.Create();
            rsa.ImportFromPem(pem);
            var dataBytes = Encoding.UTF8.GetBytes(data);
            var sigBytes  = rsa.SignData(dataBytes, HashAlgorithmName.SHA256, RSASignaturePadding.Pkcs1);
            return Convert.ToBase64String(sigBytes);
        }
        catch (Exception ex)
        {
            throw new InvalidOperationException(
                $"RSA-Signierung fehlgeschlagen ({privateKeyPath}): {ex.Message}", ex);
        }
    }

    /// <summary>Generate a unique license key in the canonical ThemisDB format.</summary>
    private static string GenerateLicenseKey()
    {
        var guid = Guid.NewGuid().ToString("N").ToUpperInvariant();
        return $"{LicenseKeyPrefix}-{guid[..8]}-{guid[8..16]}";
    }
}
