using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Inbox.Messages;
using Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2;

namespace Themis.DocumentManager.Application.Inbox.Queries.GetInboxItemById;

public class GetInboxItemByIdQueryHandler : IRequestHandler<GetInboxItemByIdQuery, Result<InboxItemDto>>
{
    private static readonly Dictionary<string, object> _items = GetSharedStorage();

    private static Dictionary<string, object> GetSharedStorage()
    {
        var createHandlerType = typeof(CreateInboxItemV2CommandHandler);
        var field = createHandlerType.GetField("_items", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static);
        return field?.GetValue(null) as Dictionary<string, object> ?? new();
    }

    public async Task<Result<InboxItemDto>> Handle(GetInboxItemByIdQuery request, CancellationToken cancellationToken)
    {
        if (!_items.TryGetValue(request.Id, out var item))
        {
            return await Task.FromResult(Result<InboxItemDto>.Fail("Inbox-Element nicht gefunden"));
        }

        var inboxItem = (InboxItemInternal)item;

        var dto = new InboxItemDto
        {
            Id = inboxItem.Id,
            ReceivedAt = inboxItem.ReceivedAt,
            Status = inboxItem.Status,
            Priority = inboxItem.Priority,
            IsRead = inboxItem.IsRead,
            AssignedTo = inboxItem.AssignedTo,
            AssignedBy = inboxItem.AssignedBy,
            AssignedAt = inboxItem.AssignedAt,
            DocumentId = inboxItem.DocumentId,
            Subject = inboxItem.Subject,
            Sender = inboxItem.Sender,
            SenderEmail = inboxItem.SenderEmail,
            Description = inboxItem.Description,
            RelatedProcessId = inboxItem.RelatedProcessId,
            Notes = inboxItem.Notes,
            Metadata = inboxItem.Metadata,
            CreatedAt = inboxItem.CreatedAt,
            CreatedBy = inboxItem.CreatedBy,
            UpdatedAt = inboxItem.UpdatedAt!,
            UpdatedBy = inboxItem.UpdatedBy
        };

        return await Task.FromResult(Result<InboxItemDto>.Ok(dto));
    }

    private class InboxItemInternal
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
