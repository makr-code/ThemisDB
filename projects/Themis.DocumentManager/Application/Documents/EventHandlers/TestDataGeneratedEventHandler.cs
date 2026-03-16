/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TestDataGeneratedEventHandler.cs                   ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:12:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Domain.Events;
using Microsoft.Extensions.Logging;

namespace Themis.DocumentManager.Application.Documents.EventHandlers;

/// <summary>
/// Handler for TestDataGeneratedEvent - logs the event
/// </summary>
public class TestDataGeneratedEventHandler : INotificationHandler<TestDataGeneratedEvent>
{
    private readonly ILogger<TestDataGeneratedEventHandler>? _logger;

    public TestDataGeneratedEventHandler(ILogger<TestDataGeneratedEventHandler>? logger = null)
    {
        _logger = logger;
    }

    public Task Handle(TestDataGeneratedEvent notification, CancellationToken cancellationToken)
    {
        _logger?.LogInformation(
            "Test data generated: {DocumentCount} documents at {GeneratedAt}",
            notification.DocumentCount,
            notification.GeneratedAt);

        return Task.CompletedTask;
    }
}
