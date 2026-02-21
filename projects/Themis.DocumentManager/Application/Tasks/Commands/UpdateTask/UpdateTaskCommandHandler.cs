/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateTaskCommandHandler.cs                        ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     78                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Tasks.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Commands.UpdateTask;

public class UpdateTaskCommandHandler : IRequestHandler<UpdateTaskCommand, Result>
{
    // Temporary in-memory storage (replace with repository)
    private static readonly Dictionary<string, OutlookTask> _tasks = new();

    public async Task<Result> Handle(UpdateTaskCommand request, CancellationToken cancellationToken)
    {
        try
        {
            if (!_tasks.ContainsKey(request.Id))
            {
                return Result.Fail($"Task mit ID {request.Id} wurde nicht gefunden");
            }

            var task = _tasks[request.Id];

            // Update only provided fields
            if (request.Subject != null) task.Subject = request.Subject;
            if (request.Body != null) task.Body = request.Body;
            if (request.DueDate.HasValue) task.DueDate = request.DueDate.Value;
            if (request.StartDate.HasValue) task.StartDate = request.StartDate.Value;
            if (request.Status.HasValue) task.Status = request.Status.Value;
            if (request.Priority.HasValue) task.Priority = request.Priority.Value;
            if (request.PercentComplete.HasValue) task.PercentComplete = request.PercentComplete.Value;
            if (request.ProcessId != null) task.ProcessId = request.ProcessId;
            if (request.AssignedTo != null) task.AssignedTo = request.AssignedTo;
            if (request.Owner != null) task.Owner = request.Owner;
            if (request.Categories != null) task.Categories = request.Categories;

            // ModifiedAt not available on OutlookTask

            if (request.Status == OutlookTaskStatus.Completed && task.CompletedAt == null)
            {
                task.CompletedAt = DateTime.UtcNow;
            }

            return await Task.FromResult(Result.Ok());
        }
        catch (Exception ex)
        {
            return Result.Fail($"Fehler beim Aktualisieren der Task: {ex.Message}");
        }
    }
}
