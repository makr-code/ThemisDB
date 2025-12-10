using MediatR;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Application.Inbox.Queries.GetInboxItems;

/// <summary>
/// Handler for GetInboxItemsQuery
/// </summary>
public class GetInboxItemsQueryHandler : IRequestHandler<GetInboxItemsQuery, List<InboxItem>>
{
    private readonly IInboxService _inboxService;

    public GetInboxItemsQueryHandler(IInboxService inboxService)
    {
        _inboxService = inboxService;
    }

    public async Task<List<InboxItem>> Handle(GetInboxItemsQuery request, CancellationToken cancellationToken)
    {
        var items = await _inboxService.GetInboxItemsAsync(request.Status, request.AssignedTo);
        return items.ToList();
    }
}
