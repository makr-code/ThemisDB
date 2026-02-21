/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CollaborationDto.cs                                ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     70                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Collaboration.Messages;

public record CollaborationDto : BaseEntityDto
{
    public string EntityId { get; init; } = string.Empty;
    public CollaborationEntityType EntityType { get; init; }
    public string UserId { get; init; } = string.Empty;
    public string UserName { get; init; } = string.Empty;
    public string? UserEmail { get; init; }
    public CollaborationRole Role { get; init; }
    public CollaborationPermissions Permissions { get; init; }
    public DateTime? AccessExpiresAt { get; init; }
    public bool IsActive { get; init; }
    public string? InvitedBy { get; init; }
    public DateTime? AcceptedAt { get; init; }
    public Dictionary<string, object> Metadata { get; init; } = new();
}

public enum CollaborationEntityType
{
    Document,
    Process,
    Folder,
    Task
}

public enum CollaborationRole
{
    Viewer,
    Commenter,
    Editor,
    Owner
}

[Flags]
public enum CollaborationPermissions
{
    None = 0,
    Read = 1,
    Write = 2,
    Delete = 4,
    Share = 8,
    Admin = 16
}
