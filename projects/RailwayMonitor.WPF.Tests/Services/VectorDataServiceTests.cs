/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            VectorDataServiceTests.cs                          ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     137                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentAssertions;
using Xunit;
using RailwayMonitor.WPF.Services;

namespace RailwayMonitor.WPF.Tests.Services;

public class VectorDataServiceTests
{
    private readonly VectorDataService _service;

    public VectorDataServiceTests()
    {
        _service = new VectorDataService();
    }

    [Fact]
    public async Task LoadMVTTile_ShouldParse_VectorTileFormat()
    {
        // Arrange
        var tileX = 8520;
        var tileY = 5567;
        var zoom = 14;

        // Act
        var tile = await _service.LoadMVTTileAsync(tileX, tileY, zoom);

        // Assert
        tile.Should().NotBeNull();
        tile.Layers.Should().NotBeEmpty();
    }

    [Theory]
    [InlineData("RailwayTracks")]
    [InlineData("Stations")]
    [InlineData("Roads")]
    [InlineData("Buildings")]
    public void GetLayerConfiguration_ShouldHave_PreConfiguredLayers(string layerName)
    {
        // Act
        var config = _service.GetLayerConfiguration(layerName);

        // Assert
        config.Should().NotBeNull();
        config.Name.Should().Be(layerName);
    }

    [Fact]
    public void ApplyMapboxGLStyle_ShouldParse_StyleSheet()
    {
        // Arrange
        var styleJson = @"{
            ""version"": 8,
            ""sources"": {},
            ""layers"": []
        }";

        // Act
        var result = _service.ApplyMapboxGLStyle(styleJson);

        // Assert
        result.Should().BeTrue();
    }

    [Fact]
    public async Task CacheTile_ShouldStore_WithTTL()
    {
        // Arrange
        var tileKey = "14/8520/5567";
        var tileData = new byte[] { 1, 2, 3, 4 };
        var ttl = TimeSpan.FromDays(30);

        // Act
        await _service.CacheTileAsync(tileKey, tileData, ttl);

        // Assert
        var cached = await _service.GetCachedTileAsync(tileKey);
        cached.Should().NotBeNull();
        cached.Should().BeEquivalentTo(tileData);
    }

    [Fact]
    public async Task RefreshTiles_ShouldUpdate_ExpiredTiles()
    {
        // Arrange
        var expiredTiles = new[] { "14/8520/5567", "14/8520/5568" };

        // Act
        await _service.RefreshTilesAsync(expiredTiles);

        // Assert
        foreach (var tileKey in expiredTiles)
        {
            var tile = await _service.GetCachedTileAsync(tileKey);
            tile.Should().NotBeNull();
        }
    }

    [Fact]
    public void IsPostGISReady_ShouldCheckIntegration()
    {
        // Act
        var isReady = _service.IsPostGISReady();

        // Assert
        isReady.Should().BeTrue(); // When PostGIS is configured
    }

    [Fact]
    public void EnableGPUAcceleration_ShouldToggle_HardwareRendering()
    {
        // Act
        _service.EnableGPUAcceleration(true);

        // Assert
        var isEnabled = _service.IsGPUAccelerationEnabled();
        isEnabled.Should().BeTrue();
    }
}
