/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CacheModels.cs                                     ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

#nullable enable

/// <summary>
/// Cache entry for documents and metadata
/// </summary>
public class CacheEntry<T>
{
    public string Key { get; set; } = string.Empty;
    public T? Value { get; set; }
    public DateTime CachedAt { get; set; }
    public DateTime ExpiresAt { get; set; }
    public long SizeInBytes { get; set; }
    public int AccessCount { get; set; }
    public DateTime LastAccessedAt { get; set; }
    public CacheEntryPriority Priority { get; set; }
    public string? ETag { get; set; }
    public string? ContentHash { get; set; } // SHA256 for verification
}

public enum CacheEntryPriority
{
    Low,
    Normal,
    High,
    Critical
}

/// <summary>
/// Cache configuration
/// </summary>
public class CacheConfiguration
{
    public long MaxSizeInBytes { get; set; } = 500 * 1024 * 1024; // 500MB default
    public int MaxEntries { get; set; } = 10000;
    public TimeSpan DefaultTTL { get; set; } = TimeSpan.FromHours(24);
    public TimeSpan CleanupInterval { get; set; } = TimeSpan.FromMinutes(5);
    public CacheEvictionPolicy EvictionPolicy { get; set; } = CacheEvictionPolicy.LRU;
    public bool EnableCompression { get; set; } = true;
    public bool EnableEncryption { get; set; } = true; // For sensitive data
    public bool ValidateIntegrity { get; set; } = true; // SHA256 verification
}

public enum CacheEvictionPolicy
{
    LRU,  // Least Recently Used
    LFU,  // Least Frequently Used
    FIFO, // First In First Out
    TTL   // Time To Live only
}

/// <summary>
/// Cache statistics
/// </summary>
public class CacheStatistics
{
    private long _hitCount;
    private long _missCount;
    
    public long TotalEntries { get; set; }
    public long TotalSizeInBytes { get; set; }
    public long HitCount 
    { 
        get => _hitCount;
        set => _hitCount = value;
    }
    public long MissCount 
    { 
        get => _missCount;
        set => _missCount = value;
    }
    public double HitRate => TotalRequests > 0 ? (double)HitCount / TotalRequests : 0;
    public long TotalRequests => HitCount + MissCount;
    public long EvictionCount { get; set; }
    public DateTime LastCleanupAt { get; set; }
}

/// <summary>
/// Document buffer for offline editing
/// </summary>
public class DocumentBuffer
{
    public string DocumentId { get; set; } = string.Empty;
    public string ProcessId { get; set; } = string.Empty;
    public byte[] Content { get; set; } = Array.Empty<byte>();
    public string ContentType { get; set; } = string.Empty;
    public DateTime BufferedAt { get; set; }
    public DateTime? LastModifiedAt { get; set; }
    public bool IsModified { get; set; }
    public bool IsSynced { get; set; }
    public string? ConflictResolution { get; set; }
    public Dictionary<string, object> Metadata { get; set; } = new();
}

/// <summary>
/// Prefetch request for intelligent caching
/// </summary>
public class PrefetchRequest
{
    public string EntityId { get; set; } = string.Empty;
    public PrefetchType Type { get; set; }
    public CacheEntryPriority Priority { get; set; } = CacheEntryPriority.Normal;
    public bool IncludeRelated { get; set; } = true;
    public int MaxRelatedDepth { get; set; } = 2;
}

public enum PrefetchType
{
    Document,
    Process,
    File,
    Timeline,
    Related
}
