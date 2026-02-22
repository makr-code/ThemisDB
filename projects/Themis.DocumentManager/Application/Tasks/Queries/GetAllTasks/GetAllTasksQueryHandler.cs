/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllTasksQueryHandler.cs                         ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     124                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

﻿using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Tasks.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Queries.GetAllTasks;

public class GetAllTasksQueryHandler : IRequestHandler<GetAllTasksQuery, Result<PagedResult<TaskItemDto>>>
{
    private static readonly Dictionary<string, OutlookTask> _tasks = new();

    public async Task<Result<PagedResult<TaskItemDto>>> Handle(GetAllTasksQuery request, CancellationToken cancellationToken)
    {
        try
        {
            var filtered = _tasks.Values.AsEnumerable();

            // SearchTerm filter
            if (!string.IsNullOrWhiteSpace(request.SearchTerm))
            {
                filtered = filtered.Where(t =>
                    t.Subject.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase) ||
                    t.Body.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase));
            }

            // ProcessId filter
            if (!string.IsNullOrEmpty(request.ProcessId))
            {
                filtered = filtered.Where(t => t.ProcessId == request.ProcessId);
            }

            // AssignedTo filter
            if (!string.IsNullOrEmpty(request.AssignedTo))
            {
                filtered = filtered.Where(t => t.AssignedTo == request.AssignedTo);
            }

            // Status filter
            if (request.Status.HasValue)
            {
                filtered = filtered.Where(t => t.Status == request.Status.Value);
            }

            // Priority filter
            if (request.Priority.HasValue)
            {
                filtered = filtered.Where(t => t.Priority == request.Priority.Value);
            }

            // IsOverdue filter
            if (request.IsOverdue == true)
            {
                var now = DateTime.UtcNow;
                filtered = filtered.Where(t => t.DueDate < now && t.Status != OutlookTaskStatus.Completed);
            }

            var totalCount = filtered.Count();
            var skip = (request.PageNumber - 1) * request.PageSize;
            var pagedTasks = filtered
                .OrderByDescending(t => t.DueDate)
                .Skip(skip)
                .Take(request.PageSize)
                .ToList();

            var dtos = pagedTasks.Select(task => new TaskItemDto
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
                CreatedBy = task.Owner ?? "System",
                UpdatedAt = task.CreatedAt,
                UpdatedBy = string.Empty
            }).ToList();

            var pagedResult = new PagedResult<TaskItemDto>
            {
                Items = dtos,
                TotalCount = totalCount,
                PageNumber = request.PageNumber,
                PageSize = request.PageSize
            };

            return await Task.FromResult(Result<PagedResult<TaskItemDto>>.Ok(pagedResult));
        }
        catch (Exception ex)
        {
            return Result<PagedResult<TaskItemDto>>.Fail($"Fehler beim Abrufen der Tasks: {ex.Message}");
        }
    }
}
