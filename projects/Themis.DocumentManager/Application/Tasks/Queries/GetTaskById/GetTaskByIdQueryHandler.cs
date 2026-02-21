/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetTaskByIdQueryHandler.cs                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
