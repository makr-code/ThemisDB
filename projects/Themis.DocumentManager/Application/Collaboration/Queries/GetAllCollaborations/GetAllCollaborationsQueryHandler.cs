/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllCollaborationsQueryHandler.cs                ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Collaboration.Messages;
using Themis.DocumentManager.Application.Collaboration.Commands.CreateCollaboration;

namespace Themis.DocumentManager.Application.Collaboration.Queries.GetAllCollaborations;

public class GetAllCollaborationsQueryHandler : IRequestHandler<GetAllCollaborationsQuery, Result<PagedResult<CollaborationDto>>>
{
    private static readonly Dictionary<string, CollaborationItem> _collaborations = new();

    public async Task<Result<PagedResult<CollaborationDto>>> Handle(GetAllCollaborationsQuery request, CancellationToken cancellationToken)
    {
        try
        {
            var filtered = _collaborations.Values.AsEnumerable();

            // SearchTerm filter
            if (!string.IsNullOrWhiteSpace(request.SearchTerm))
            {
                filtered = filtered.Where(c =>
                    c.UserName.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase) ||
                    (c.UserEmail != null && c.UserEmail.Contains(request.SearchTerm, StringComparison.OrdinalIgnoreCase)));
            }

            // EntityId filter
            if (!string.IsNullOrEmpty(request.EntityId))
            {
                filtered = filtered.Where(c => c.EntityId == request.EntityId);
            }

            // EntityType filter
            if (request.EntityType.HasValue)
            {
                filtered = filtered.Where(c => c.EntityType == request.EntityType.Value);
            }

            // UserId filter
            if (!string.IsNullOrEmpty(request.UserId))
            {
                filtered = filtered.Where(c => c.UserId == request.UserId);
            }

            // Role filter
            if (request.Role.HasValue)
            {
                filtered = filtered.Where(c => c.Role == request.Role.Value);
            }

            // IsActive filter
            if (request.IsActive.HasValue)
            {
                filtered = filtered.Where(c => c.IsActive == request.IsActive.Value);
            }

            var totalCount = filtered.Count();
            var skip = (request.PageNumber - 1) * request.PageSize;
            var items = filtered
                .OrderByDescending(c => c.CreatedAt)
                .Skip(skip)
                .Take(request.PageSize)
                .ToList();

            var dtos = items.Select(collaboration => new CollaborationDto
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
            }).ToList();

            var pagedResult = new PagedResult<CollaborationDto>
            {
                Items = dtos,
                TotalCount = totalCount,
                PageNumber = request.PageNumber,
                PageSize = request.PageSize
            };

            return await Task.FromResult(Result<PagedResult<CollaborationDto>>.Ok(pagedResult));
        }
        catch (Exception ex)
        {
            return Result<PagedResult<CollaborationDto>>.Fail($"Fehler beim Abrufen der Collaborations: {ex.Message}");
        }
    }
}
