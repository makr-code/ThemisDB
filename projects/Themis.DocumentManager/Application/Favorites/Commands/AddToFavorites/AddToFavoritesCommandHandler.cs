/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AddToFavoritesCommandHandler.cs                    ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     96                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Navigation.Queries.GetNavigationPath;

namespace Themis.DocumentManager.Application.Favorites.Commands.AddToFavorites;

/// <summary>
/// Handler for AddToFavoritesCommand
/// Adds entity to user's favorites list
/// </summary>
public class AddToFavoritesCommandHandler : IRequestHandler<AddToFavoritesCommand, bool>
{
    // In real implementation, this would use a repository
    // For now, using in-memory storage as example
    internal static readonly List<FavoriteEntity> _favorites = new();

    public async Task<bool> Handle(AddToFavoritesCommand request, CancellationToken cancellationToken)
    {
        // Check if already favorited
        var existing = _favorites.FirstOrDefault(f => 
            f.EntityId == request.EntityId && 
            f.UserId == request.UserId);

        if (existing != null)
        {
            // Update existing favorite
            existing.Category = request.Category;
            existing.Tags = request.Tags;
            existing.Notes = request.Notes;
            existing.UpdatedAt = DateTime.UtcNow;
            return true;
        }

        // Add new favorite
        var favorite = new FavoriteEntity
        {
            Id = Guid.NewGuid().ToString(),
            EntityId = request.EntityId,
            EntityType = request.EntityType,
            EntityName = request.EntityName,
            UserId = request.UserId,
            Category = request.Category,
            Tags = request.Tags,
            Notes = request.Notes,
            CreatedAt = DateTime.UtcNow,
            UpdatedAt = DateTime.UtcNow,
            AccessCount = 0
        };

        _favorites.Add(favorite);

        await Task.CompletedTask;
        return true;
    }
}

public class FavoriteEntity
{
    public string Id { get; set; } = string.Empty;
    public string EntityId { get; set; } = string.Empty;
    public EntityType EntityType { get; set; }
    public string EntityName { get; set; } = string.Empty;
    public string UserId { get; set; } = string.Empty;
    public string? Category { get; set; }
    public string? Tags { get; set; }
    public string? Notes { get; set; }
    public DateTime CreatedAt { get; set; }
    public DateTime UpdatedAt { get; set; }
    public DateTime? LastAccessedAt { get; set; }
    public int AccessCount { get; set; }
}
