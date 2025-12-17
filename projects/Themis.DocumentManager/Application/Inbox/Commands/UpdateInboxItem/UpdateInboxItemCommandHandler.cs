using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Inbox.Messages;

namespace Themis.DocumentManager.Application.Inbox.Commands.UpdateInboxItem;

public class UpdateInboxItemCommandHandler : IRequestHandler<UpdateInboxItemCommand, Result<InboxItemDto>>
{
    private static readonly Dictionary<string, InboxItemInternal> _items = GetSharedStorage();

    private static Dictionary<string, InboxItemInternal> GetSharedStorage()
    {
        var createHandlerType = Type.GetType("Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2.CreateInboxItemV2CommandHandler");
        if (createHandlerType == null) return new();
        
        var field = createHandlerType.GetField("_items", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static);
        return field?.GetValue(null) as Dictionary<string, InboxItemInternal> ?? new();
    }

    public async Task<Result<InboxItemDto>> Handle(UpdateInboxItemCommand request, CancellationToken cancellationToken)
    {
        if (!_items.TryGetValue(request.Id, out var item))
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
            UpdatedAt = item.UpdatedAt!,
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
