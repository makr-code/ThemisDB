/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllTasksQuery.cs                                ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
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

/// <summary>
/// Query to get all tasks with pagination
/// </summary>
public record GetAllTasksQuery : IGetAllQuery<TaskItemDto>
{
    public int PageNumber { get; init; } = 1;
    public int PageSize { get; init; } = 50;
    public string? SearchTerm { get; init; }
    public Dictionary<string, object>? Filters { get; init; }
    
    // Specific filter options
    public string? ProcessId { get; init; }
    public string? AssignedTo { get; init; }
    public OutlookTaskStatus? Status { get; init; }
    public OutlookTaskPriority? Priority { get; init; }
    public bool? IsOverdue { get; init; }
}
