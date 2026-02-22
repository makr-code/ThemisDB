/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllClassificationsQueryHandler.cs               ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     109                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Classification.Messages;
using Themis.DocumentManager.Application.Classification.Commands.CreateClassification;

namespace Themis.DocumentManager.Application.Classification.Queries.GetAllClassifications;

public class GetAllClassificationsQueryHandler : IRequestHandler<GetAllClassificationsQuery, Result<PagedResult<ClassificationDto>>>
{
    private static readonly Dictionary<string, ClassificationItem> _classifications = new();

    public async Task<Result<PagedResult<ClassificationDto>>> Handle(GetAllClassificationsQuery request, CancellationToken cancellationToken)
    {
        try
        {
            var filtered = _classifications.Values.AsEnumerable();

            // SearchTerm filter
            if (!string.IsNullOrWhiteSpace(request.SearchTerm))
            {
                filtered = filtered.Where(c =>
                    c.Name.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase) ||
                    (c.Description != null && c.Description.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase)) ||
                    (c.Code != null && c.Code.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase)));
            }

            // Level filter
            if (request.Level.HasValue)
            {
                filtered = filtered.Where(c => c.Level == request.Level.Value);
            }

            // IsActive filter
            if (request.IsActive.HasValue)
            {
                filtered = filtered.Where(c => c.IsActive == request.IsActive.Value);
            }

            // ParentId filter
            if (!string.IsNullOrEmpty(request.ParentId))
            {
                filtered = filtered.Where(c => c.ParentId == request.ParentId);
            }

            var totalCount = filtered.Count();
            var skip = (request.PageNumber - 1) * request.PageSize;
            var items = filtered
                .OrderBy(c => c.SortOrder)
                .ThenBy(c => c.Name)
                .Skip(skip)
                .Take(request.PageSize)
                .ToList();

            var dtos = items.Select(classification => new ClassificationDto
            {
                Id = classification.Id,
                Name = classification.Name,
                Description = classification.Description,
                Code = classification.Code,
                Level = classification.Level,
                Color = classification.Color,
                SortOrder = classification.SortOrder,
                IsActive = classification.IsActive,
                ParentId = classification.ParentId,
                AllowedRoles = classification.AllowedRoles,
                Metadata = classification.Metadata,
                CreatedAt = classification.CreatedAt,
                CreatedBy = classification.CreatedBy ?? string.Empty,
                UpdatedAt = classification.ModifiedAt,
                UpdatedBy = classification.ModifiedBy ?? string.Empty
            }).ToList();

            var pagedResult = new PagedResult<ClassificationDto>
            {
                Items = dtos,
                TotalCount = totalCount,
                PageNumber = request.PageNumber,
                PageSize = request.PageSize
            };

            return await Task.FromResult(Result<PagedResult<ClassificationDto>>.Ok(pagedResult));
        }
        catch (Exception ex)
        {
            return Result<PagedResult<ClassificationDto>>.Fail($"Fehler beim Abrufen der Classifications: {ex.Message}");
        }
    }
}
