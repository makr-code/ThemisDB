using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

#nullable enable

/// <summary>
/// Service for managing dashboard features (recent items, favorites, quick actions)
/// </summary>
public interface IDashboardService
{
    // Recent items
    Task<List<RecentItem>> GetRecentItemsAsync(string userId, int count = 20, CancellationToken cancellationToken = default);
    Task AddRecentItemAsync(RecentItem item, CancellationToken cancellationToken = default);
    Task PinRecentItemAsync(string itemId, bool pinned, CancellationToken cancellationToken = default);
    Task ClearRecentItemsAsync(string userId, CancellationToken cancellationToken = default);
    
    // Favorites
    Task<List<FavoriteItem>> GetFavoritesAsync(string userId, CancellationToken cancellationToken = default);
    Task<FavoriteItem> AddFavoriteAsync(FavoriteItem favorite, CancellationToken cancellationToken = default);
    Task RemoveFavoriteAsync(string favoriteId, CancellationToken cancellationToken = default);
    Task ReorderFavoritesAsync(List<string> orderedIds, CancellationToken cancellationToken = default);
    
    // Quick actions
    Task<List<QuickAction>> GetQuickActionsAsync(string userId, CancellationToken cancellationToken = default);
    Task<QuickAction> SaveQuickActionAsync(QuickAction action, CancellationToken cancellationToken = default);
    Task RemoveQuickActionAsync(string actionId, CancellationToken cancellationToken = default);
    
    // Dashboard widgets
    Task<List<DashboardWidget>> GetWidgetsAsync(string userId, CancellationToken cancellationToken = default);
    Task SaveWidgetLayoutAsync(List<DashboardWidget> widgets, CancellationToken cancellationToken = default);
}

public class DashboardService : IDashboardService
{
    private readonly IThemisDbClient _db;
    private readonly ILogger<DashboardService> _logger;
    private readonly string _collectionName = "dashboard_data";
    
    public DashboardService(IThemisDbClient db, ILogger<DashboardService> logger)
    {
        _db = db ?? throw new ArgumentNullException(nameof(db));
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
    }
    
    public async Task<List<RecentItem>> GetRecentItemsAsync(string userId, int count = 20, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);
        
        try
        {
            var query = @"
                FOR item IN @@collection
                    FILTER item.userId == @userId AND item.type == 'recent'
                    SORT item.lastAccessedAt DESC
                    LIMIT @count
                    RETURN item.data
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = userId,
                ["count"] = count
            };
            
            var cursor = await _db.QueryAsync<RecentItem>(query, bindVars, cancellationToken);
            return await cursor.ToListAsync(cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting recent items for user {UserId}", userId);
            return new List<RecentItem>();
        }
    }
    
    public async Task AddRecentItemAsync(RecentItem item, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(item);
        
        try
        {
            item.LastAccessedAt = DateTime.UtcNow;
            
            // Upsert: update if exists, insert if not
            var query = @"
                UPSERT { userId: @userId, entityId: @entityId, type: 'recent' }
                INSERT { userId: @userId, entityId: @entityId, type: 'recent', data: @data, lastAccessedAt: @now }
                UPDATE { data: @data, lastAccessedAt: @now }
                IN @@collection
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = "current-user", // TODO: Get from context
                ["entityId"] = item.Id,
                ["data"] = item,
                ["now"] = DateTime.UtcNow
            };
            
            await _db.ExecuteAsync(query, bindVars, cancellationToken);
            
            _logger.LogInformation("Added recent item: {ItemName}", item.Name);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error adding recent item {ItemId}", item.Id);
            throw;
        }
    }
    
    public async Task PinRecentItemAsync(string itemId, bool pinned, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(itemId);
        
        try
        {
            var query = @"
                FOR item IN @@collection
                    FILTER item.entityId == @itemId AND item.type == 'recent'
                    UPDATE item WITH { 'data.isPinned': @pinned } IN @@collection
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["itemId"] = itemId,
                ["pinned"] = pinned
            };
            
            await _db.ExecuteAsync(query, bindVars, cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error pinning recent item {ItemId}", itemId);
            throw;
        }
    }
    
    public async Task ClearRecentItemsAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);
        
        try
        {
            var query = @"
                FOR item IN @@collection
                    FILTER item.userId == @userId AND item.type == 'recent' AND item.data.isPinned != true
                    REMOVE item IN @@collection
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = userId
            };
            
            await _db.ExecuteAsync(query, bindVars, cancellationToken);
            
            _logger.LogInformation("Cleared recent items for user {UserId}", userId);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error clearing recent items for user {UserId}", userId);
            throw;
        }
    }
    
    public async Task<List<FavoriteItem>> GetFavoritesAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);
        
        try
        {
            var query = @"
                FOR item IN @@collection
                    FILTER item.userId == @userId AND item.type == 'favorite'
                    SORT item.data.order ASC
                    RETURN item.data
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = userId
            };
            
            var cursor = await _db.QueryAsync<FavoriteItem>(query, bindVars, cancellationToken);
            return await cursor.ToListAsync(cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting favorites for user {UserId}", userId);
            return new List<FavoriteItem>();
        }
    }
    
    public async Task<FavoriteItem> AddFavoriteAsync(FavoriteItem favorite, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(favorite);
        
        try
        {
            favorite.Id = Guid.NewGuid().ToString();
            favorite.CreatedAt = DateTime.UtcNow;
            
            var doc = new
            {
                userId = "current-user",
                type = "favorite",
                data = favorite
            };
            
            await _db.InsertAsync(_collectionName, doc, cancellationToken);
            
            _logger.LogInformation("Added favorite: {FavoriteName}", favorite.Name);
            return favorite;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error adding favorite {FavoriteName}", favorite.Name);
            throw;
        }
    }
    
    public async Task RemoveFavoriteAsync(string favoriteId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(favoriteId);
        
        try
        {
            var query = @"
                FOR item IN @@collection
                    FILTER item.data.id == @favoriteId AND item.type == 'favorite'
                    REMOVE item IN @@collection
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["favoriteId"] = favoriteId
            };
            
            await _db.ExecuteAsync(query, bindVars, cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error removing favorite {FavoriteId}", favoriteId);
            throw;
        }
    }
    
    public async Task ReorderFavoritesAsync(List<string> orderedIds, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(orderedIds);
        
        try
        {
            for (int i = 0; i < orderedIds.Count; i++)
            {
                var query = @"
                    FOR item IN @@collection
                        FILTER item.data.id == @favoriteId AND item.type == 'favorite'
                        UPDATE item WITH { 'data.order': @order } IN @@collection
                ";
                
                var bindVars = new Dictionary<string, object>
                {
                    ["@collection"] = _collectionName,
                    ["favoriteId"] = orderedIds[i],
                    ["order"] = i
                };
                
                await _db.ExecuteAsync(query, bindVars, cancellationToken);
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error reordering favorites");
            throw;
        }
    }
    
    public async Task<List<QuickAction>> GetQuickActionsAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);
        
        try
        {
            var query = @"
                FOR item IN @@collection
                    FILTER item.userId == @userId AND item.type == 'quickaction'
                    FILTER item.data.isEnabled == true
                    SORT item.data.order ASC
                    RETURN item.data
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = userId
            };
            
            var cursor = await _db.QueryAsync<QuickAction>(query, bindVars, cancellationToken);
            return await cursor.ToListAsync(cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting quick actions for user {UserId}", userId);
            return new List<QuickAction>();
        }
    }
    
    public async Task<QuickAction> SaveQuickActionAsync(QuickAction action, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(action);
        
        try
        {
            if (string.IsNullOrEmpty(action.Id))
            {
                action.Id = Guid.NewGuid().ToString();
            }
            
            var query = @"
                UPSERT { userId: @userId, actionId: @actionId, type: 'quickaction' }
                INSERT { userId: @userId, actionId: @actionId, type: 'quickaction', data: @data }
                UPDATE { data: @data }
                IN @@collection
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = "current-user",
                ["actionId"] = action.Id,
                ["data"] = action
            };
            
            await _db.ExecuteAsync(query, bindVars, cancellationToken);
            
            return action;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error saving quick action {ActionId}", action.Id);
            throw;
        }
    }
    
    public async Task RemoveQuickActionAsync(string actionId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(actionId);
        
        try
        {
            var query = @"
                FOR item IN @@collection
                    FILTER item.actionId == @actionId AND item.type == 'quickaction'
                    REMOVE item IN @@collection
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["actionId"] = actionId
            };
            
            await _db.ExecuteAsync(query, bindVars, cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error removing quick action {ActionId}", actionId);
            throw;
        }
    }
    
    public async Task<List<DashboardWidget>> GetWidgetsAsync(string userId, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(userId);
        
        try
        {
            var query = @"
                FOR item IN @@collection
                    FILTER item.userId == @userId AND item.type == 'widget'
                    SORT item.data.row ASC, item.data.column ASC
                    RETURN item.data
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = userId
            };
            
            var cursor = await _db.QueryAsync<DashboardWidget>(query, bindVars, cancellationToken);
            return await cursor.ToListAsync(cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting widgets for user {UserId}", userId);
            return new List<DashboardWidget>();
        }
    }
    
    public async Task SaveWidgetLayoutAsync(List<DashboardWidget> widgets, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(widgets);
        
        try
        {
            // Delete existing widgets
            var deleteQuery = @"
                FOR item IN @@collection
                    FILTER item.userId == @userId AND item.type == 'widget'
                    REMOVE item IN @@collection
            ";
            
            await _db.ExecuteAsync(deleteQuery, new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = "current-user"
            }, cancellationToken);
            
            // Insert new widgets
            foreach (var widget in widgets)
            {
                var doc = new
                {
                    userId = "current-user",
                    type = "widget",
                    data = widget
                };
                
                await _db.InsertAsync(_collectionName, doc, cancellationToken);
            }
            
            _logger.LogInformation("Saved dashboard layout with {Count} widgets", widgets.Count);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error saving widget layout");
            throw;
        }
    }
}
