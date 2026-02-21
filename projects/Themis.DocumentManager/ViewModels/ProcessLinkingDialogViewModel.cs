/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ProcessLinkingDialogViewModel.cs                   ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     294                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.ViewModels;

/// <summary>
/// ViewModel für Process Linking Dialog
/// Zeigt verfügbare Prozessvorlagen und ermöglicht Verknüpfung mit Dokumenten/Akten
/// </summary>
public partial class ProcessLinkingDialogViewModel : ObservableObject
{
    private readonly IProcessLinkingService _processLinkingService;
    private readonly IAuditLoggingService _auditLoggingService;
    private readonly IAuthenticationService _authService;

    [ObservableProperty]
    private string entityId = string.Empty;

    [ObservableProperty]
    private string entityType = string.Empty;

    [ObservableProperty]
    private ObservableCollection<ProcessTemplateViewModel> availableTemplates = new();

    [ObservableProperty]
    private ProcessTemplateViewModel? selectedTemplate;

    [ObservableProperty]
    private ObservableCollection<ProcessLinkViewModel> linkedProcesses = new();

    [ObservableProperty]
    private bool isLoading = false;

    [ObservableProperty]
    private string? statusMessage;

    [ObservableProperty]
    private bool showLinkedProcesses = false;

    public event EventHandler? RequestClose;

    public ProcessLinkingDialogViewModel(
        IProcessLinkingService processLinkingService,
        IAuditLoggingService auditLoggingService,
        IAuthenticationService authService)
    {
        _processLinkingService = processLinkingService;
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
            // Lade alle verfügbaren Process Templates
            await LoadAvailableTemplatesAsync();

            // Lade bereits verknüpfte Prozesse
            await LoadLinkedProcessesAsync();

            StatusMessage = "Bereit für Prozess-Verknüpfung";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Laden: {ex.Message}";
            System.Diagnostics.Debug.WriteLine($"[ERROR] ProcessLinkingDialog: {ex}");
        }
        finally
        {
            IsLoading = false;
        }
    }

    private async Task LoadAvailableTemplatesAsync()
    {
        AvailableTemplates.Clear();

        // Hole alle Prozess-Templates vom Service
        var templates = await _processLinkingService.GetAllProcessTemplatesAsync();

        foreach (var template in templates)
        {
            AvailableTemplates.Add(new ProcessTemplateViewModel
            {
                Id = template.Id,
                Name = template.Name,
                Description = template.Description,
                Category = template.Category,
                StepCount = template.Steps?.Count ?? 0,
                ApplicableEntityTypes = string.Join(", ", template.ApplicableEntityTypes ?? Array.Empty<EntityType>())
            });
        }
    }

    private async Task LoadLinkedProcessesAsync()
    {
        LinkedProcesses.Clear();

        try
        {
            var linked = await _processLinkingService.GetLinkedProcessesAsync(EntityId);

            foreach (var link in linked)
            {
                LinkedProcesses.Add(new ProcessLinkViewModel
                {
                    Id = link.Id,
                    ProcessTemplateId = link.ProcessTemplateId,
                    Status = link.Status,
                    CreatedDate = DateTime.Now,
                    ExecutionCount = link.ExecutionCount
                });
            }

            ShowLinkedProcesses = LinkedProcesses.Count > 0;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[ERROR] LoadLinkedProcesses: {ex}");
        }
    }

    [RelayCommand]
    public async Task LinkProcessAsync()
    {
        if (SelectedTemplate == null)
        {
            StatusMessage = "Bitte wählen Sie einen Prozess aus.";
            return;
        }

        IsLoading = true;
        try
        {
            // Erstelle neue Process Link Request
            var request = new ProcessLinkRequest
            {
                EntityId = EntityId,
                EntityType = (EntityType == "Dokument") ? Services.EntityType.Dokument : 
                             (EntityType == "Akte") ? Services.EntityType.Akte :
                             (EntityType == "Vorgang") ? Services.EntityType.Vorgang :
                             (EntityType == "Datei") ? Services.EntityType.Datei :
                             Services.EntityType.Ablage,
                ProcessTemplateId = SelectedTemplate.Id,
                LinkedBy = _authService.CurrentUserId ?? "system"
            };

            // Speichere Link
            var link = await _processLinkingService.LinkProcessAsync(request);

            // Audit Log
            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = _authService.CurrentUserId ?? "system",
                ActionType = "LinkProcess",
                EntityType = EntityType,
                EntityId = EntityId,
                Details = $"Prozess '{SelectedTemplate.Name}' verknüpft",
                Timestamp = DateTime.UtcNow,
                Result = AuditActionResult.Success
            });

            // Neu laden
            await LoadLinkedProcessesAsync();
            StatusMessage = $"Prozess '{SelectedTemplate.Name}' erfolgreich verknüpft!";

            // Nach kurzer Verzögerung Dialog schließen
            await Task.Delay(1500);
            RequestClose?.Invoke(this, EventArgs.Empty);
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler: {ex.Message}";
            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = _authService.CurrentUserId ?? "system",
                ActionType = "LinkProcess",
                EntityType = EntityType,
                EntityId = EntityId,
                Details = $"Fehler beim Verknüpfen: {ex.Message}",
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
    public async Task UnlinkProcessAsync(ProcessLinkViewModel? link)
    {
        if (link == null) return;

        IsLoading = true;
        try
        {
            await _processLinkingService.UnlinkProcessAsync(link.Id);

            // Audit Log
            await _auditLoggingService.LogActionAsync(new AuditLogEntry
            {
                UserId = _authService.CurrentUserId ?? "system",
                ActionType = "UnlinkProcess",
                EntityType = EntityType,
                EntityId = EntityId,
                Details = $"Prozess '{link.ProcessTemplateId}' entfernt",
                Timestamp = DateTime.UtcNow,
                Result = AuditActionResult.Success
            });

            // Neu laden
            await LoadLinkedProcessesAsync();
            StatusMessage = "Prozess-Verknüpfung entfernt";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Entfernen: {ex.Message}";
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    public void CloseDialog()
    {
        RequestClose?.Invoke(this, EventArgs.Empty);
    }
}

/// <summary>
/// View Model für Process Template in der Dialog-Liste
/// </summary>
public class ProcessTemplateViewModel
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string Category { get; set; } = string.Empty;
    public int StepCount { get; set; }
    public string ApplicableEntityTypes { get; set; } = string.Empty;
}

/// <summary>
/// View Model für verknüpfte Prozesse
/// </summary>
public class ProcessLinkViewModel
{
    public string Id { get; set; } = string.Empty;
    public string ProcessTemplateId { get; set; } = string.Empty;
    public ProcessLinkStatus Status { get; set; }
    public DateTime CreatedDate { get; set; }
    public int ExecutionCount { get; set; }
}
