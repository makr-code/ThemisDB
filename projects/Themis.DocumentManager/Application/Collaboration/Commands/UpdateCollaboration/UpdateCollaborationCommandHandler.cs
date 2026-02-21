/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateCollaborationCommandHandler.cs               ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     64                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
