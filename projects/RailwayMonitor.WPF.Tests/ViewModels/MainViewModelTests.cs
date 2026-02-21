/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MainViewModelTests.cs                              ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   84.0/100                                       ║
    • Total Lines:     181                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using RailwayMonitor.WPF.ViewModels;
using RailwayMonitor.WPF.Services;
using RailwayMonitor.WPF.Models;
using System.Collections.ObjectModel;

namespace RailwayMonitor.WPF.Tests.ViewModels;

/// <summary>
/// Unit tests for MainViewModel - Core MVVM logic
/// </summary>
public class MainViewModelTests
{
    private readonly Mock<IThemisDbService> _mockDbService;
    private readonly Mock<IEnergyManagementService> _mockEnergyService;
    private readonly MainViewModel _viewModel;

    public MainViewModelTests()
    {
        _mockDbService = new Mock<IThemisDbService>();
        _mockEnergyService = new Mock<IEnergyManagementService>();
        _viewModel = new MainViewModel(_mockDbService.Object, _mockEnergyService.Object);
    }

    [Fact]
    public async Task LoadStationsAsync_ShouldPopulateStationsCollection()
    {
        // Arrange
        var expectedStations = new List<Station>
        {
            new() { Name = "Frankfurt Hbf", Location = new(50.107, 8.663) },
            new() { Name = "München Hbf", Location = new(48.140, 11.560) }
        };

        _mockDbService.Setup(s => s.GetStationsAsync())
            .ReturnsAsync(expectedStations);

        // Act
        await _viewModel.LoadStationsAsync();

        // Assert
        _viewModel.Stations.Should().NotBeNull();
        _viewModel.Stations.Should().HaveCount(2);
        _viewModel.Stations.First().Name.Should().Be("Frankfurt Hbf");
    }

    [Fact]
    public async Task LoadTrainsAsync_ShouldPopulateTrainsCollection()
    {
        // Arrange
        var expectedTrains = new List<Train>
        {
            new() { Number = "ICE 123", CurrentSpeed = 250, Delayed = false },
            new() { Number = "RE 456", CurrentSpeed = 120, Delayed = true }
        };

        _mockDbService.Setup(s => s.GetTrainsAsync())
            .ReturnsAsync(expectedTrains);

        // Act
        await _viewModel.LoadTrainsAsync();

        // Assert
        _viewModel.Trains.Should().NotBeNull();
        _viewModel.Trains.Should().HaveCount(2);
        _viewModel.Trains.Should().Contain(t => t.Number == "ICE 123");
    }

    [Fact]
    public void RefreshCommand_ShouldBeExecutable()
    {
        // Act
        var canExecute = _viewModel.RefreshCommand.CanExecute(null);

        // Assert
        canExecute.Should().BeTrue();
    }

    [Fact]
    public async Task RefreshCommand_ShouldReloadAllData()
    {
        // Arrange
        var stations = new List<Station> { new() { Name = "Test Station" } };
        var trains = new List<Train> { new() { Number = "TEST 1" } };

        _mockDbService.Setup(s => s.GetStationsAsync()).ReturnsAsync(stations);
        _mockDbService.Setup(s => s.GetTrainsAsync()).ReturnsAsync(trains);

        // Act
        _viewModel.RefreshCommand.Execute(null);
        await Task.Delay(100); // Allow async execution

        // Assert
        _mockDbService.Verify(s => s.GetStationsAsync(), Times.Once);
        _mockDbService.Verify(s => s.GetTrainsAsync(), Times.Once);
    }

    [Fact]
    public void SearchText_ShouldFilterTrains()
    {
        // Arrange
        _viewModel.Trains = new ObservableCollection<Train>
        {
            new() { Number = "ICE 123" },
            new() { Number = "RE 456" },
            new() { Number = "ICE 789" }
        };

        // Act
        _viewModel.SearchText = "ICE";

        // Assert
        _viewModel.FilteredTrains.Should().HaveCount(2);
        _viewModel.FilteredTrains.Should().OnlyContain(t => t.Number.Contains("ICE"));
    }

    [Fact]
    public void FilterDelayedCommand_ShouldShowOnlyDelayedTrains()
    {
        // Arrange
        _viewModel.Trains = new ObservableCollection<Train>
        {
            new() { Number = "ICE 123", Delayed = false },
            new() { Number = "RE 456", Delayed = true },
            new() { Number = "S 1", Delayed = true }
        };

        // Act
        _viewModel.FilterDelayedCommand.Execute(null);

        // Assert
        _viewModel.FilteredTrains.Should().HaveCount(2);
        _viewModel.FilteredTrains.Should().OnlyContain(t => t.Delayed);
    }

    [Fact]
    public void OptimizeCostCommand_ShouldCallEnergyService()
    {
        // Arrange
        _mockEnergyService.Setup(s => s.OptimizeDispatch(
            It.IsAny<List<PowerSource>>(),
            It.IsAny<double>(),
            OptimizationGoal.MinimizeCost))
            .Returns(new Dictionary<string, double>());

        // Act
        _viewModel.OptimizeCostCommand.Execute(null);

        // Assert
        _mockEnergyService.Verify(s => s.OptimizeDispatch(
            It.IsAny<List<PowerSource>>(),
            It.IsAny<double>(),
            OptimizationGoal.MinimizeCost),
            Times.Once);
    }
}
