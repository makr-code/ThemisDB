/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentsRefreshRequestedEventHandler.cs           ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
