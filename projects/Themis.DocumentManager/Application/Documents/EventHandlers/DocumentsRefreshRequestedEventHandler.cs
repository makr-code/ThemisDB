/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentsRefreshRequestedEventHandler.cs           ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:56:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
