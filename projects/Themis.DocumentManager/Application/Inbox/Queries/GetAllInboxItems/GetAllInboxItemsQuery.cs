/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllInboxItemsQuery.cs                           ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     36                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
