/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentsRefreshRequestedEventHandler.cs           ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Domain.Events;

namespace Themis.DocumentManager.Application.Documents.EventHandlers;

/// <summary>
/// Handler for DocumentsRefreshRequestedEvent - triggers UI refresh
/// </summary>
public class DocumentsRefreshRequestedEventHandler : INotificationHandler<DocumentsRefreshRequestedEvent>
{
    private readonly ILogger<DocumentsRefreshRequestedEventHandler>? _logger;

    public DocumentsRefreshRequestedEventHandler(ILogger<DocumentsRefreshRequestedEventHandler>? logger = null)
    {
        _logger = logger;
    }

    public Task Handle(DocumentsRefreshRequestedEvent notification, CancellationToken cancellationToken)
    {
        _logger?.LogInformation(
            "Documents refresh requested from {Source} at {Time}",
            notification.Source,
            notification.RequestedAt);

        // The actual refresh will be handled by ViewModels subscribing to this event
        return Task.CompletedTask;
    }
}
