/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            EnergyManagementServiceTests.cs                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     168                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using RailwayMonitor.WPF.Services;
using RailwayMonitor.WPF.Models;

namespace RailwayMonitor.WPF.Tests.Services;

/// <summary>
/// Unit tests for EnergyManagementService - Power management and optimization
/// </summary>
public class EnergyManagementServiceTests
{
    private readonly EnergyManagementService _service;

    public EnergyManagementServiceTests()
    {
        _service = new EnergyManagementService();
    }

    [Fact]
    public void CalculateTrainPower_ShouldReturnCorrectPower_ForICETrain()
    {
        // Arrange
        var train = new Train
        {
            Number = "ICE 123",
            CurrentSpeed = 250, // km/h
            MaxSpeed = 300,
            Weight = 400 // tons
        };

        // Act
        var power = _service.CalculateTrainPower(train);

        // Assert
        power.Should().BeGreaterThan(0);
        power.Should().BeLessThan(10000); // Reasonable power consumption in kW
    }

    [Theory]
    [InlineData(0, 100)] // Stopped, only auxiliary
    [InlineData(100, 1000)] // Medium speed
    [InlineData(250, 5000)] // High speed (non-linear increase)
    public void CalculateTrainPower_ShouldScaleWithSpeed(double speed, double expectedMinPower)
    {
        // Arrange
        var train = new Train
        {
            Number = "TEST",
            CurrentSpeed = speed,
            MaxSpeed = 300,
            Weight = 400
        };

        // Act
        var power = _service.CalculateTrainPower(train);

        // Assert
        power.Should().BeGreaterThanOrEqualTo(expectedMinPower);
    }

    [Fact]
    public void OptimizeDispatch_ForCost_ShouldPrioritizeCheapestSources()
    {
        // Arrange
        var powerSources = new List<PowerSource>
        {
            new() { Name = "Coal", Cost = 50, CO2 = 100, Capacity = 1000, Available = 1000 },
            new() { Name = "Solar", Cost = 10, CO2 = 0, Capacity = 500, Available = 500 },
            new() { Name = "Wind", Cost = 15, CO2 = 0, Capacity = 300, Available = 300 }
        };
        var demand = 800.0;

        // Act
        var result = _service.OptimizeDispatch(powerSources, demand, OptimizationGoal.MinimizeCost);

        // Assert
        result.Should().NotBeNull();
        result["Solar"].Should().Be(500); // Use all cheap solar first
        result["Wind"].Should().Be(300); // Then all wind
        result.Values.Sum().Should().Be(demand);
    }

    [Fact]
    public void OptimizeDispatch_ForCO2_ShouldPrioritizeRenewables()
    {
        // Arrange
        var powerSources = new List<PowerSource>
        {
            new() { Name = "Coal", Cost = 30, CO2 = 100, Capacity = 1000, Available = 1000 },
            new() { Name = "Solar", Cost = 50, CO2 = 0, Capacity = 500, Available = 500 },
            new() { Name = "Wind", Cost = 45, CO2 = 0, Capacity = 300, Available = 300 }
        };
        var demand = 800.0;

        // Act
        var result = _service.OptimizeDispatch(powerSources, demand, OptimizationGoal.MinimizeCO2);

        // Assert
        result.Should().NotBeNull();
        result["Solar"].Should().Be(500); // Use all renewables first
        result["Wind"].Should().Be(300);
        result.Values.Sum().Should().Be(demand);
    }

    [Fact]
    public void ForecastDemand_ShouldReturn24HourProfile()
    {
        // Arrange
        var historicalDemand = Enumerable.Range(0, 24)
            .Select(h => 1000.0 + Math.Sin(h / 24.0 * Math.PI * 2) * 200)
            .ToList();

        // Act
        var forecast = _service.ForecastDemand(historicalDemand);

        // Assert
        forecast.Should().HaveCount(24);
        forecast.Should().OnlyContain(f => f > 0);
        forecast.Max().Should().BeGreaterThan(forecast.Min()); // Should show variation
    }

    [Fact]
    public void DistributeSubstationLoad_ShouldBalanceLoad()
    {
        // Arrange
        var substations = new List<Substation>
        {
            new() { Name = "Sub1", Capacity = 1000, CurrentLoad = 200, Location = new(50.0, 8.0) },
            new() { Name = "Sub2", Capacity = 1000, CurrentLoad = 800, Location = new(50.1, 8.1) },
            new() { Name = "Sub3", Capacity = 1000, CurrentLoad = 300, Location = new(50.2, 8.2) }
        };
        var additionalLoad = 600.0;

        // Act
        var distribution = _service.DistributeSubstationLoad(substations, additionalLoad);

        // Assert
        distribution.Should().HaveCount(3);
        distribution.Values.Sum().Should().BeApproximately(additionalLoad, 0.01);
        // Should favor underloaded substations
        distribution["Sub1"].Should().BeGreaterThan(distribution["Sub2"]);
    }
}
