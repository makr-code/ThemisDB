/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCollaborationCommand.cs                      ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-03-02 03:55:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;
using Themis.DocumentManager.Application.Collaboration.Messages;

namespace Themis.DocumentManager.Application.Collaboration.Commands.UpdateCollaboration;

public record UpdateCollaborationCommand : IUpdateCommand
{
    public string Id { get; init; } = string.Empty;
    public CollaborationRole? Role { get; init; }
    public CollaborationPermissions? Permissions { get; init; }
    public DateTime? AccessExpiresAt { get; init; }
    public bool? IsActive { get; init; }
    public Dictionary<string, object>? Metadata { get; init; }
}
