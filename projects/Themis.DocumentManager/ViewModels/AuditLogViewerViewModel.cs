/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AuditLogViewerViewModel.cs                         ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     179                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.ViewModels;

public partial class AuditLogViewerViewModel : ObservableObject
{
    private readonly IAuditLoggingService _auditService;

    [ObservableProperty]
    private DateTime _from = DateTime.UtcNow.AddDays(-7);

    [ObservableProperty]
    private DateTime _to = DateTime.UtcNow.AddMinutes(1);

    [ObservableProperty]
    private string? _userIdFilter;

    [ObservableProperty]
    private string? _entityTypeFilter;

    [ObservableProperty]
    private string? _entityIdFilter;

    [ObservableProperty]
    private string? _actionTypeFilter;

    [ObservableProperty]
    private bool _isLoading;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    public ObservableCollection<AuditLogEntry> Logs { get; } = new();

    public ObservableCollection<string> EntityTypeOptions { get; } = new();
    public ObservableCollection<string> ActionTypeOptions { get; } = new();

    [ObservableProperty]
    private AuditStatistics? _statistics;

    public AuditLogViewerViewModel(IAuditLoggingService auditService)
    {
        _auditService = auditService;
    }

    [RelayCommand]
    public async Task LoadAsync()
    {
        try
        {
            IsLoading = true;
            StatusMessage = "Lade Audit-Logs...";

            var rangeLogs = await _auditService.GetLogsInRangeAsync(From, To);
            var filtered = rangeLogs.AsEnumerable();

            if (!string.IsNullOrWhiteSpace(UserIdFilter))
                filtered = filtered.Where(l => string.Equals(l.UserId, UserIdFilter, StringComparison.OrdinalIgnoreCase));

            if (!string.IsNullOrWhiteSpace(EntityTypeFilter))
                filtered = filtered.Where(l => string.Equals(l.EntityType, EntityTypeFilter, StringComparison.OrdinalIgnoreCase));

            if (!string.IsNullOrWhiteSpace(EntityIdFilter))
                filtered = filtered.Where(l => string.Equals(l.EntityId, EntityIdFilter, StringComparison.OrdinalIgnoreCase));

            if (!string.IsNullOrWhiteSpace(ActionTypeFilter))
                filtered = filtered.Where(l => string.Equals(l.ActionType, ActionTypeFilter, StringComparison.OrdinalIgnoreCase));

            var list = filtered.OrderByDescending(l => l.Timestamp).ToList();

            Logs.Clear();
            foreach (var log in list)
                Logs.Add(log);

            Statistics = await _auditService.GetStatisticsAsync();

            // Optionen aktualisieren
            var entityTypes = rangeLogs.Select(l => l.EntityType).Where(s => !string.IsNullOrWhiteSpace(s)).Distinct(StringComparer.OrdinalIgnoreCase).OrderBy(s => s).ToList();
            var actionTypes = rangeLogs.Select(l => l.ActionType).Where(s => !string.IsNullOrWhiteSpace(s)).Distinct(StringComparer.OrdinalIgnoreCase).OrderBy(s => s).ToList();

            EntityTypeOptions.Clear();
            ActionTypeOptions.Clear();
            foreach (var et in entityTypes) EntityTypeOptions.Add(et);
            foreach (var at in actionTypes) ActionTypeOptions.Add(at);
            StatusMessage = $"{Logs.Count} Einträge geladen.";
        }
        catch (Exception ex)
        {
            StatusMessage = $"Fehler beim Laden: {ex.Message}";
        }
        finally
        {
            IsLoading = false;
        }
    }

    [RelayCommand]
    public void ClearFilters()
    {
        UserIdFilter = null;
        EntityTypeFilter = null;
        EntityIdFilter = null;
        ActionTypeFilter = null;
    }

    public async Task ExportCsvAsync(string filePath)
    {
        try
        {
            IsLoading = true;
            var sb = new StringBuilder();
            sb.AppendLine("Timestamp;UserId;ActionType;EntityType;EntityId;Result;Details");
            foreach (var l in Logs)
            {
                string esc(string? v) => (v ?? string.Empty).Replace("\"", "\"\"");
                var line = string.Join(';',
                    l.Timestamp.ToString("yyyy-MM-dd HH:mm:ss"),
                    $"\"{esc(l.UserId)}\"",
                    $"\"{esc(l.ActionType)}\"",
                    $"\"{esc(l.EntityType)}\"",
                    $"\"{esc(l.EntityId)}\"",
                    $"\"{esc(l.Result?.ToString())}\"",
                    $"\"{esc(l.Details)}\""
                );
                sb.AppendLine(line);
            }
            await System.IO.File.WriteAllTextAsync(filePath, sb.ToString(), Encoding.UTF8);
            StatusMessage = $"CSV exportiert: {filePath}";
        }
        catch (Exception ex)
        {
            StatusMessage = $"CSV-Export fehlgeschlagen: {ex.Message}";
        }
        finally { IsLoading = false; }
    }

    public async Task ExportJsonAsync(string filePath)
    {
        try
        {
            IsLoading = true;
            var json = JsonSerializer.Serialize(Logs, new JsonSerializerOptions { WriteIndented = true });
            await System.IO.File.WriteAllTextAsync(filePath, json, Encoding.UTF8);
            StatusMessage = $"JSON exportiert: {filePath}";
        }
        catch (Exception ex)
        {
            StatusMessage = $"JSON-Export fehlgeschlagen: {ex.Message}";
        }
        finally { IsLoading = false; }
    }
}
