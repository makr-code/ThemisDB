/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateCollaborationCommandHandler.cs               ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     111                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Collaboration.Messages;

namespace Themis.DocumentManager.Application.Collaboration.Commands.CreateCollaboration;

public class CreateCollaborationCommandHandler : IRequestHandler<CreateCollaborationCommand, Result<CollaborationDto>>
{
    private static readonly Dictionary<string, CollaborationItem> _collaborations = new();

    public async Task<Result<CollaborationDto>> Handle(CreateCollaborationCommand request, CancellationToken cancellationToken)
    {
        try
        {
            var collaboration = new CollaborationItem
            {
                Id = Guid.NewGuid().ToString(),
                EntityId = request.EntityId,
                EntityType = request.EntityType,
                UserId = request.UserId,
                UserName = request.UserName,
                UserEmail = request.UserEmail,
                Role = request.Role,
                Permissions = request.Permissions,
                AccessExpiresAt = request.AccessExpiresAt,
                IsActive = true,
                InvitedBy = request.InvitedBy,
                Metadata = request.Metadata ?? new Dictionary<string, object>(),
                CreatedAt = DateTime.UtcNow,
                CreatedBy = "System"
            };

            _collaborations[collaboration.Id] = collaboration;

            var dto = new CollaborationDto
            {
                Id = collaboration.Id,
                EntityId = collaboration.EntityId,
                EntityType = collaboration.EntityType,
                UserId = collaboration.UserId,
                UserName = collaboration.UserName,
                UserEmail = collaboration.UserEmail,
                Role = collaboration.Role,
                Permissions = collaboration.Permissions,
                AccessExpiresAt = collaboration.AccessExpiresAt,
                IsActive = collaboration.IsActive,
                InvitedBy = collaboration.InvitedBy,
                AcceptedAt = collaboration.AcceptedAt,
                Metadata = collaboration.Metadata,
                CreatedAt = collaboration.CreatedAt,
                CreatedBy = collaboration.CreatedBy ?? string.Empty,
                UpdatedAt = collaboration.ModifiedAt,
                UpdatedBy = collaboration.ModifiedBy ?? string.Empty
            };

            return await Task.FromResult(Result<CollaborationDto>.Ok(dto));
        }
        catch (Exception ex)
        {
            return Result<CollaborationDto>.Fail($"Fehler beim Erstellen der Collaboration: {ex.Message}");
        }
    }
}

internal class CollaborationItem
{
    public string Id { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public CollaborationEntityType EntityType { get; set; }
    public string UserId { get; set; } = string.Empty;
    public string UserName { get; set; } = string.Empty;
    public string? UserEmail { get; set; }
    public CollaborationRole Role { get; set; }
    public CollaborationPermissions Permissions { get; set; }
    public DateTime? AccessExpiresAt { get; set; }
    public bool IsActive { get; set; }
    public string? InvitedBy { get; set; }
    public DateTime? AcceptedAt { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
    public DateTime CreatedAt { get; set; }
    public string? CreatedBy { get; set; }
    public DateTime ModifiedAt { get; set; }
    public string? ModifiedBy { get; set; }
}
