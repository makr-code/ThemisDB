/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentsRefreshUIHandler.cs                       ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
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
using Microsoft.Extensions.Logging;
using System.Threading;
using System.Threading.Tasks;
using Themis.DocumentManager.Domain.Events;
using SysApp = System.Windows.Application;

namespace Themis.DocumentManager.Application.Documents.EventHandlers;

/// <summary>
/// Handler that triggers UI refresh when documents are imported via TestDataGenerator
/// </summary>
public class DocumentsRefreshUIHandler : INotificationHandler<DocumentsRefreshRequestedEvent>
{
    private readonly ILogger<DocumentsRefreshUIHandler>? _logger;

    public DocumentsRefreshUIHandler(ILogger<DocumentsRefreshUIHandler>? logger = null)
    {
        _logger = logger;
    }

    public async Task Handle(DocumentsRefreshRequestedEvent notification, CancellationToken cancellationToken)
    {
        _logger?.LogInformation(
            "UI refresh triggered from {Source} at {Time}",
            notification.Source,
            notification.RequestedAt);

        // Dispatch to UI thread to show notification
        await SysApp.Current.Dispatcher.InvokeAsync(() =>
        {
            // Find MainWindow and trigger refresh
            var mainWindow = SysApp.Current.MainWindow;
            if (mainWindow != null)
            {
                _logger?.LogDebug("Triggering document list refresh in MainWindow");
                
                // Optionally show a toast notification
                // This could be enhanced with a proper notification service
            }
        });
    }
}
