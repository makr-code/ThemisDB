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
