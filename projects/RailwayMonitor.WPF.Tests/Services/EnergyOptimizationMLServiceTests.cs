/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            EnergyOptimizationMLServiceTests.cs                ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     190                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentAssertions;
using Xunit;
using RailwayMonitor.WPF.Services;

namespace RailwayMonitor.WPF.Tests.Services;

public class EnergyOptimizationMLServiceTests
{
    private readonly EnergyOptimizationMLService _service;

    public EnergyOptimizationMLServiceTests()
    {
        _service = new EnergyOptimizationMLService();
    }

    [Fact]
    public async Task Initialize_ShouldSetup_QLearningAgent()
    {
        // Act
        await _service.InitializeAsync();

        // Assert
        _service.IsInitialized.Should().BeTrue();
        _service.GetStateDimensions().Should().Be(7); // 7D state space
        _service.GetActionCount().Should().Be(6);     // 6 actions
    }

    [Fact]
    public async Task GetOptimalAction_ShouldSelect_BestDispatch()
    {
        // Arrange
        var state = new
        {
            Demand = 1000.0,           // kW
            RenewableAvailable = 400.0, // kW
            StorageLevel = 0.5,        // 50%
            EnergyPrice = 0.25,        // €/kWh
            CO2Intensity = 0.4,        // kg/kWh
            WeatherForecast = 0.7,     // Sunny
            TimeOfDay = 12.0           // Noon
        };

        // Act
        var action = await _service.GetOptimalActionAsync(state);

        // Assert
        action.Should().NotBeNullOrEmpty();
        action.Should().BeOneOf(
            "DispatchFossil",
            "DispatchRenewable",
            "ChargeBattery",
            "DischargeBattery",
            "ImportFromGrid",
            "LoadShedding"
        );
    }

    [Theory]
    [InlineData("MinimizeCost")]
    [InlineData("MinimizeCO2")]
    [InlineData("MaximizeRenewables")]
    public async Task SetObjective_ShouldOptimize_ForGoal(string objective)
    {
        // Arrange
        _service.SetOptimizationObjective(objective);
        var state = CreateTestState();

        // Act
        var action = await _service.GetOptimalActionAsync(state);

        // Assert
        action.Should().NotBeNullOrEmpty();
        // Verify action aligns with objective
        if (objective == "MaximizeRenewables")
        {
            action.Should().BeOneOf("DispatchRenewable", "ChargeBattery");
        }
    }

    [Fact]
    public async Task Integrate24HourForecast_ShouldPlan_AheadDispatch()
    {
        // Arrange
        var forecast = new
        {
            DemandProfile = Enumerable.Range(0, 24).Select(h => 800.0 + h * 20.0).ToArray(),
            RenewableProfile = Enumerable.Range(0, 24).Select(h => h < 6 || h > 18 ? 100.0 : 600.0).ToArray(),
            PriceProfile = Enumerable.Range(0, 24).Select(h => h >= 8 && h <= 20 ? 0.30 : 0.15).ToArray()
        };

        // Act
        var dispatchPlan = await _service.Plan24HourDispatchAsync(forecast);

        // Assert
        dispatchPlan.Should().HaveCount(24);
        dispatchPlan.Should().AllSatisfy(plan => plan.Action.Should().NotBeNullOrEmpty());
    }

    [Fact]
    public async Task CalculateParetoFrontier_ShouldFind_OptimalTradeoffs()
    {
        // Act
        var paretoSolutions = await _service.CalculateParetoOptimalSolutionsAsync();

        // Assert
        paretoSolutions.Should().NotBeEmpty();
        paretoSolutions.Should().AllSatisfy(solution =>
        {
            solution.Cost.Should().BeGreaterOrEqualTo(0);
            solution.CO2.Should().BeGreaterOrEqualTo(0);
        });
    }

    [Fact]
    public async Task GetDispatchRecommendation_ShouldInclude_ExpectedSavings()
    {
        // Arrange
        var currentState = CreateTestState();

        // Act
        var recommendation = await _service.GetDispatchRecommendationAsync(currentState);

        // Assert
        recommendation.Action.Should().NotBeNullOrEmpty();
        recommendation.ExpectedSavings.Should().BeInRange(0.08, 0.15); // 8-15% savings
    }

    [Fact]
    public async Task TrainAgent_ShouldImprove_OverTime()
    {
        // Arrange
        var episodes = 100;
        var initialReward = await GetAverageReward();

        // Act
        for (int i = 0; i < episodes; i++)
        {
            await _service.TrainEpisodeAsync();
        }
        var finalReward = await GetAverageReward();

        // Assert
        finalReward.Should().BeGreaterThan(initialReward); // Learning improvement
    }

    private object CreateTestState()
    {
        return new
        {
            Demand = 1000.0,
            RenewableAvailable = 400.0,
            StorageLevel = 0.5,
            EnergyPrice = 0.25,
            CO2Intensity = 0.4,
            WeatherForecast = 0.7,
            TimeOfDay = 12.0
        };
    }

    private async Task<double> GetAverageReward()
    {
        return await Task.FromResult(_service.GetAverageReward());
    }
}
