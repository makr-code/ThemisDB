/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GeoSpatialAnalyzerTests.cs                         ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     113                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Xunit;
using FluentAssertions;
using RailwayMonitor.WPF.Services;
using System.Collections.Generic;
using System.Numerics;

namespace RailwayMonitor.WPF.Tests.Services
{
    public class GeoSpatialAnalyzerTests
    {
        private readonly GeoSpatialAnalyzer _analyzer;

        public GeoSpatialAnalyzerTests()
        {
            _analyzer = new GeoSpatialAnalyzer();
        }

        [Fact]
        public void FindPath_ShouldReturnPath_WhenStartAndEndAreValid()
        {
            // Arrange
            var start = new Vector2(50.0f, 8.0f); // Frankfurt area
            var end = new Vector2(52.5f, 13.4f);  // Berlin area

            // Act
            var path = _analyzer.FindPath(start, end, PathOptimization.Distance);

            // Assert
            path.Should().NotBeNull();
            path.Should().NotBeEmpty();
            path.First().Should().BeEquivalentTo(start);
        }

        [Theory]
        [InlineData(PathOptimization.Distance)]
        [InlineData(PathOptimization.Cost)]
        [InlineData(PathOptimization.Gradient)]
        [InlineData(PathOptimization.Terrain)]
        public void FindPath_ShouldWork_ForAllOptimizationModes(PathOptimization mode)
        {
            // Arrange
            var start = new Vector2(50.0f, 8.0f);
            var end = new Vector2(50.1f, 8.1f);

            // Act
            var path = _analyzer.FindPath(start, end, mode);

            // Assert
            path.Should().NotBeNull();
        }

        [Fact]
        public void CalculateTerrainCost_ShouldReturn_HigherCostForMountains()
        {
            // Arrange
            var flatLand = new Vector2(50.0f, 8.0f);
            var mountainous = new Vector2(47.5f, 11.0f); // Alps region

            // Act
            var flatCost = _analyzer.CalculateTerrainCost(flatLand);
            var mountainCost = _analyzer.CalculateTerrainCost(mountainous);

            // Assert
            mountainCost.Should().BeGreaterThan(flatCost);
        }

        [Fact]
        public void EstimateTotalCost_ShouldInclude_AllCostComponents()
        {
            // Arrange
            var path = new List<Vector2>
            {
                new Vector2(50.0f, 8.0f),
                new Vector2(50.5f, 8.5f),
                new Vector2(51.0f, 9.0f)
            };

            // Act
            var cost = _analyzer.EstimateTotalCost(path);

            // Assert
            cost.Should().BeGreaterThan(0);
            cost.LandAcquisition.Should().BeGreaterThan(0);
            cost.Construction.Should().BeGreaterThan(0);
        }
    }
}
