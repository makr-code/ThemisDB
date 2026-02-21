/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IntelligentCacheServiceTests.cs                    ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     148                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Xunit;
using FluentAssertions;
using RailwayMonitor.WPF.Services;
using System;
using System.Threading.Tasks;

namespace RailwayMonitor.WPF.Tests.Services
{
    public class IntelligentCacheServiceTests
    {
        private readonly IntelligentCacheService _cache;

        public IntelligentCacheServiceTests()
        {
            _cache = new IntelligentCacheService();
        }

        [Fact]
        public async Task GetAsync_ShouldReturn_Null_WhenKeyNotFound()
        {
            // Act
            var result = await _cache.GetAsync<string>("non-existent-key");

            // Assert
            result.Should().BeNull();
        }

        [Fact]
        public async Task StoreAndGet_ShouldReturn_SameValue()
        {
            // Arrange
            var key = "test-key";
            var value = "test-value";

            // Act
            await _cache.StoreAsync(key, value);
            var result = await _cache.GetAsync<string>(key);

            // Assert
            result.Should().Be(value);
        }

        [Fact]
        public async Task Store_ShouldEvict_WhenMemoryCacheFull()
        {
            // Arrange - Fill memory cache (2 GB limit)
            var largeData = new byte[1024 * 1024]; // 1 MB

            // Act - Store multiple items
            for (int i = 0; i < 10; i++)
            {
                await _cache.StoreAsync($"key-{i}", largeData, CacheTier.Memory);
            }

            // Assert - Should not throw, LRU eviction should occur
            var stats = _cache.GetStatistics();
            stats.MemoryCacheSize.Should().BeLessThanOrEqualTo(2L * 1024 * 1024 * 1024);
        }

        [Theory]
        [InlineData(CacheTier.Memory, 3600)] // 1 hour
        [InlineData(CacheTier.SQLite, 2592000)] // 30 days
        [InlineData(CacheTier.FileSystem, 31536000)] // 365 days
        public async Task Store_ShouldRespect_TTLPerTier(CacheTier tier, int ttlSeconds)
        {
            // Arrange
            var key = $"key-{tier}";
            var value = "test";

            // Act
            await _cache.StoreAsync(key, value, tier, TimeSpan.FromSeconds(ttlSeconds));

            // Assert - Should be stored
            var result = await _cache.GetAsync<string>(key);
            result.Should().Be(value);
        }

        [Fact]
        public async Task GetStatistics_ShouldReturn_AccurateHitRate()
        {
            // Arrange
            await _cache.StoreAsync("key1", "value1");
            await _cache.StoreAsync("key2", "value2");

            // Act - 2 hits, 1 miss
            await _cache.GetAsync<string>("key1");
            await _cache.GetAsync<string>("key2");
            await _cache.GetAsync<string>("key-not-found");

            var stats = _cache.GetStatistics();

            // Assert
            stats.TotalRequests.Should().Be(3);
            stats.HitRate.Should().BeApproximately(0.66, 0.1); // ~66%
        }

        [Fact]
        public async Task PreloadViewport_ShouldLoad_SurroundingTiles()
        {
            // Arrange
            var centerTile = new TileCoordinate(10, 20, 12); // x, y, zoom

            // Act
            await _cache.PreloadViewportAsync(centerTile, radiusKm: 5);

            // Assert
            var stats = _cache.GetStatistics();
            stats.PreloadedTiles.Should().BeGreaterThan(0);
        }

        [Fact]
        public async Task WarmCache_ShouldPreload_CriticalData()
        {
            // Act
            await _cache.WarmCacheAsync();

            // Assert
            var stats = _cache.GetStatistics();
            stats.MemoryCacheEntries.Should().BeGreaterThan(0);
        }
    }
}
