/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateFavoriteCommandHandler.cs                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     59                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Favorites.Messages;

namespace Themis.DocumentManager.Application.Favorites.Commands.CreateFavorite;

public class CreateFavoriteCommandHandler : IRequestHandler<CreateFavoriteCommand, Result<FavoriteDto>>
{
    public Task<Result<FavoriteDto>> Handle(CreateFavoriteCommand request, CancellationToken cancellationToken)
    {
        try
        {
            var dto = new FavoriteDto
            {
                Id = Guid.NewGuid().ToString(),
                EntityId = request.EntityId,
                EntityType = request.EntityType,
                EntityTitle = request.EntityTitle,
                EntityDescription = request.EntityDescription,
                UserId = request.UserId,
                SortOrder = request.SortOrder,
                Metadata = request.Metadata,
                CreatedAt = DateTime.UtcNow,
                CreatedBy = request.UserId
            };

            return Task.FromResult(Result<FavoriteDto>.Ok(dto));
        }
        catch (Exception ex)
        {
            return Task.FromResult(Result<FavoriteDto>.Fail($"Fehler beim Erstellen des Favoriten: {ex.Message}"));
        }
    }
}
