/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateInboxItemV2CommandHandler.cs                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     119                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Inbox.Messages;

namespace Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2;

public class CreateInboxItemV2CommandHandler : IRequestHandler<CreateInboxItemV2Command, Result<InboxItemDto>>
{
    internal static readonly Dictionary<string, InboxItemInternal> _items = new();

    public async Task<Result<InboxItemDto>> Handle(CreateInboxItemV2Command request, CancellationToken cancellationToken)
    {
        var id = Guid.NewGuid().ToString();
        var now = DateTime.UtcNow;

        var item = new InboxItemInternal
        {
            Id = id,
            Subject = request.Subject,
            Sender = request.Sender,
            SenderEmail = request.SenderEmail,
            DocumentId = request.DocumentId ?? string.Empty,
            Description = request.Description,
            Priority = request.Priority,
            AssignedTo = request.AssignedTo ?? string.Empty,
            RelatedProcessId = request.RelatedProcessId,
            Notes = request.Notes,
            Metadata = request.Metadata,
            ReceivedAt = now,
            Status = InboxStatus.New,
            IsRead = false,
            CreatedAt = now,
            CreatedBy = "System"
        };

        if (!string.IsNullOrWhiteSpace(request.AssignedTo))
        {
            item.Status = InboxStatus.Assigned;
            item.AssignedBy = "System";
            item.AssignedAt = now;
        }

        _items[id] = item;

        var dto = new InboxItemDto
        {
            Id = item.Id,
            ReceivedAt = item.ReceivedAt,
            Status = item.Status,
            Priority = item.Priority,
            IsRead = item.IsRead,
            AssignedTo = item.AssignedTo,
            AssignedBy = item.AssignedBy,
            AssignedAt = item.AssignedAt,
            DocumentId = item.DocumentId,
            Subject = item.Subject,
            Sender = item.Sender,
            SenderEmail = item.SenderEmail,
            Description = item.Description,
            RelatedProcessId = item.RelatedProcessId,
            Notes = item.Notes,
            Metadata = item.Metadata,
            CreatedAt = item.CreatedAt,
            CreatedBy = item.CreatedBy,
            UpdatedAt = item.UpdatedAt,
            UpdatedBy = item.UpdatedBy
        };

        return await Task.FromResult(Result<InboxItemDto>.Ok(dto));
    }

    internal class InboxItemInternal
    {
        public string Id { get; set; } = string.Empty;
        public DateTime ReceivedAt { get; set; }
        public InboxStatus Status { get; set; }
        public InboxPriority Priority { get; set; }
        public bool IsRead { get; set; }
        public string AssignedTo { get; set; } = string.Empty;
        public string? AssignedBy { get; set; }
        public DateTime? AssignedAt { get; set; }
        public string DocumentId { get; set; } = string.Empty;
        public string Subject { get; set; } = string.Empty;
        public string Sender { get; set; } = string.Empty;
        public string? SenderEmail { get; set; }
        public string? Description { get; set; }
        public string? RelatedProcessId { get; set; }
        public string? Notes { get; set; }
        public Dictionary<string, object> Metadata { get; set; } = new();
        public DateTime CreatedAt { get; set; }
        public string? CreatedBy { get; set; }
        public DateTime? UpdatedAt { get; set; }
        public string? UpdatedBy { get; set; }
    }
}
