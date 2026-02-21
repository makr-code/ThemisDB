/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IGetAllQuery.cs                                    ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     73                                             ║
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
using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Common.Queries;

/// <summary>
/// Generic interface for GetAll queries with pagination
/// </summary>
/// <typeparam name="TDto">The DTO type to return</typeparam>
public interface IGetAllQuery<TDto> : IRequest<Result<PagedResult<TDto>>>
    where TDto : IEntityDto
{
    /// <summary>
    /// Page number (1-based)
    /// </summary>
    int PageNumber { get; init; }
    
    /// <summary>
    /// Page size (number of items per page)
    /// </summary>
    int PageSize { get; init; }
    
    /// <summary>
    /// Optional search term
    /// </summary>
    string? SearchTerm { get; init; }
    
    /// <summary>
    /// Optional filter criteria
    /// </summary>
    Dictionary<string, object>? Filters { get; init; }
}

/// <summary>
/// Paginated result wrapper
/// </summary>
/// <typeparam name="T">Item type</typeparam>
public record PagedResult<T>
{
    public List<T> Items { get; init; } = new();
    public int TotalCount { get; init; }
    public int PageNumber { get; init; }
    public int PageSize { get; init; }
    public int TotalPages => (int)Math.Ceiling((double)TotalCount / PageSize);
    public bool HasPreviousPage => PageNumber > 1;
    public bool HasNextPage => PageNumber < TotalPages;
}
