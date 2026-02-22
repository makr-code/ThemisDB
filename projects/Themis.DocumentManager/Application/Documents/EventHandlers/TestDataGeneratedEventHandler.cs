/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TestDataGeneratedEventHandler.cs                   ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
