/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FormAuditService.cs                                ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     177                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Form audit entry for tracking submissions
/// </summary>
public class FormAuditEntry
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string FormId { get; set; } = string.Empty;
    public string SubmissionId { get; set; } = string.Empty;
    public string Action { get; set; } = string.Empty; // SUBMIT, VALIDATE, DELETE, etc.
    public string UserId { get; set; } = string.Empty;
    public DateTime Timestamp { get; set; } = DateTime.Now;
    public Dictionary<string, object>? Details { get; set; }
    public string Status { get; set; } = "Success"; // Success, Warning, Error
    public string? ErrorMessage { get; set; }
}

/// <summary>
/// Service interface for form audit logging
/// </summary>
public interface IFormAuditService
{
    Task LogSubmissionAsync(FormSubmissionData submission, string userId, string action, string status = "Success", string? errorMessage = null, CancellationToken cancellationToken = default);
    Task<List<FormAuditEntry>> GetAuditTrailAsync(string formId, DateTime from, DateTime to, CancellationToken cancellationToken = default);
    Task<List<FormAuditEntry>> GetSubmissionAuditAsync(string submissionId, CancellationToken cancellationToken = default);
    Task<int> GetSubmissionCountAsync(string formId, CancellationToken cancellationToken = default);
    Task<List<FormAuditEntry>> GetUserActivityAsync(string userId, DateTime from, DateTime to, CancellationToken cancellationToken = default);
    Task ClearAuditTrailAsync(string formId, CancellationToken cancellationToken = default);
}

/// <summary>
/// In-memory implementation of form audit service
/// </summary>
public class FormAuditService : IFormAuditService
{
    private readonly List<FormAuditEntry> _auditLog = new();
    private readonly object _lockObject = new();

    public Task LogSubmissionAsync(
        FormSubmissionData submission, 
        string userId, 
        string action, 
        string status = "Success", 
        string? errorMessage = null, 
        CancellationToken cancellationToken = default)
    {
        var entry = new FormAuditEntry
        {
            FormId = submission.FormId,
            SubmissionId = submission.SubmissionId,
            Action = action,
            UserId = userId,
            Timestamp = DateTime.Now,
            Status = status,
            ErrorMessage = errorMessage,
            Details = new Dictionary<string, object>
            {
                { "FieldCount", submission.FieldValues.Count },
                { "SubmittedDate", submission.SubmittedDate },
                { "SubmittedBy", submission.SubmittedBy }
            }
        };

        lock (_lockObject)
        {
            _auditLog.Add(entry);
        }

        return Task.CompletedTask;
    }

    public Task<List<FormAuditEntry>> GetAuditTrailAsync(
        string formId, 
        DateTime from, 
        DateTime to, 
        CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            var entries = _auditLog
                .Where(e => e.FormId == formId && e.Timestamp >= from && e.Timestamp <= to)
                .OrderByDescending(e => e.Timestamp)
                .ToList();
            return Task.FromResult(entries);
        }
    }

    public Task<List<FormAuditEntry>> GetSubmissionAuditAsync(
        string submissionId, 
        CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            var entries = _auditLog
                .Where(e => e.SubmissionId == submissionId)
                .OrderByDescending(e => e.Timestamp)
                .ToList();
            return Task.FromResult(entries);
        }
    }

    public Task<int> GetSubmissionCountAsync(
        string formId, 
        CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            var count = _auditLog
                .Where(e => e.FormId == formId && e.Action == "SUBMIT")
                .Count();
            return Task.FromResult(count);
        }
    }

    public Task<List<FormAuditEntry>> GetUserActivityAsync(
        string userId, 
        DateTime from, 
        DateTime to, 
        CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            var entries = _auditLog
                .Where(e => e.UserId == userId && e.Timestamp >= from && e.Timestamp <= to)
                .OrderByDescending(e => e.Timestamp)
                .ToList();
            return Task.FromResult(entries);
        }
    }

    public Task ClearAuditTrailAsync(
        string formId, 
        CancellationToken cancellationToken = default)
    {
        lock (_lockObject)
        {
            var toRemove = _auditLog.Where(e => e.FormId == formId).ToList();
            foreach (var entry in toRemove)
                _auditLog.Remove(entry);
        }

        return Task.CompletedTask;
    }
}
