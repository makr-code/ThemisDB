using MediatR;

namespace Themis.DocumentManager.Domain.Events;

/// <summary>
/// Domain event raised when a document is created
/// </summary>
public record DocumentCreatedEvent(string DocumentId, string Title, DateTime CreatedAt) : INotification;
