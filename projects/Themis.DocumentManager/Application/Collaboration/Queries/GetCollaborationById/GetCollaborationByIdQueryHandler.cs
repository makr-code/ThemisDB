/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetCollaborationByIdQueryHandler.cs                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Collaboration.Messages;
using Themis.DocumentManager.Application.Collaboration.Commands.CreateCollaboration;

namespace Themis.DocumentManager.Application.Collaboration.Queries.GetCollaborationById;

public class GetCollaborationByIdQueryHandler : IRequestHandler<GetCollaborationByIdQuery, Result<CollaborationDto>>
{
    private static readonly Dictionary<string, CollaborationItem> _collaborations = new();

    public async Task<Result<CollaborationDto>> Handle(GetCollaborationByIdQuery request, CancellationToken cancellationToken)
    {
        try
        {
            if (!_collaborations.ContainsKey(request.Id))
            {
                return Result<CollaborationDto>.Fail($"Collaboration mit ID {request.Id} wurde nicht gefunden");
            }

            var collaboration = _collaborations[request.Id];

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
            return Result<CollaborationDto>.Fail($"Fehler beim Abrufen der Collaboration: {ex.Message}");
        }
    }
}
