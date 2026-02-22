/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetInboxItemsQueryHandler.cs                       ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
