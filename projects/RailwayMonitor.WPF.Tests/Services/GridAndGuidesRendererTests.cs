/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GridAndGuidesRendererTests.cs                      ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     124                                            ║
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

using FluentAssertions;
using Xunit;
using RailwayMonitor.WPF.Services;

namespace RailwayMonitor.WPF.Tests.Services;

public class GridAndGuidesRendererTests
{
    private readonly GridAndGuidesRenderer _service;

    public GridAndGuidesRendererTests()
    {
        _service = new GridAndGuidesRenderer();
    }

    [Theory]
    [InlineData("Cartesian")]
    [InlineData("Polar")]
    [InlineData("Geographic")]
    [InlineData("UTM")]
    public void GenerateGrid_ShouldSupport_AllGridTypes(string gridType)
    {
        // Arrange
        var cellSize = 1000.0; // 1km

        // Act
        var grid = _service.GenerateGrid(gridType, cellSize);

        // Assert
        grid.Should().NotBeNull();
        grid.Lines.Should().NotBeEmpty();
    }

    [Fact]
    public void Generate3DAxes_ShouldCreate_XYZAxes()
    {
        // Arrange
        var axisLength = 10000.0; // 10km

        // Act
        var axes = _service.Generate3DAxes(axisLength);

        // Assert
        axes.Should().HaveCount(3); // X, Y, Z
        axes.Should().Contain(a => a.Color == "Red");   // X = East
        axes.Should().Contain(a => a.Color == "Green"); // Y = Height
        axes.Should().Contain(a => a.Color == "Blue");  // Z = North
    }

    [Theory]
    [InlineData("Distance")]
    [InlineData("Elevation")]
    [InlineData("Gradient")]
    public void CreateMeasurementTool_ShouldSupport_RulerTypes(string toolType)
    {
        // Arrange
        var start = new { X = 0.0, Y = 0.0, Z = 0.0 };
        var end = new { X = 1000.0, Y = 100.0, Z = 500.0 };

        // Act
        var tool = _service.CreateMeasurementTool(toolType, start, end);

        // Assert
        tool.Should().NotBeNull();
        tool.Type.Should().Be(toolType);
    }

    [Fact]
    public void DynamicCentering_ShouldFollow_CameraPosition()
    {
        // Arrange
        var cameraPos = new { X = 5000.0, Y = 0.0, Z = 5000.0 };

        // Act
        _service.UpdateCameraPosition(cameraPos.X, cameraPos.Y, cameraPos.Z);
        var grid = _service.GetCurrentGrid();

        // Assert
        grid.CenterX.Should().BeApproximately(cameraPos.X, 1000.0);
        grid.CenterZ.Should().BeApproximately(cameraPos.Z, 1000.0);
    }

    [Fact]
    public void DistanceFade_ShouldReduce_AlphaAtDistance()
    {
        // Arrange
        var nearDistance = 1000.0;  // 1km - full visibility
        var farDistance = 45000.0;  // 45km - faded

        // Act
        var nearAlpha = _service.CalculateFadeAlpha(nearDistance);
        var farAlpha = _service.CalculateFadeAlpha(farDistance);

        // Assert
        nearAlpha.Should().BeGreaterThan(0.9); // Nearly opaque
        farAlpha.Should().BeLessThan(0.3);    // Mostly transparent
    }
}
