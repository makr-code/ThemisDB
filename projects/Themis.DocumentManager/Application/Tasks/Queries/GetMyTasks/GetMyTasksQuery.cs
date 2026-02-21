/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetMyTasksQuery.cs                                 ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     68                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Queries.GetMyTasks;

/// <summary>
/// Query to get all tasks delegated to current user
/// Supports filtering, sorting, and grouping
/// </summary>
public record GetMyTasksQuery : IRequest<List<TaskItem>>
{
    public string UserId { get; init; } = string.Empty;
    public TaskStatus? StatusFilter { get; init; }
    public TaskPriority? PriorityFilter { get; init; }
    public string? CategoryFilter { get; init; }
    public string? EntityId { get; init; }
    public LinkedEntityType? EntityType { get; init; }
    public string? ProcessId { get; init; }
    public TaskSortBy SortBy { get; init; } = TaskSortBy.DueDate;
    public bool SortDescending { get; init; } = false;
}

public enum TaskSortBy
{
    DueDate,
    Priority,
    CreatedDate,
    Title,
    Category
}

/// <summary>
/// Ziel-Entität, an die eine Aufgabe angehängt ist (Dokument, Vorgang, Akte etc.)
/// </summary>
public enum LinkedEntityType
{
    Document,
    Case,     // Vorgang
    File,     // Akte
    Folder,
    Process
}
