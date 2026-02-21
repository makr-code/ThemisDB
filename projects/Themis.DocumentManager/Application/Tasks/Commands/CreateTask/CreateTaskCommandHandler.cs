/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateTaskCommandHandler.cs                        ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     95                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
