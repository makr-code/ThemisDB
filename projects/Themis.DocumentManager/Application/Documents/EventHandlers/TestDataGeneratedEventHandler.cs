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
