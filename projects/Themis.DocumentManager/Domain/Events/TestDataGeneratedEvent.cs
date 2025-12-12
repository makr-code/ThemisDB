using MediatR;

namespace Themis.DocumentManager.Domain.Events;

/// <summary>
/// Domain event raised when test data has been generated
/// </summary>
public record TestDataGeneratedEvent(int DocumentCount, DateTime GeneratedAt) : INotification;
