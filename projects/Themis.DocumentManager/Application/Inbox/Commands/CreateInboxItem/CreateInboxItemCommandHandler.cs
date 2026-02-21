/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateInboxItemCommandHandler.cs                   ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItem;

/// <summary>
/// Handler for CreateInboxItemCommand
/// Implements VIS Posteingang functionality
/// </summary>
public class CreateInboxItemCommandHandler : IRequestHandler<CreateInboxItemCommand, string>
{
    private readonly IInboxService _inboxService;
    private readonly IMediator _mediator;

    public CreateInboxItemCommandHandler(IInboxService inboxService, IMediator mediator)
    {
        _inboxService = inboxService;
        _mediator = mediator;
    }

    public async Task<string> Handle(CreateInboxItemCommand request, CancellationToken cancellationToken)
    {
        var inboxItem = new InboxItem
        {
            Id = Guid.NewGuid().ToString(),
            Subject = request.Subject,
            Sender = request.Sender,
            DocumentId = request.DocumentId ?? string.Empty,
            Priority = request.Priority,
            AssignedTo = request.AssignedTo ?? string.Empty,
            Notes = request.Notes ?? string.Empty,
            ReceivedAt = DateTime.UtcNow,
            Status = InboxStatus.New,
            IsRead = false
        };

        var createdItem = await _inboxService.CreateInboxItemAsync(inboxItem);
        
        return createdItem.Id;
    }
}
