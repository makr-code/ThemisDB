/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateCollaborationCommand.cs                      ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     38                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;
using Themis.DocumentManager.Application.Collaboration.Messages;

namespace Themis.DocumentManager.Application.Collaboration.Commands.CreateCollaboration;

public record CreateCollaborationCommand : ICreateCommand<CollaborationDto>
{
    public string EntityId { get; init; } = string.Empty;
    public CollaborationEntityType EntityType { get; init; }
    public string UserId { get; init; } = string.Empty;
    public string UserName { get; init; } = string.Empty;
    public string? UserEmail { get; init; }
    public CollaborationRole Role { get; init; } = CollaborationRole.Viewer;
    public CollaborationPermissions Permissions { get; init; } = CollaborationPermissions.Read;
    public DateTime? AccessExpiresAt { get; init; }
    public string? InvitedBy { get; init; }
    public Dictionary<string, object>? Metadata { get; init; }
}
