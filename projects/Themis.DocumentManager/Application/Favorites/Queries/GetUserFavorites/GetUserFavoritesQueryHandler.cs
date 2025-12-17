using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Favorites.Messages;

namespace Themis.DocumentManager.Application.Favorites.Queries.GetUserFavorites;

public class GetUserFavoritesQueryHandler : IRequestHandler<GetUserFavoritesQuery, Result<PagedResult<FavoriteDto>>>
{
    public Task<Result<PagedResult<FavoriteDto>>> Handle(GetUserFavoritesQuery request, CancellationToken cancellationToken)
    {
        try
        {
            // In-memory: Would query favorites here
            var favorites = new List<FavoriteDto>();
            
            var pagedResult = new PagedResult<FavoriteDto>
            {
                Items = favorites,
                TotalCount = 0,
                PageNumber = request.PageNumber,
                PageSize = request.PageSize
            };

            return Task.FromResult(Result<PagedResult<FavoriteDto>>.Ok(pagedResult));
        }
        catch (Exception ex)
        {
            return Task.FromResult(Result<PagedResult<FavoriteDto>>.Fail($"Fehler beim Abrufen der Favoriten: {ex.Message}"));
        }
    }
}
