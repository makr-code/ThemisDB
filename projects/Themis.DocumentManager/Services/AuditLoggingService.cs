/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AuditLoggingService.cs                             ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     160                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Audit-Logging Service für CRUD-Operationen und Benutzeraktionen
/// Protokolliert alle Zugriffe nach rollenbasierten Berechtigungen
/// </summary>
public interface IAuditLoggingService
{
    Task LogActionAsync(AuditLogEntry entry, CancellationToken cancellationToken = default);
    Task<List<AuditLogEntry>> GetAuditLogsAsync(string entityId, CancellationToken cancellationToken = default);
    Task<List<AuditLogEntry>> GetUserAuditLogsAsync(string userId, CancellationToken cancellationToken = default);
    Task<List<AuditLogEntry>> GetLogsInRangeAsync(DateTime from, DateTime to, CancellationToken cancellationToken = default);
    Task<AuditStatistics> GetStatisticsAsync(CancellationToken cancellationToken = default);
}

public class AuditLoggingService : IAuditLoggingService
{
    private readonly List<AuditLogEntry> _auditLogs = new();

    public async Task LogActionAsync(AuditLogEntry entry, CancellationToken cancellationToken = default)
    {
        if (entry == null)
            throw new ArgumentNullException(nameof(entry));

        entry.Id = Guid.NewGuid().ToString();
        entry.LoggedAt = DateTime.UtcNow;
        _auditLogs.Add(entry);

        // Optional: Persist to database/file
        await Task.CompletedTask;
        System.Diagnostics.Debug.WriteLine($"[AUDIT] {entry.ActionType} | User: {entry.UserId} | Entity: {entry.EntityId} | {entry.Timestamp:yyyy-MM-dd HH:mm:ss}");
    }

    public async Task<List<AuditLogEntry>> GetAuditLogsAsync(string entityId, CancellationToken cancellationToken = default)
    {
        var logs = _auditLogs
            .Where(l => l.EntityId == entityId)
            .OrderByDescending(l => l.Timestamp)
            .ToList();
        await Task.CompletedTask;
        return logs;
    }

    public async Task<List<AuditLogEntry>> GetUserAuditLogsAsync(string userId, CancellationToken cancellationToken = default)
    {
        var logs = _auditLogs
            .Where(l => l.UserId == userId)
            .OrderByDescending(l => l.Timestamp)
            .ToList();
        await Task.CompletedTask;
        return logs;
    }

    public async Task<List<AuditLogEntry>> GetLogsInRangeAsync(DateTime from, DateTime to, CancellationToken cancellationToken = default)
    {
        var logs = _auditLogs
            .Where(l => l.Timestamp >= from && l.Timestamp <= to)
            .OrderByDescending(l => l.Timestamp)
            .ToList();
        await Task.CompletedTask;
        return logs;
    }

    public async Task<AuditStatistics> GetStatisticsAsync(CancellationToken cancellationToken = default)
    {
        var stats = new AuditStatistics
        {
            TotalLogCount = _auditLogs.Count,
            UniqueUsers = _auditLogs.Select(l => l.UserId).Distinct().Count(),
            UniqueEntities = _auditLogs.Select(l => l.EntityId).Distinct().Count(),
            ActionTypeCounts = _auditLogs
                .GroupBy(l => l.ActionType)
                .ToDictionary(g => g.Key, g => g.Count()),
            EntityTypeCounts = _auditLogs
                .GroupBy(l => l.EntityType)
                .ToDictionary(g => g.Key, g => g.Count()),
            UsersWithMostActions = _auditLogs
                .GroupBy(l => l.UserId)
                .OrderByDescending(g => g.Count())
                .Take(10)
                .ToDictionary(g => g.Key, g => g.Count()),
            MostAccessedEntities = _auditLogs
                .GroupBy(l => l.EntityId)
                .OrderByDescending(g => g.Count())
                .Take(10)
                .ToDictionary(g => g.Key, g => g.Count())
        };
        await Task.CompletedTask;
        return stats;
    }
}

#region DTOs

public class AuditLogEntry
{
    public string Id { get; set; } = string.Empty;
    public string UserId { get; set; } = string.Empty;
    public string ActionType { get; set; } = string.Empty;
    public string EntityType { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public string? Details { get; set; }
    public DateTime Timestamp { get; set; }
    public DateTime LoggedAt { get; set; }
    public string? IpAddress { get; set; }
    public string? UserAgent { get; set; }
    public AuditActionResult? Result { get; set; }
}

public enum AuditActionResult
{
    Success,
    Failed,
    Denied,
    PartialSuccess
}

public class AuditStatistics
{
    public int TotalLogCount { get; set; }
    public int UniqueUsers { get; set; }
    public int UniqueEntities { get; set; }
    public Dictionary<string, int> ActionTypeCounts { get; set; } = new();
    public Dictionary<string, int> EntityTypeCounts { get; set; } = new();
    public Dictionary<string, int> UsersWithMostActions { get; set; } = new();
    public Dictionary<string, int> MostAccessedEntities { get; set; } = new();
}

#endregion
