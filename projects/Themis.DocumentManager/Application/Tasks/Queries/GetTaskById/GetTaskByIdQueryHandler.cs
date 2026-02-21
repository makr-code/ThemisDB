/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetTaskByIdQueryHandler.cs                         ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     81                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Tasks.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Queries.GetTaskById;

public class GetTaskByIdQueryHandler : IRequestHandler<GetTaskByIdQuery, Result<TaskItemDto>>
{
    // Temporary in-memory storage (replace with repository)
    private static readonly Dictionary<string, OutlookTask> _tasks = new();

    public async Task<Result<TaskItemDto>> Handle(GetTaskByIdQuery request, CancellationToken cancellationToken)
    {
        try
        {
            if (!_tasks.ContainsKey(request.Id))
            {
                return Result<TaskItemDto>.Fail($"Task mit ID {request.Id} wurde nicht gefunden");
            }

            var task = _tasks[request.Id];

            var dto = new TaskItemDto
            {
                Id = task.Id,
                Subject = task.Subject,
                Body = task.Body,
                DueDate = task.DueDate,
                StartDate = task.StartDate,
                Status = task.Status,
                Priority = task.Priority,
                PercentComplete = task.PercentComplete,
                ProcessId = task.ProcessId,
                AssignedTo = task.AssignedTo,
                Owner = task.Owner,
                Categories = task.Categories,
                CompletedAt = task.CompletedAt,
                LinkedEntityId = null,
                LinkedEntityType = null,
                CreatedAt = task.CreatedAt,
                CreatedBy = string.Empty,
                UpdatedAt = DateTime.MinValue,
                UpdatedBy = string.Empty
            };

            return await Task.FromResult(Result<TaskItemDto>.Ok(dto));
        }
        catch (Exception ex)
        {
            return Result<TaskItemDto>.Fail($"Fehler beim Abrufen der Task: {ex.Message}");
        }
    }
}
