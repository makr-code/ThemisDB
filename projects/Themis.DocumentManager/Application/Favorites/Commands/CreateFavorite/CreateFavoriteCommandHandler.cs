/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateFavoriteCommandHandler.cs                    ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
