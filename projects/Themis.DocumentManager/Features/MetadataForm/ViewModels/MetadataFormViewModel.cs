/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MetadataFormViewModel.cs                           ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     216                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Features.MetadataForm.Services;

namespace Themis.DocumentManager.Features.MetadataForm.ViewModels;

/// <summary>
/// ViewModel für Metadaten-Formular-Anzeige und -Bearbeitung
/// </summary>
public partial class MetadataFormViewModel : ObservableObject
{
    private readonly IMetadataFormGeneratorService _formGeneratorService;
    private readonly IAuditLoggingService _auditLoggingService;
    private readonly IAuthenticationService _authService;

    [ObservableProperty]
    private MetadataFormDefinition? formDefinition;

    [ObservableProperty]
    private string entityId = string.Empty;

    [ObservableProperty]
    private string entityType = string.Empty;

    [ObservableProperty]
    private bool isLoading = false;

    [ObservableProperty]
    private string? statusMessage;

    [ObservableProperty]
    private bool hasValidationErrors = false;

    public event EventHandler? FormSaved;
    public event EventHandler? FormClosed;

    public MetadataFormViewModel(
        IMetadataFormGeneratorService formGeneratorService,
        IAuditLoggingService auditLoggingService,
        IAuthenticationService authService)
    {
        _formGeneratorService = formGeneratorService;
        _auditLoggingService = auditLoggingService;
        _authService = authService;
    }

    public async Task InitializeAsync(string entityId, string entityType)
    {
        EntityId = entityId;
        EntityType = entityType;
        IsLoading = true;

        try
        {
            FormDefinition = await _formGeneratorService.LoadFormTemplateAsync(entityType);
            StatusMessage = "Metadaten-Formular geladen";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler: {ex.Message}";
            System.Diagnostics.Debug.WriteLine($"[ERROR] Loading form: {ex}");
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    public async Task SaveFormAsync()
    {
        // Validierung
        if (!ValidateForm())
        {
            StatusMessage = "Bitte füllen Sie alle erforderlichen Felder aus.";
            HasValidationErrors = true;
            return;
        }

        IsLoading = true;
        try
        {
            // Sammle Formular-Daten
            var metadata = CollectFormData();

            // Audit Log
            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = _authService.CurrentUserId ?? "system",
                ActionType = "SaveMetadata",
                EntityType = EntityType,
                EntityId = EntityId,
                Details = $"Metadaten gespeichert für {EntityType}: {EntityId}",
                Timestamp = DateTime.UtcNow,
                Result = AuditActionResult.Success
            });

            StatusMessage = "✓ Metadaten erfolgreich gespeichert";
            FormSaved?.Invoke(this, EventArgs.Empty);
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Speichern: {ex.Message}";
            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = _authService.CurrentUserId ?? "system",
                ActionType = "SaveMetadata",
                EntityType = EntityType,
                EntityId = EntityId,
                Details = $"Fehler: {ex.Message}",
                Timestamp = DateTime.UtcNow,
                Result = AuditActionResult.Failed
            });
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    public void ResetForm()
    {
        if (FormDefinition != null)
        {
            foreach (var section in FormDefinition.Sections)
            {
                foreach (var field in section.Fields)
                {
                    field.Value = field.DefaultValue;
                }
            }
            StatusMessage = "Formular zurückgesetzt";
        }
    }

    [RelayCommand]
    public void CloseForm()
    {
        FormClosed?.Invoke(this, EventArgs.Empty);
    }

    private bool ValidateForm()
    {
        if (FormDefinition == null)
            return false;

        var allFieldsValid = true;

        foreach (var section in FormDefinition.Sections)
        {
            foreach (var field in section.Fields)
            {
                if (!field.Validate())
                {
                    allFieldsValid = false;
                }
            }
        }

        return allFieldsValid;
    }

    private Dictionary<string, string?> CollectFormData()
    {
        var data = new Dictionary<string, string?>();

        if (FormDefinition == null)
            return data;

        foreach (var section in FormDefinition.Sections)
        {
            foreach (var field in section.Fields)
            {
                data[field.Name] = field.Value ?? field.DefaultValue;
            }
        }

        return data;
    }
}


