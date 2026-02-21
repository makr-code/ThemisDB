/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateTaskCommandHandler.cs                        ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     95                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Tasks.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Commands.CreateTask;

/// <summary>
/// Handler for CreateTaskCommand
/// </summary>
public class CreateTaskCommandHandler : IRequestHandler<CreateTaskCommand, Result<TaskItemDto>>
{
    private static readonly List<OutlookTask> _tasks = new(); // Temporary in-memory storage

    public async Task<Result<TaskItemDto>> Handle(CreateTaskCommand request, CancellationToken cancellationToken)
    {
        try
        {
            var task = new OutlookTask
            {
                Id = Guid.NewGuid().ToString(),
                Subject = request.Subject,
                Body = request.Body,
                DueDate = request.DueDate,
                StartDate = request.StartDate,
                Status = OutlookTaskStatus.NotStarted,
                Priority = request.Priority,
                PercentComplete = 0,
                ProcessId = request.ProcessId,
                AssignedTo = request.AssignedTo,
                Owner = request.Owner,
                Categories = request.Categories,
                CreatedAt = DateTime.UtcNow
            };

            _tasks.Add(task);

            var dto = MapToDto(task, request.Owner ?? "System");
            return await Task.FromResult(Result<TaskItemDto>.Ok(dto));
        }
        catch (Exception ex)
        {
            return Result<TaskItemDto>.Fail($"Fehler beim Erstellen der Aufgabe: {ex.Message}");
        }
    }

    private static TaskItemDto MapToDto(OutlookTask task, string createdBy)
    {
        return new TaskItemDto
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
            CreatedAt = task.CreatedAt,
            CreatedBy = createdBy,
            UpdatedAt = task.CreatedAt
        };
    }
}
