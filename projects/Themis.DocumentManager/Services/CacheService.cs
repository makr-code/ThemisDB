/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CacheService.cs                                    ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     393                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
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

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services;

#nullable enable

/// <summary>
/// Intelligent cache and buffer system for offline access and performance optimization
/// Similar to browser cache but with revision safety and integrity verification
/// </summary>
public interface ICacheService
{
    Task<T?> GetAsync<T>(string key, CancellationToken cancellationToken = default);
    Task SetAsync<T>(string key, T value, TimeSpan? ttl = null, CacheEntryPriority priority = CacheEntryPriority.Normal, CancellationToken cancellationToken = default);
    Task<bool> ExistsAsync(string key, CancellationToken cancellationToken = default);
    Task RemoveAsync(string key, CancellationToken cancellationToken = default);
    Task ClearAsync(CancellationToken cancellationToken = default);
    Task<CacheStatistics> GetStatisticsAsync(CancellationToken cancellationToken = default);
    Task<bool> VerifyIntegrityAsync(string key, CancellationToken cancellationToken = default);
    
    // Prefetching for intelligent caching
    Task PrefetchAsync(List<PrefetchRequest> requests, CancellationToken cancellationToken = default);
}

public class CacheService : ICacheService, IDisposable
{
    private readonly ILogger<CacheService> _logger;
    private readonly CacheConfiguration _config;
    private readonly ConcurrentDictionary<string, CacheEntry<object>> _cache;
    private readonly SemaphoreSlim _cleanupLock;
    private readonly Timer _cleanupTimer;
    private readonly CacheStatistics _statistics;
    private readonly string _cacheDirectory;
    
    public CacheService(ILogger<CacheService> logger, CacheConfiguration? config = null)
    {
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        _config = config ?? new CacheConfiguration();
        _cache = new ConcurrentDictionary<string, CacheEntry<object>>();
        _cleanupLock = new SemaphoreSlim(1, 1);
        _statistics = new CacheStatistics();
        
        // Setup cache directory
        _cacheDirectory = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "ThemisDB", "Cache"
        );
        Directory.CreateDirectory(_cacheDirectory);
        
        // Start cleanup timer
        _cleanupTimer = new Timer(
            _ => CleanupExpiredEntriesAsync().GetAwaiter().GetResult(),
            null,
            _config.CleanupInterval,
            _config.CleanupInterval
        );
        
        _logger.LogInformation("Cache service initialized. Max size: {MaxSize}MB, Max entries: {MaxEntries}",
            _config.MaxSizeInBytes / 1024 / 1024, _config.MaxEntries);
    }
    
    public async Task<T?> GetAsync<T>(string key, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(key);
        
        try
        {
            if (_cache.TryGetValue(key, out var entry))
            {
                // Check expiration
                if (DateTime.UtcNow > entry.ExpiresAt)
                {
                    _cache.TryRemove(key, out _);
                    _statistics.MissCount++;
                    return default;
                }
                
                // Verify integrity if enabled
                if (_config.ValidateIntegrity && !string.IsNullOrEmpty(entry.ContentHash))
                {
                    var isValid = await VerifyIntegrityAsync(key, cancellationToken);
                    if (!isValid)
                    {
                        _logger.LogWarning("Cache integrity check failed for key: {Key}", key);
                        _cache.TryRemove(key, out _);
                        _statistics.MissCount++;
                        return default;
                    }
                }
                
                // Update access statistics
                entry.AccessCount++;
                entry.LastAccessedAt = DateTime.UtcNow;
                
                _statistics.HitCount++;
                
                return entry.Value is T typedValue ? typedValue : default;
            }
            
            _statistics.MissCount++;
            return default;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error getting cache entry for key: {Key}", key);
            return default;
        }
    }
    
    public async Task SetAsync<T>(string key, T value, TimeSpan? ttl = null, CacheEntryPriority priority = CacheEntryPriority.Normal, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(key);
        ArgumentNullException.ThrowIfNull(value);
        
        try
        {
            var effectiveTtl = ttl ?? _config.DefaultTTL;
            var now = DateTime.UtcNow;
            
            // Calculate entry size
            var serialized = JsonSerializer.Serialize(value);
            var sizeInBytes = Encoding.UTF8.GetByteCount(serialized);
            
            // Calculate content hash for integrity verification
            string? contentHash = null;
            if (_config.ValidateIntegrity)
            {
                contentHash = CalculateHash(serialized);
            }
            
            var entry = new CacheEntry<object>
            {
                Key = key,
                Value = value,
                CachedAt = now,
                ExpiresAt = now.Add(effectiveTtl),
                SizeInBytes = sizeInBytes,
                AccessCount = 0,
                LastAccessedAt = now,
                Priority = priority,
                ContentHash = contentHash
            };
            
            // Check if we need to evict entries
            await EnsureCapacityAsync(sizeInBytes, cancellationToken);
            
            // Add or update entry
            _cache.AddOrUpdate(key, entry, (k, old) => entry);
            
            // Update statistics
            _statistics.TotalEntries++;
            _statistics.TotalSizeInBytes += sizeInBytes;
            
            _logger.LogDebug("Cached entry: {Key}, Size: {Size}KB, TTL: {TTL}",
                key, sizeInBytes / 1024, effectiveTtl);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error setting cache entry for key: {Key}", key);
            throw;
        }
    }
    
    public Task<bool> ExistsAsync(string key, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(key);
        
        if (_cache.TryGetValue(key, out var entry))
        {
            return Task.FromResult(DateTime.UtcNow <= entry.ExpiresAt);
        }
        
        return Task.FromResult(false);
    }
    
    public Task RemoveAsync(string key, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(key);
        
        if (_cache.TryRemove(key, out var entry))
        {
            _statistics.TotalEntries--;
            _statistics.TotalSizeInBytes -= entry.SizeInBytes;
            
            _logger.LogDebug("Removed cache entry: {Key}", key);
        }
        
        return Task.CompletedTask;
    }
    
    public Task ClearAsync(CancellationToken cancellationToken = default)
    {
        _cache.Clear();
        _statistics.TotalEntries = 0;
        _statistics.TotalSizeInBytes = 0;
        
        _logger.LogInformation("Cache cleared");
        
        return Task.CompletedTask;
    }
    
    public Task<CacheStatistics> GetStatisticsAsync(CancellationToken cancellationToken = default)
    {
        _statistics.TotalEntries = _cache.Count;
        _statistics.TotalSizeInBytes = _cache.Values.Sum(e => e.SizeInBytes);
        
        return Task.FromResult(_statistics);
    }
    
    public Task<bool> VerifyIntegrityAsync(string key, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(key);
        
        if (!_cache.TryGetValue(key, out var entry))
        {
            return Task.FromResult(false);
        }
        
        if (string.IsNullOrEmpty(entry.ContentHash))
        {
            return Task.FromResult(true); // No hash to verify
        }
        
        try
        {
            var serialized = JsonSerializer.Serialize(entry.Value);
            var currentHash = CalculateHash(serialized);
            
            return Task.FromResult(currentHash == entry.ContentHash);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error verifying cache integrity for key: {Key}", key);
            return Task.FromResult(false);
        }
    }
    
    public async Task PrefetchAsync(List<PrefetchRequest> requests, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(requests);
        
        try
        {
            foreach (var request in requests)
            {
                // TODO: Implement intelligent prefetching based on usage patterns
                _logger.LogDebug("Prefetch request for {EntityId} (Type: {Type})",
                    request.EntityId, request.Type);
                
                // This would fetch the entity and related entities from ThemisDB
                // and cache them proactively
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error prefetching cache entries");
        }
    }
    
    private async Task EnsureCapacityAsync(long newEntrySize, CancellationToken cancellationToken)
    {
        var currentSize = _cache.Values.Sum(e => e.SizeInBytes);
        
        // Check if we need to evict entries
        if (currentSize + newEntrySize > _config.MaxSizeInBytes || _cache.Count >= _config.MaxEntries)
        {
            await EvictEntriesAsync(newEntrySize, cancellationToken);
        }
    }
    
    private async Task EvictEntriesAsync(long requiredSpace, CancellationToken cancellationToken)
    {
        var freedSpace = 0L;
        var entriesToRemove = new List<string>();
        
        // Apply eviction policy
        IEnumerable<KeyValuePair<string, CacheEntry<object>>> candidates;
        
        switch (_config.EvictionPolicy)
        {
            case CacheEvictionPolicy.LRU:
                candidates = _cache.OrderBy(e => e.Value.LastAccessedAt);
                break;
            case CacheEvictionPolicy.LFU:
                candidates = _cache.OrderBy(e => e.Value.AccessCount);
                break;
            case CacheEvictionPolicy.FIFO:
                candidates = _cache.OrderBy(e => e.Value.CachedAt);
                break;
            default:
                candidates = _cache.OrderBy(e => e.Value.ExpiresAt);
                break;
        }
        
        // Skip high priority entries in initial pass
        foreach (var entry in candidates.Where(e => e.Value.Priority != CacheEntryPriority.Critical))
        {
            if (freedSpace >= requiredSpace)
                break;
            
            entriesToRemove.Add(entry.Key);
            freedSpace += entry.Value.SizeInBytes;
        }
        
        // Remove selected entries
        foreach (var key in entriesToRemove)
        {
            await RemoveAsync(key, cancellationToken);
            _statistics.EvictionCount++;
        }
        
        _logger.LogInformation("Evicted {Count} cache entries, freed {Size}MB",
            entriesToRemove.Count, freedSpace / 1024 / 1024);
    }
    
    private async Task CleanupExpiredEntriesAsync()
    {
        await _cleanupLock.WaitAsync();
        try
        {
            var now = DateTime.UtcNow;
            var expiredKeys = _cache.Where(e => e.Value.ExpiresAt < now)
                                   .Select(e => e.Key)
                                   .ToList();
            
            foreach (var key in expiredKeys)
            {
                await RemoveAsync(key);
            }
            
            _statistics.LastCleanupAt = now;
            
            if (expiredKeys.Count > 0)
            {
                _logger.LogInformation("Cleanup removed {Count} expired cache entries", expiredKeys.Count);
            }
        }
        finally
        {
            _cleanupLock.Release();
        }
    }
    
    private string CalculateHash(string content)
    {
        using var sha256 = SHA256.Create();
        var hashBytes = sha256.ComputeHash(Encoding.UTF8.GetBytes(content));
        return Convert.ToBase64String(hashBytes);
    }
    
    public void Dispose()
    {
        _cleanupTimer?.Dispose();
        _cleanupLock?.Dispose();
    }
}
