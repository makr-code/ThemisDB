/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateInboxItemCommandHandler.cs                   ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     91                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2;
using Themis.DocumentManager.Application.Inbox.Messages;

namespace Themis.DocumentManager.Application.Inbox.Commands.UpdateInboxItem;

public class UpdateInboxItemCommandHandler : IRequestHandler<UpdateInboxItemCommand, Result<InboxItemDto>>
{
    private static readonly Dictionary<string, CreateInboxItemV2CommandHandler.InboxItemInternal> _items = CreateInboxItemV2CommandHandler._items;

    public async Task<Result<InboxItemDto>> Handle(UpdateInboxItemCommand request, CancellationToken cancellationToken)
    {
        if (!_items.TryGetValue(request.Id, out var item) || item is null)
        {
            return await Task.FromResult(Result<InboxItemDto>.Fail("Inbox-Element nicht gefunden"));
        }

        var now = DateTime.UtcNow;

        if (request.Status.HasValue)
            item.Status = request.Status.Value;

        if (request.Priority.HasValue)
            item.Priority = request.Priority.Value;

        if (request.IsRead.HasValue)
            item.IsRead = request.IsRead.Value;

        if (!string.IsNullOrWhiteSpace(request.AssignedTo))
        {
            item.AssignedTo = request.AssignedTo;
            item.AssignedBy = "System";
            item.AssignedAt = now;
        }

        if (!string.IsNullOrWhiteSpace(request.Notes))
            item.Notes = request.Notes;

        if (!string.IsNullOrWhiteSpace(request.Description))
            item.Description = request.Description;

        item.UpdatedAt = now;
        item.UpdatedBy = "System";

        var dto = new InboxItemDto
        {
            Id = item.Id ?? string.Empty,
            ReceivedAt = item.ReceivedAt,
            Status = item.Status,
            Priority = item.Priority,
            IsRead = item.IsRead,
            AssignedTo = item.AssignedTo,
            AssignedBy = item.AssignedBy,
            AssignedAt = item.AssignedAt,
            DocumentId = item.DocumentId ?? string.Empty,
            Subject = item.Subject ?? string.Empty,
            Sender = item.Sender ?? string.Empty,
            SenderEmail = item.SenderEmail,
            Description = item.Description,
            RelatedProcessId = item.RelatedProcessId,
            Notes = item.Notes,
            Metadata = item.Metadata ?? new(),
            CreatedAt = item.CreatedAt,
            CreatedBy = item.CreatedBy ?? "System",
            UpdatedAt = item.UpdatedAt ?? item.CreatedAt,
            UpdatedBy = item.UpdatedBy ?? item.CreatedBy ?? "System"
        };

        return await Task.FromResult(Result<InboxItemDto>.Ok(dto));
    }
}
