/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateInboxItemCommand.cs                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     18                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
