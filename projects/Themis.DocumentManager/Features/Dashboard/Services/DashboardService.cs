/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DashboardService.cs                                ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   88.0/100                                       ║
    • Total Lines:     459                                            ║
    • Open Issues:     TODOs: 4, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Features.Dashboard.Services;

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
            favorite.Order = favorite.Order == 0 ? (int)DateTimeOffset.UtcNow.ToUnixTimeSeconds() : favorite.Order;
            
            var query = @"
                INSERT { userId: @userId, type: 'favorite', data: @data }
                IN @@collection
                RETURN NEW.data
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = "current-user", // TODO: Get from context
                ["data"] = favorite
            };
            
            var cursor = await _db.QueryAsync<FavoriteItem>(query, bindVars, cancellationToken);
            return await cursor.FirstOrDefaultAsync(cancellationToken) ?? favorite;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error adding favorite {FavoriteId}", favorite.Id);
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
            var query = @"
                FOR item IN @@collection
                    FILTER item.type == 'favorite' AND item.data.id IN @orderedIds
                    LET pos = POSITION(@orderedIds, item.data.id)
                    UPDATE item WITH { 'data.order': pos } IN @@collection
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["orderedIds"] = orderedIds
            };
            
            await _db.ExecuteAsync(query, bindVars, cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error reordering favorites for ids {Ids}", string.Join(",", orderedIds));
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
                    FILTER item.userId == @userId AND item.type == 'quickAction'
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
            action.LastModifiedAt = DateTime.UtcNow;
            
            var query = @"
                UPSERT { userId: @userId, type: 'quickAction', data.id: @actionId }
                INSERT { userId: @userId, type: 'quickAction', data: @data, lastModifiedAt: @now }
                UPDATE { data: @data, lastModifiedAt: @now }
                IN @@collection
                RETURN NEW.data
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = "current-user", // TODO: Get from context
                ["actionId"] = action.Id,
                ["data"] = action,
                ["now"] = DateTime.UtcNow
            };
            
            var cursor = await _db.QueryAsync<QuickAction>(query, bindVars, cancellationToken);
            return await cursor.FirstOrDefaultAsync(cancellationToken) ?? action;
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
                    FILTER item.type == 'quickAction' AND item.data.id == @actionId
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
            var query = @"
                FOR widget IN @widgets
                    UPSERT { userId: @userId, type: 'widget', data.id: widget.id }
                    INSERT { userId: @userId, type: 'widget', data: widget, lastModifiedAt: @now }
                    UPDATE { data: widget, lastModifiedAt: @now }
                    IN @@collection
            ";
            
            var bindVars = new Dictionary<string, object>
            {
                ["@collection"] = _collectionName,
                ["userId"] = "current-user", // TODO: Get from context
                ["widgets"] = widgets,
                ["now"] = DateTime.UtcNow
            };
            
            await _db.ExecuteAsync(query, bindVars, cancellationToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error saving widget layout for user {UserId}", "current-user");
            throw;
        }
    }
}
