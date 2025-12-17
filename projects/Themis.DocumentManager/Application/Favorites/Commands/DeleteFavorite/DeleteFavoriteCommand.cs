using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;

namespace Themis.DocumentManager.Application.Favorites.Commands.DeleteFavorite;

public record DeleteFavoriteCommand : IDeleteCommand
{
    public string Id { get; init; } = string.Empty;
}
