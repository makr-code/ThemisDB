/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ProcessLinkingService.cs                           ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     334                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service zur Verknüpfung von vorgefertigten Prozessen mit Entitäten
/// (Dokumenten, Vorgängen, Akten, Dateien, Ablagen)
/// </summary>
public interface IProcessLinkingService
{
    Task<ProcessLink> LinkProcessAsync(ProcessLinkRequest request, CancellationToken cancellationToken = default);
    Task<List<ProcessLink>> GetLinkedProcessesAsync(string entityId, CancellationToken cancellationToken = default);
    Task<bool> UnlinkProcessAsync(string linkId, CancellationToken cancellationToken = default);
    Task<List<ProcessTemplate>> GetAllProcessTemplatesAsync(CancellationToken cancellationToken = default);
    Task<List<ProcessTemplate>> GetAvailableProcessTemplatesAsync(EntityType entityType, CancellationToken cancellationToken = default);
    Task<ProcessTemplate> CreateProcessTemplateAsync(ProcessTemplate template, CancellationToken cancellationToken = default);
}

public class ProcessLinkingService : IProcessLinkingService
{
    private readonly List<ProcessTemplate> _processTemplates;
    private readonly List<ProcessLink> _processLinks;

    public ProcessLinkingService()
    {
        _processTemplates = InitializeDefaultTemplates();
        _processLinks = new List<ProcessLink>();
    }

    public async Task<ProcessLink> LinkProcessAsync(ProcessLinkRequest request, CancellationToken cancellationToken = default)
    {
        var template = _processTemplates.FirstOrDefault(t => t.Id == request.ProcessTemplateId);
        if (template == null)
            throw new ArgumentException($"Prozessvorlage nicht gefunden: {request.ProcessTemplateId}");

        var processLink = new ProcessLink
        {
            Id = Guid.NewGuid().ToString(),
            EntityId = request.EntityId,
            EntityType = request.EntityType,
            ProcessTemplateId = request.ProcessTemplateId,
            ProcessTemplateName = template.Name,
            LinkedAt = DateTime.UtcNow,
            LinkedBy = request.LinkedBy,
            Status = ProcessLinkStatus.Active,
            ExecutionCount = 0
        };

        _processLinks.Add(processLink);
        await Task.CompletedTask;
        return processLink;
    }

    public async Task<List<ProcessLink>> GetLinkedProcessesAsync(string entityId, CancellationToken cancellationToken = default)
    {
        var links = _processLinks.Where(l => l.EntityId == entityId && l.Status == ProcessLinkStatus.Active).ToList();
        await Task.CompletedTask;
        return links;
    }

    public async Task<bool> UnlinkProcessAsync(string linkId, CancellationToken cancellationToken = default)
    {
        var link = _processLinks.FirstOrDefault(l => l.Id == linkId);
        if (link != null)
        {
            link.Status = ProcessLinkStatus.Inactive;
            await Task.CompletedTask;
            return true;
        }
        return false;
    }

    public async Task<List<ProcessTemplate>> GetAllProcessTemplatesAsync(CancellationToken cancellationToken = default)
    {
        await Task.CompletedTask;
        return _processTemplates.ToList();
    }

    public async Task<List<ProcessTemplate>> GetAvailableProcessTemplatesAsync(
        EntityType entityType,
        CancellationToken cancellationToken = default
    )
    {
        var templates = _processTemplates
            .Where(t => t.ApplicableEntityTypes.Contains(entityType))
            .ToList();
        await Task.CompletedTask;
        return templates;
    }

    public async Task<ProcessTemplate> CreateProcessTemplateAsync(ProcessTemplate template, CancellationToken cancellationToken = default)
    {
        template.Id = Guid.NewGuid().ToString();
        template.CreatedAt = DateTime.UtcNow;
        _processTemplates.Add(template);
        await Task.CompletedTask;
        return template;
    }

    private static List<ProcessTemplate> InitializeDefaultTemplates()
    {
        return new List<ProcessTemplate>
        {
            new()
            {
                Id = "proc-001",
                Name = "Vier-Augen-Prinzip",
                Description = "Genehmigung durch zwei autorisierte Personen erforderlich",
                Category = "Approval",
                ApplicableEntityTypes = new[] { EntityType.Dokument, EntityType.Vorgang },
                Steps = new List<ProcessStep>
                {
                    new() { Order = 1, Name = "Zur Genehmigung einreichen", Type = ProcessStepType.Submit },
                    new() { Order = 2, Name = "Erste Genehmigung", Type = ProcessStepType.Approval },
                    new() { Order = 3, Name = "Zweite Genehmigung", Type = ProcessStepType.Approval },
                    new() { Order = 4, Name = "Abgeschlossen", Type = ProcessStepType.Complete }
                },
                RequiredRoles = new[] { "Admin", "Manager" }
            },
            new()
            {
                Id = "proc-002",
                Name = "Versionskontrolle",
                Description = "Automatische Versionsverwaltung und Revisions-Tracking",
                Category = "Document Management",
                ApplicableEntityTypes = new[] { EntityType.Dokument, EntityType.Datei },
                Steps = new List<ProcessStep>
                {
                    new() { Order = 1, Name = "Neue Version erstellen", Type = ProcessStepType.Create },
                    new() { Order = 2, Name = "Revision dokumentieren", Type = ProcessStepType.Notify },
                    new() { Order = 3, Name = "Historie aktualisieren", Type = ProcessStepType.Update }
                },
                RequiredRoles = new[] { "Admin", "Editor" }
            },
            new()
            {
                Id = "proc-003",
                Name = "Datenschutz-Klassifizierung",
                Description = "Automatische Einstufung nach DSGVO-Anforderungen",
                Category = "Compliance",
                ApplicableEntityTypes = new[] { EntityType.Datei, EntityType.Dokument, EntityType.Akte },
                Steps = new List<ProcessStep>
                {
                    new() { Order = 1, Name = "Datenschutz-Analyse", Type = ProcessStepType.Analyze },
                    new() { Order = 2, Name = "Klassifizierung festlegen", Type = ProcessStepType.Update },
                    new() { Order = 3, Name = "Zugriffsrechte anpassen", Type = ProcessStepType.Configure }
                },
                RequiredRoles = new[] { "Admin", "Manager" }
            },
            new()
            {
                Id = "proc-004",
                Name = "Vorgangsbearbeitung",
                Description = "Standard-Workflow für Vorgänge: Neu → In Bearbeitung → Abgeschlossen",
                Category = "Workflow",
                ApplicableEntityTypes = new[] { EntityType.Vorgang },
                Steps = new List<ProcessStep>
                {
                    new() { Order = 1, Name = "Vorgang zuweisen", Type = ProcessStepType.Assign },
                    new() { Order = 2, Name = "In Bearbeitung", Type = ProcessStepType.Update },
                    new() { Order = 3, Name = "Vorgang überprüfen", Type = ProcessStepType.Review },
                    new() { Order = 4, Name = "Abgeschlossen", Type = ProcessStepType.Complete }
                },
                RequiredRoles = new[] { "Admin", "Manager", "Editor" }
            },
            new()
            {
                Id = "proc-005",
                Name = "Aufbewahrungsfrist-Management",
                Description = "Automatische Verwaltung von Aufbewahrungsfristen und Vernichtung",
                Category = "Archive",
                ApplicableEntityTypes = new[] { EntityType.Akte, EntityType.Ablage },
                Steps = new List<ProcessStep>
                {
                    new() { Order = 1, Name = "Aufbewahrungsfrist berechnen", Type = ProcessStepType.Analyze },
                    new() { Order = 2, Name = "Ablauffrist überwachen", Type = ProcessStepType.Monitor },
                    new() { Order = 3, Name = "Vernichtung genehmigen", Type = ProcessStepType.Approval },
                    new() { Order = 4, Name = "Vernichtung durchführen", Type = ProcessStepType.Execute }
                },
                RequiredRoles = new[] { "Admin" }
            },
            new()
            {
                Id = "proc-006",
                Name = "Vertraulichkeits-Markierung",
                Description = "Markierung und Verfolgung von vertraulichen Dokumenten",
                Category = "Security",
                ApplicableEntityTypes = new[] { EntityType.Dokument, EntityType.Akte },
                Steps = new List<ProcessStep>
                {
                    new() { Order = 1, Name = "Vertraulichkeitsstufe festlegen", Type = ProcessStepType.Configure },
                    new() { Order = 2, Name = "Zugriffslogging aktivieren", Type = ProcessStepType.Configure },
                    new() { Order = 3, Name = "Benutzer benachrichtigen", Type = ProcessStepType.Notify }
                },
                RequiredRoles = new[] { "Admin", "Manager" }
            },
            new()
            {
                Id = "proc-007",
                Name = "Massenimport-Verarbeitung",
                Description = "Workflow für Massenimporte mit Validierung und Fehlerbehandlung",
                Category = "Data Import",
                ApplicableEntityTypes = new[] { EntityType.Datei, EntityType.Dokument },
                Steps = new List<ProcessStep>
                {
                    new() { Order = 1, Name = "Import-Datei validieren", Type = ProcessStepType.Validate },
                    new() { Order = 2, Name = "Daten parsen", Type = ProcessStepType.Execute },
                    new() { Order = 3, Name = "Metadaten zuweisen", Type = ProcessStepType.Update },
                    new() { Order = 4, Name = "Abgespeichert", Type = ProcessStepType.Complete }
                },
                RequiredRoles = new[] { "Admin" }
            },
            new()
            {
                Id = "proc-008",
                Name = "Export & Archivierung",
                Description = "Strukturierter Export und Archivierung mit Integritätsprüfung",
                Category = "Archive",
                ApplicableEntityTypes = new[] { EntityType.Akte, EntityType.Ablage },
                Steps = new List<ProcessStep>
                {
                    new() { Order = 1, Name = "Exportziel konfigurieren", Type = ProcessStepType.Configure },
                    new() { Order = 2, Name = "Daten exportieren", Type = ProcessStepType.Execute },
                    new() { Order = 3, Name = "Integritätsprüfung (SHA-256)", Type = ProcessStepType.Validate },
                    new() { Order = 4, Name = "Archiv-Metadaten aktualisieren", Type = ProcessStepType.Update }
                },
                RequiredRoles = new[] { "Admin", "Manager" }
            }
        };
    }
}

#region DTOs

public class ProcessLinkRequest
{
    public string EntityId { get; set; } = string.Empty;
    public EntityType EntityType { get; set; }
    public string ProcessTemplateId { get; set; } = string.Empty;
    public string LinkedBy { get; set; } = string.Empty;
}

public class ProcessLink
{
    public string Id { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public EntityType EntityType { get; set; }
    public string ProcessTemplateId { get; set; } = string.Empty;
    public string ProcessTemplateName { get; set; } = string.Empty;
    public DateTime LinkedAt { get; set; }
    public string LinkedBy { get; set; } = string.Empty;
    public ProcessLinkStatus Status { get; set; }
    public int ExecutionCount { get; set; }
    public DateTime? LastExecuted { get; set; }
}

public class ProcessTemplate
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public string Category { get; set; } = string.Empty;
    public EntityType[] ApplicableEntityTypes { get; set; } = Array.Empty<EntityType>();
    public List<ProcessStep> Steps { get; set; } = new();
    public string[] RequiredRoles { get; set; } = Array.Empty<string>();
    public DateTime CreatedAt { get; set; }
}

public class ProcessStep
{
    public int Order { get; set; }
    public string Name { get; set; } = string.Empty;
    public ProcessStepType Type { get; set; }
    public string? Description { get; set; }
    public int? TimeoutMinutes { get; set; }
}

public enum ProcessLinkStatus
{
    Active,
    Inactive,
    Paused,
    Completed
}

public enum ProcessStepType
{
    Submit,
    Approval,
    Create,
    Update,
    Delete,
    Notify,
    Analyze,
    Configure,
    Validate,
    Execute,
    Review,
    Assign,
    Monitor,
    Complete
}

#endregion
