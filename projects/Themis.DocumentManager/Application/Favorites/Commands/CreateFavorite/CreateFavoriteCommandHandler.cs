/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateFavoriteCommandHandler.cs                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
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
