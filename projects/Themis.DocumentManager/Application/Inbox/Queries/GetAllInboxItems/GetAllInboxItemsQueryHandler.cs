/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllInboxItemsQueryHandler.cs                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     93                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 4b4dd1ff7  2025-12-29  Add performance benchmark results and update benchmarking... ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Inbox.Messages;
using Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2;

namespace Themis.DocumentManager.Application.Inbox.Queries.GetAllInboxItems;

public class GetAllInboxItemsQueryHandler : IRequestHandler<GetAllInboxItemsQuery, Result<PagedResult<InboxItemDto>>>
{
    private static readonly Dictionary<string, CreateInboxItemV2CommandHandler.InboxItemInternal> _items = CreateInboxItemV2CommandHandler._items;

    public async Task<Result<PagedResult<InboxItemDto>>> Handle(GetAllInboxItemsQuery request, CancellationToken cancellationToken)
    {
        var query = _items.Values.AsQueryable();

        if (request.Status.HasValue)
            query = query.Where(i => i.Status == request.Status.Value);

        if (request.Priority.HasValue)
            query = query.Where(i => i.Priority == request.Priority.Value);

        if (!string.IsNullOrWhiteSpace(request.AssignedTo))
            query = query.Where(i => i.AssignedTo == request.AssignedTo);

        if (request.IsRead.HasValue)
            query = query.Where(i => i.IsRead == request.IsRead.Value);

        if (!string.IsNullOrWhiteSpace(request.RelatedProcessId))
            query = query.Where(i => i.RelatedProcessId == request.RelatedProcessId);

        var totalCount = query.Count();
        var items = query
            .OrderByDescending(i => i.Priority)
            .ThenByDescending(i => i.ReceivedAt)
            .Skip((request.PageNumber - 1) * request.PageSize)
            .Take(request.PageSize)
            .Select(i => new InboxItemDto
            {
                Id = i.Id ?? string.Empty,
                ReceivedAt = i.ReceivedAt,
                Status = i.Status,
                Priority = i.Priority,
                IsRead = i.IsRead,
                AssignedTo = i.AssignedTo,
                AssignedBy = i.AssignedBy,
                AssignedAt = i.AssignedAt,
                DocumentId = i.DocumentId ?? string.Empty,
                Subject = i.Subject ?? string.Empty,
                Sender = i.Sender ?? string.Empty,
                SenderEmail = i.SenderEmail,
                Description = i.Description,
                RelatedProcessId = i.RelatedProcessId,
                Notes = i.Notes,
                Metadata = i.Metadata ?? new(),
                CreatedAt = i.CreatedAt,
                CreatedBy = i.CreatedBy ?? "System",
                UpdatedAt = i.UpdatedAt ?? i.CreatedAt,
                UpdatedBy = i.UpdatedBy ?? i.CreatedBy ?? "System"
            })
            .ToList();

        var result = new PagedResult<InboxItemDto>
        {
            Items = items,
            TotalCount = totalCount,
            PageNumber = request.PageNumber,
            PageSize = request.PageSize
        };

        return await Task.FromResult(Result<PagedResult<InboxItemDto>>.Ok(result));
    }

}
