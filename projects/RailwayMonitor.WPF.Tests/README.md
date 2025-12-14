# Railway Monitor WPF - Unit Tests

## Overview

Comprehensive unit test suite for the Railway Monitoring System WPF client, targeting **80%+ code coverage**.

## Test Framework

- **xUnit** - Modern, extensible testing framework
- **FluentAssertions** - Readable, expressive assertions
- **Moq** - Mocking framework for dependencies
- **Coverlet** - Code coverage collection

## Running Tests

```bash
# Run all tests
dotnet test

# Run with code coverage
dotnet test /p:CollectCoverage=true /p:CoverletOutputFormat=opencover

# Run specific test class
dotnet test --filter "FullyQualifiedName~ThemisDbServiceTests"

# Run tests with detailed output
dotnet test --verbosity detailed
```

## Test Structure

```
RailwayMonitor.WPF.Tests/
├── Services/
│   ├── ThemisDbServiceTests.cs          # REST/AQL integration tests
│   ├── EnergyManagementServiceTests.cs  # Power management tests
│   ├── ChangeFeedServiceTests.cs        # SSE streaming tests
│   ├── MapServiceTests.cs               # Map rendering tests
│   ├── GeoSpatialAnalyzerTests.cs       # Pathfinding tests
│   ├── DataPipelineServiceTests.cs      # Data download tests
│   └── CacheServiceTests.cs             # 3-tier cache tests
├── ViewModels/
│   ├── MainViewModelTests.cs            # Core MVVM tests
│   └── TrainViewModelTests.cs           # Component VM tests
├── Models/
│   └── TrainTests.cs                    # Model validation tests
└── Integration/
    └── DataPipelineIntegrationTests.cs  # End-to-end tests
```

## Code Coverage Targets

| Component | Target | Status |
|-----------|--------|--------|
| Services | 85% | ⏳ In Progress |
| ViewModels | 80% | ⏳ In Progress |
| Models | 90% | ⏳ In Progress |
| **Overall** | **80%+** | ⏳ In Progress |

## Test Patterns

### Arrange-Act-Assert (AAA)

```csharp
[Fact]
public async Task GetTrainsAsync_ShouldReturnTrains_WhenApiReturnsData()
{
    // Arrange
    var expectedData = CreateTestData();
    _mockService.Setup(...).ReturnsAsync(expectedData);
    
    // Act
    var result = await _service.GetTrainsAsync();
    
    // Assert
    result.Should().NotBeNull();
    result.Should().HaveCount(2);
}
```

### Theory-Based Data-Driven Tests

```csharp
[Theory]
[InlineData("ICE")]
[InlineData("RE")]
[InlineData("S")]
public async Task GetTrainsByType_ShouldFilter(string trainType)
{
    // Test with multiple input values
}
```

### Mocking Dependencies

```csharp
var mockService = new Mock<IThemisDbService>();
mockService.Setup(s => s.GetTrainsAsync())
    .ReturnsAsync(testData);
```

## Next Steps

- [ ] Complete service layer tests (20+ services)
- [ ] Add integration tests for data pipeline
- [ ] Performance benchmarks with BenchmarkDotNet
- [ ] Mutation testing with Stryker.NET
- [ ] CI/CD integration (when re-enabled)

## Phase 1 Progress

**Week 1-6 Goal**: 80%+ code coverage

- ✅ Test framework setup
- ✅ ThemisDbService tests (10 tests)
- ✅ EnergyManagementService tests (7 tests)
- ✅ MainViewModel tests (8 tests)
- ⏳ Remaining services (15+)
- ⏳ Integration tests
- ⏳ Benchmarks

**Current Coverage**: ~10% (foundation complete)
**Target Coverage**: 80%+ (by Week 6)
