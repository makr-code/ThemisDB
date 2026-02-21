/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentsRefreshRequestedEventHandler.cs           ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 362722340  2025-12-12  chore: workspace reorganization and build system consolid... ║
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
