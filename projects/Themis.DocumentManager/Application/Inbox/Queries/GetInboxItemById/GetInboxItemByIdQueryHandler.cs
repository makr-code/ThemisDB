/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetInboxItemByIdQueryHandler.cs                    ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     72                                             ║
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
