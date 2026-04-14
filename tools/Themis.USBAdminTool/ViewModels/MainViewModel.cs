/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainViewModel.cs                                   ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 19:10:20                                ║
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

using System.Collections.ObjectModel;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Win32;
using Themis.USBAdminTool.Models;
using Themis.USBAdminTool.Services;

namespace Themis.USBAdminTool.ViewModels;

/// <summary>
/// Main view-model powering all five tabs of the USB Admin Tool:
/// <list type="bullet">
///   <item>Laufwerke — list detected USB drives</item>
///   <item>Provisionieren — create and write a new license</item>
///   <item>Verifizieren — check integrity of an existing USB</item>
///   <item>Übertragen — copy a license from one USB to another</item>
///   <item>Reparieren — re-provision a damaged or expired USB</item>
/// </list>
/// </summary>
public sealed partial class MainViewModel : ObservableObject
{
    private readonly UsbDetectionService    _detection;
    private readonly UsbProvisioningService _provisioning;

    // ── Observable properties ─────────────────────────────────────────────────

    [ObservableProperty] private ObservableCollection<UsbDriveInfo> _drives = new();
    [ObservableProperty] private UsbDriveInfo? _selectedDrive;
    [ObservableProperty] private string _statusMessage = "Bereit";
    [ObservableProperty] private bool   _isLoading;

    // ── Provision tab ─────────────────────────────────────────────────────────
    [ObservableProperty] private string _provOrganization  = string.Empty;
    [ObservableProperty] private string _provHardwareId    = string.Empty;
    [ObservableProperty] private DateTime _provExpiryDate  = DateTime.Today.AddYears(1);
    [ObservableProperty] private string _provScopes
        = "admin, config:write, cdc:admin, admin:backup, admin:restore, admin:topology, admin:rebalance";
    [ObservableProperty] private string _provPrivateKeyPath = string.Empty;
    [ObservableProperty] private bool   _provWriteHardening = true;
    [ObservableProperty] private string _provResult          = string.Empty;
    [ObservableProperty] private string _provDrivePath       = string.Empty;

    // ── Verify tab ────────────────────────────────────────────────────────────
    [ObservableProperty] private string  _verifyDrivePath = string.Empty;
    [ObservableProperty] private string  _verifyResult    = string.Empty;
    [ObservableProperty] private bool    _verifyOk;

    // ── Transfer tab ─────────────────────────────────────────────────────────
    [ObservableProperty] private string _transferSource      = string.Empty;
    [ObservableProperty] private string _transferTarget      = string.Empty;
    [ObservableProperty] private string _transferKeyPath     = string.Empty;
    [ObservableProperty] private string _transferResult      = string.Empty;

    // ── Repair tab ───────────────────────────────────────────────────────────
    [ObservableProperty] private string _repairDrivePath     = string.Empty;
    [ObservableProperty] private string _repairKeyPath       = string.Empty;
    [ObservableProperty] private string _repairResult        = string.Empty;
    [ObservableProperty] private DateTime _repairExpiryDate  = DateTime.Today.AddYears(1);

    // ─────────────────────────────────────────────────────────────────────────

    public MainViewModel(UsbDetectionService detection, UsbProvisioningService provisioning)
    {
        _detection    = detection;
        _provisioning = provisioning;
    }

    // ── Commands: Drives tab ──────────────────────────────────────────────────

    [RelayCommand]
    private async Task RefreshDrivesAsync()
    {
        IsLoading = true;
        StatusMessage = "Laufwerke werden erkannt…";
        try
        {
            var found = await _detection.GetRemovableDrivesAsync();
            Drives = new ObservableCollection<UsbDriveInfo>(found);
            StatusMessage = Drives.Count > 0
                ? $"{Drives.Count} Wechseldatenträger gefunden"
                : "Keine Wechseldatenträger gefunden";

            // Auto-fill drive path inputs if a USB with a license was found.
            var withLicense = Drives.FirstOrDefault(d => d.License is not null);
            if (withLicense is not null)
            {
                var r = withLicense.RootPath;
                VerifyDrivePath  = VerifyDrivePath.Length  == 0 ? r : VerifyDrivePath;
                RepairDrivePath  = RepairDrivePath.Length  == 0 ? r : RepairDrivePath;
                TransferSource   = TransferSource.Length   == 0 ? r : TransferSource;
                ProvDrivePath    = ProvDrivePath.Length    == 0 ? r : ProvDrivePath;
            }
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler: {ex.Message}";
        }
        finally
        {
            IsLoading = false;
        }
    }

    // ── Commands: Provision tab ───────────────────────────────────────────────

    [RelayCommand]
    private async Task ProvisionAsync()
    {
        if (string.IsNullOrWhiteSpace(ProvDrivePath) ||
            string.IsNullOrWhiteSpace(ProvOrganization) ||
            string.IsNullOrWhiteSpace(ProvHardwareId))
        {
            ProvResult = "❌  Bitte alle Pflichtfelder ausfüllen (Laufwerk, Organisation, Hardware-ID).";
            return;
        }

        IsLoading = true;
        StatusMessage = "Provisionierung läuft…";
        try
        {
            var config = new ProvisionConfig
            {
                DrivePath            = ProvDrivePath,
                Organization         = ProvOrganization,
                HardwareId           = ProvHardwareId,
                ExpiryDate           = ProvExpiryDate,
                AdminScopes          = ProvScopes
                                           .Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
                                           .ToList(),
                PrivateKeyPath       = ProvPrivateKeyPath,
                WriteHardeningRecord = ProvWriteHardening,
            };

            var record = await _provisioning.ProvisionAsync(config);

            var jsonOpts = new JsonSerializerOptions { WriteIndented = true };
            ProvResult = $"✅  USB erfolgreich provisioniert.\n\n" +
                         $"Lizenzschlüssel:  {record.LicenseKey}\n" +
                         $"Volume-Hash:      {record.ExpectedVolumeHash}\n" +
                         $"Seriennummer:     {record.ExpectedUsbSerial}\n\n" +
                         $"Tragen Sie folgende Werte in die ThemisDB-Serverkonfiguration ein:\n" +
                         $"  expected_volume_hash = \"{record.ExpectedVolumeHash}\"\n" +
                         $"  expected_usb_serial  = \"{record.ExpectedUsbSerial}\"\n\n" +
                         $"Hardening-Record (auch auf dem Stick gespeichert):\n" +
                         JsonSerializer.Serialize(record, jsonOpts);

            StatusMessage = "Provisionierung abgeschlossen";
            await RefreshDrivesAsync();
        }
        catch (Exception ex)
        {
            ProvResult    = $"❌  Fehler: {ex.Message}";
            StatusMessage = "Provisionierung fehlgeschlagen";
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private void BrowseProvPrivateKey()
    {
        var path = BrowsePemFile("RSA-Privat-Schlüssel auswählen");
        if (path is not null) ProvPrivateKeyPath = path;
    }

    // ── Commands: Verify tab ──────────────────────────────────────────────────

    [RelayCommand]
    private async Task VerifyAsync()
    {
        if (string.IsNullOrWhiteSpace(VerifyDrivePath))
        {
            VerifyResult = "❌  Bitte einen Laufwerkspfad eingeben.";
            return;
        }

        IsLoading = true;
        StatusMessage = "Verifizierung läuft…";
        try
        {
            var result = await _provisioning.VerifyAsync(VerifyDrivePath);
            VerifyOk = result.OverallOk;

            var sb = new System.Text.StringBuilder();
            sb.AppendLine(result.OverallOk
                ? "✅  Alle Integritätsprüfungen bestanden."
                : "❌  Fehler bei der Integritätsprüfung.");
            sb.AppendLine();
            sb.AppendLine($"Lizenzdatei gefunden:  {(result.LicenseFileFound  ? "✓" : "✗")}");
            sb.AppendLine($"JSON gültig:           {(result.LicenseJsonValid  ? "✓" : "✗")}");
            sb.AppendLine($"Nicht abgelaufen:      {(result.LicenseNotExpired ? "✓" : "✗")}");

            if (result.VolumeHashOk is bool hashOk)
            {
                sb.AppendLine($"Volume-Hash OK:        {(hashOk ? "✓" : "✗")}");
                sb.AppendLine($"  Aktuell:  {result.CurrentHash}");
                sb.AppendLine($"  Gepinnt:  {result.PinnedHash}");
            }
            else
            {
                sb.AppendLine("Volume-Hash:           (kein Hardening-Record – übersprungen)");
            }

            if (result.SerialOk is bool serialOk)
            {
                sb.AppendLine($"Seriennummer OK:       {(serialOk ? "✓" : "✗")}");
                sb.AppendLine($"  Aktuell:  {result.CurrentSerial}");
                sb.AppendLine($"  Gepinnt:  {result.PinnedSerial}");
            }
            else
            {
                sb.AppendLine("Seriennummer:          (kein Hardening-Record – übersprungen)");
            }

            sb.AppendLine();
            sb.AppendLine($"Zusammenfassung: {result.Summary}");

            VerifyResult  = sb.ToString();
            StatusMessage = result.OverallOk ? "Verifikation erfolgreich" : "Verifikation fehlgeschlagen";
        }
        catch (Exception ex)
        {
            VerifyResult  = $"❌  Fehler: {ex.Message}";
            StatusMessage = "Verifikation fehlgeschlagen";
        }
        finally
        {
            IsLoading = false;
        }
    }

    // ── Commands: Transfer tab ────────────────────────────────────────────────

    [RelayCommand]
    private async Task TransferAsync()
    {
        if (string.IsNullOrWhiteSpace(TransferSource) || string.IsNullOrWhiteSpace(TransferTarget))
        {
            TransferResult = "❌  Bitte Quell- und Ziel-Laufwerk angeben.";
            return;
        }

        if (string.Equals(
                TransferSource.TrimEnd('\\', '/'),
                TransferTarget.TrimEnd('\\', '/'),
                StringComparison.OrdinalIgnoreCase))
        {
            TransferResult = "❌  Quelle und Ziel dürfen nicht identisch sein.";
            return;
        }

        IsLoading = true;
        StatusMessage = "Übertragung läuft…";
        try
        {
            var record = await _provisioning.TransferAsync(
                TransferSource, TransferTarget, TransferKeyPath);

            TransferResult =
                $"✅  Lizenz erfolgreich auf Ziel-Stick übertragen.\n\n" +
                $"Neuer Lizenzschlüssel:  {record.LicenseKey}\n" +
                $"Neuer Volume-Hash:      {record.ExpectedVolumeHash}\n" +
                $"Neue Seriennummer:      {record.ExpectedUsbSerial}\n\n" +
                $"Aktualisieren Sie die ThemisDB-Serverkonfiguration für den neuen Stick:\n" +
                $"  expected_volume_hash = \"{record.ExpectedVolumeHash}\"\n" +
                $"  expected_usb_serial  = \"{record.ExpectedUsbSerial}\"";

            StatusMessage = "Übertragung abgeschlossen";
            await RefreshDrivesAsync();
        }
        catch (Exception ex)
        {
            TransferResult = $"❌  Fehler: {ex.Message}";
            StatusMessage  = "Übertragung fehlgeschlagen";
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private void BrowseTransferKey() => TransferKeyPath = BrowsePemFile("Privat-Schlüssel für Übertragung") ?? TransferKeyPath;

    // ── Commands: Repair tab ──────────────────────────────────────────────────

    [RelayCommand]
    private async Task RepairAsync()
    {
        if (string.IsNullOrWhiteSpace(RepairDrivePath))
        {
            RepairResult = "❌  Bitte einen Laufwerkspfad eingeben.";
            return;
        }

        IsLoading = true;
        StatusMessage = "Reparatur läuft…";
        try
        {
            var config = new ProvisionConfig
            {
                DrivePath            = RepairDrivePath,
                PrivateKeyPath       = RepairKeyPath,
                ExpiryDate           = RepairExpiryDate,
                WriteHardeningRecord = true,
                // Organization, HardwareId, AdminScopes are loaded from the
                // existing license by RepairAsync if not overridden here.
            };

            var record = await _provisioning.RepairAsync(config);

            RepairResult =
                $"✅  USB-Stick erfolgreich repariert und neu provisioniert.\n\n" +
                $"Neuer Lizenzschlüssel:  {record.LicenseKey}\n" +
                $"Neuer Volume-Hash:      {record.ExpectedVolumeHash}\n" +
                $"Seriennummer:          {record.ExpectedUsbSerial}\n\n" +
                $"Aktualisieren Sie den Hardening-Wert in der Serverkonfiguration:\n" +
                $"  expected_volume_hash = \"{record.ExpectedVolumeHash}\"";

            StatusMessage = "Reparatur abgeschlossen";
            await RefreshDrivesAsync();
        }
        catch (Exception ex)
        {
            RepairResult  = $"❌  Fehler: {ex.Message}";
            StatusMessage = "Reparatur fehlgeschlagen";
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    private void BrowseRepairKey() => RepairKeyPath = BrowsePemFile("Privat-Schlüssel für Reparatur") ?? RepairKeyPath;

    // ── Helpers ───────────────────────────────────────────────────────────────

    private static string? BrowsePemFile(string title)
    {
        var dlg = new OpenFileDialog
        {
            Title  = title,
            Filter = "PEM-Dateien (*.pem)|*.pem|Alle Dateien (*.*)|*.*",
        };
        return dlg.ShowDialog() == true ? dlg.FileName : null;
    }
}
