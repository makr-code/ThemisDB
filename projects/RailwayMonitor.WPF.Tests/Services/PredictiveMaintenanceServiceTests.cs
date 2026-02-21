/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            PredictiveMaintenanceServiceTests.cs               ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     203                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using FluentAssertions;
using Xunit;
using RailwayMonitor.WPF.Services;

namespace RailwayMonitor.WPF.Tests.Services;

public class PredictiveMaintenanceServiceTests
{
    private readonly PredictiveMaintenanceService _service;

    public PredictiveMaintenanceServiceTests()
    {
        _service = new PredictiveMaintenanceService();
    }

    [Theory]
    [InlineData("Motor")]
    [InlineData("Brakes")]
    [InlineData("Bogie")]
    [InlineData("Wheels")]
    [InlineData("Pantograph")]
    [InlineData("Transmission")]
    [InlineData("Battery")]
    [InlineData("HVAC")]
    public async Task MonitorComponent_ShouldTrack_AllComponentTypes(string componentType)
    {
        // Arrange
        var trainId = "ICE_001";
        var componentId = $"{componentType}_001";

        // Act
        await _service.StartMonitoringAsync(trainId, componentId, componentType);

        // Assert
        var isMonitored = _service.IsComponentMonitored(trainId, componentId);
        isMonitored.Should().BeTrue();
    }

    [Fact]
    public async Task AnalyzeSensorData_ShouldDetect_TemperatureAnomaly()
    {
        // Arrange
        var normalTemp = 45.0;
        var anomalousTemp = 95.0; // Overheating
        var sensorData = new[] { normalTemp, normalTemp, anomalousTemp };

        // Act
        var anomalies = await _service.DetectAnomaliesAsync("Motor_001", "Temperature", sensorData);

        // Assert
        anomalies.Should().ContainSingle();
        anomalies.First().Value.Should().Be(anomalousTemp);
    }

    [Fact]
    public async Task AnalyzeSensorData_ShouldDetect_VibrationAnomaly()
    {
        // Arrange
        var normalVibration = 2.0;
        var anomalousVibration = 15.0; // Excessive
        var sensorData = new[] { normalVibration, normalVibration, anomalousVibration };

        // Act
        var anomalies = await _service.DetectAnomaliesAsync("Bogie_001", "Vibration", sensorData);

        // Assert
        anomalies.Should().ContainSingle();
    }

    [Fact]
    public async Task DetectAnomalies_ZScore_ShouldIdentify_Outliers()
    {
        // Arrange
        var sensorData = new[] { 10.0, 11.0, 10.5, 11.5, 50.0 }; // Last value is outlier

        // Act
        var anomalies = await _service.DetectAnomalies_ZScoreAsync("Component_001", sensorData);

        // Assert
        anomalies.Should().ContainSingle();
        anomalies.First().Should().BeApproximately(50.0, 0.1);
    }

    [Fact]
    public async Task DetectAnomalies_IsolationForest_ShouldWork()
    {
        // Arrange
        var normalData = Enumerable.Range(1, 100).Select(i => 10.0 + i * 0.1).ToArray();
        var dataWithAnomaly = normalData.Append(100.0).ToArray();

        // Act
        var anomalies = await _service.DetectAnomalies_IsolationForestAsync("Component_001", dataWithAnomaly);

        // Assert
        anomalies.Should().NotBeEmpty();
    }

    [Fact]
    public async Task DetectAnomalies_DBSCAN_ShouldCluster()
    {
        // Arrange
        var clusteredData = new[] { 10.0, 10.5, 11.0, 10.8, 50.0, 51.0 }; // Two clusters

        // Act
        var anomalies = await _service.DetectAnomalies_DBSCANAsync("Component_001", clusteredData);

        // Assert
        anomalies.Should().NotBeEmpty(); // Outliers from main cluster
    }

    [Fact]
    public async Task PredictRUL_ShouldEstimate_RemainingUsefulLife()
    {
        // Arrange
        var componentId = "Motor_001";
        var sensorHistory = Enumerable.Range(1, 1000).Select(i => 45.0 + i * 0.01).ToArray();

        // Act
        var rul = await _service.PredictRemainingUsefulLifeAsync(componentId, sensorHistory);

        // Assert
        rul.Days.Should().BeGreaterThan(0);
        rul.Days.Should().BeLessThan(365); // Within a year
    }

    [Fact]
    public async Task ScheduleMaintenance_ShouldRecommend_Action()
    {
        // Arrange
        var componentId = "Brakes_001";
        var rul = TimeSpan.FromDays(30);

        // Act
        var recommendation = await _service.ScheduleMaintenanceAsync(componentId, rul);

        // Assert
        recommendation.Action.Should().NotBeNullOrEmpty();
        recommendation.Priority.Should().BeOneOf("High", "Medium", "Low");
    }

    [Fact]
    public async Task CalculateCostBenefit_ShouldCompare_PreventiveVsReactive()
    {
        // Arrange
        var preventiveCost = 5000.0;
        var reactiveCost = 25000.0;
        var failureProbability = 0.3;

        // Act
        var analysis = await _service.CalculateCostBenefitAsync(preventiveCost, reactiveCost, failureProbability);

        // Assert
        analysis.RecommendPreventive.Should().BeTrue(); // Expected cost lower
        analysis.ExpectedSavings.Should().BeGreaterThan(0);
    }

    [Fact]
    public async Task FalsePositiveRate_ShouldBe_LessThan5Percent()
    {
        // Arrange
        var normalDataSets = Enumerable.Range(1, 100)
            .Select(i => Enumerable.Range(1, 100).Select(j => 10.0 + j * 0.05).ToArray());
        
        var falsePositives = 0;
        foreach (var dataSet in normalDataSets)
        {
            var anomalies = await _service.DetectAnomaliesAsync($"Component_{falsePositives}", "Test", dataSet);
            if (anomalies.Any()) falsePositives++;
        }

        // Act
        var falsePositiveRate = falsePositives / 100.0;

        // Assert
        falsePositiveRate.Should().BeLessThan(0.05); // < 5%
    }
}
