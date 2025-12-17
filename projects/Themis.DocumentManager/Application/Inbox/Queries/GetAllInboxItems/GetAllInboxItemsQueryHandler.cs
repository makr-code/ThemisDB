using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Inbox.Messages;
using Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItemV2;

namespace Themis.DocumentManager.Application.Inbox.Queries.GetAllInboxItems;

public class GetAllInboxItemsQueryHandler : IRequestHandler<GetAllInboxItemsQuery, Result<PagedResult<InboxItemDto>>>
{
    private static readonly Dictionary<string, InboxItemInternal> _items = GetSharedStorage();

    private static Dictionary<string, InboxItemInternal> GetSharedStorage()
    {
        var createHandlerType = typeof(CreateInboxItemV2CommandHandler);
        var field = createHandlerType.GetField("_items", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static);
        return field?.GetValue(null) as Dictionary<string, InboxItemInternal> ?? new();
    }

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
                Id = i.Id,
                ReceivedAt = i.ReceivedAt,
                Status = i.Status,
                Priority = i.Priority,
                IsRead = i.IsRead,
                AssignedTo = i.AssignedTo,
                AssignedBy = i.AssignedBy,
                AssignedAt = i.AssignedAt,
                DocumentId = i.DocumentId,
                Subject = i.Subject,
                Sender = i.Sender,
                SenderEmail = i.SenderEmail,
                Description = i.Description,
                RelatedProcessId = i.RelatedProcessId,
                Notes = i.Notes,
                Metadata = i.Metadata,
                CreatedAt = i.CreatedAt,
                CreatedBy = i.CreatedBy,
                UpdatedAt = i.UpdatedAt!,
                UpdatedBy = i.UpdatedBy
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
