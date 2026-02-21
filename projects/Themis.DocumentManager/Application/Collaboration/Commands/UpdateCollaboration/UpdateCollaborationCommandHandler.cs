/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCollaborationCommandHandler.cs               ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     64                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Collaboration.Commands.CreateCollaboration;

namespace Themis.DocumentManager.Application.Collaboration.Commands.UpdateCollaboration;

public class UpdateCollaborationCommandHandler : IRequestHandler<UpdateCollaborationCommand, Result>
{
    private static readonly Dictionary<string, CollaborationItem> _collaborations = new();

    public async Task<Result> Handle(UpdateCollaborationCommand request, CancellationToken cancellationToken)
    {
        try
        {
            if (!_collaborations.ContainsKey(request.Id))
            {
                return Result.Fail($"Collaboration mit ID {request.Id} wurde nicht gefunden");
            }

            var collaboration = _collaborations[request.Id];

            if (request.Role.HasValue) collaboration.Role = request.Role.Value;
            if (request.Permissions.HasValue) collaboration.Permissions = request.Permissions.Value;
            if (request.AccessExpiresAt.HasValue) collaboration.AccessExpiresAt = request.AccessExpiresAt;
            if (request.IsActive.HasValue) collaboration.IsActive = request.IsActive.Value;
            if (request.Metadata != null) collaboration.Metadata = request.Metadata;

            collaboration.ModifiedAt = DateTime.UtcNow;

            return await Task.FromResult(Result.Ok());
        }
        catch (Exception ex)
        {
            return Result.Fail($"Fehler beim Aktualisieren der Collaboration: {ex.Message}");
        }
    }
}
