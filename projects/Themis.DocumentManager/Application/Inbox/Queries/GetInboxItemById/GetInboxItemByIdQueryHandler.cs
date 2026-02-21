/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetInboxItemByIdQueryHandler.cs                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 4b4dd1ff7  2025-12-29  Add performance benchmark results and update benchmarking... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Inbox.Messages;
using Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2;

namespace Themis.DocumentManager.Application.Inbox.Queries.GetInboxItemById;

public class GetInboxItemByIdQueryHandler : IRequestHandler<GetInboxItemByIdQuery, Result<InboxItemDto>>
{
    private static readonly Dictionary<string, CreateInboxItemV2CommandHandler.InboxItemInternal> _items = CreateInboxItemV2CommandHandler._items;

    public async Task<Result<InboxItemDto>> Handle(GetInboxItemByIdQuery request, CancellationToken cancellationToken)
    {
        if (!_items.TryGetValue(request.Id, out var inboxItem) || inboxItem is null)
        {
            return await Task.FromResult(Result<InboxItemDto>.Fail("Inbox-Element nicht gefunden"));
        }

        var dto = new InboxItemDto
        {
            Id = inboxItem.Id ?? string.Empty,
            ReceivedAt = inboxItem.ReceivedAt,
            Status = inboxItem.Status,
            Priority = inboxItem.Priority,
            IsRead = inboxItem.IsRead,
            AssignedTo = inboxItem.AssignedTo,
            AssignedBy = inboxItem.AssignedBy,
            AssignedAt = inboxItem.AssignedAt,
            DocumentId = inboxItem.DocumentId ?? string.Empty,
            Subject = inboxItem.Subject ?? string.Empty,
            Sender = inboxItem.Sender ?? string.Empty,
            SenderEmail = inboxItem.SenderEmail,
            Description = inboxItem.Description,
            RelatedProcessId = inboxItem.RelatedProcessId,
            Notes = inboxItem.Notes,
            Metadata = inboxItem.Metadata ?? new(),
            CreatedAt = inboxItem.CreatedAt,
            CreatedBy = inboxItem.CreatedBy ?? "System",
            UpdatedAt = inboxItem.UpdatedAt ?? inboxItem.CreatedAt,
            UpdatedBy = inboxItem.UpdatedBy ?? inboxItem.CreatedBy ?? "System"
        };

        return await Task.FromResult(Result<InboxItemDto>.Ok(dto));
    }

}
