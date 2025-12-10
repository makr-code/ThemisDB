using MediatR;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Inbox.Commands.CreateInboxItem;

/// <summary>
/// Command to create a new inbox item (Posteingang)
/// Based on PDV VIS analysis requirements
/// </summary>
public record CreateInboxItemCommand : IRequest<string>
{
    public string Subject { get; init; } = string.Empty;
    public string Sender { get; init; } = string.Empty;
    public string? DocumentId { get; init; }
    public InboxPriority Priority { get; init; } = InboxPriority.Normal;
    public string? AssignedTo { get; init; }
    public string? Notes { get; init; }
}
