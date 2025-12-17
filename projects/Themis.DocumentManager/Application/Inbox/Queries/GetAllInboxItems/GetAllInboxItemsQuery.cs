using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Inbox.Messages;

namespace Themis.DocumentManager.Application.Inbox.Queries.GetAllInboxItems;

public record GetAllInboxItemsQuery : IRequest<Result<PagedResult<InboxItemDto>>>
{
    public InboxStatus? Status { get; init; }
    public InboxPriority? Priority { get; init; }
    public string? AssignedTo { get; init; }
    public bool? IsRead { get; init; }
    public string? RelatedProcessId { get; init; }
    public int PageNumber { get; init; } = 1;
    public int PageSize { get; init; } = 10;
}
