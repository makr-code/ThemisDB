/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateInboxItemCommandHandler.cs                   ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     98                                             ║
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
