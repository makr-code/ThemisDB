/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisDbServiceTests.cs                            ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     165                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using RailwayMonitor.WPF.Services;
using RailwayMonitor.WPF.Models;
using System.Net;
using System.Net.Http;

namespace RailwayMonitor.WPF.Tests.Services;

/// <summary>
/// Unit tests for ThemisDbService - REST/AQL integration layer
/// </summary>
public class ThemisDbServiceTests
{
    private readonly Mock<HttpMessageHandler> _mockHttpHandler;
    private readonly ThemisDbService _service;

    public ThemisDbServiceTests()
    {
        _mockHttpHandler = new Mock<HttpMessageHandler>();
        var httpClient = new HttpClient(_mockHttpHandler.Object)
        {
            BaseAddress = new Uri("http://localhost:8529")
        };
        _service = new ThemisDbService(httpClient);
    }

    [Fact]
    public async Task GetTrainsAsync_ShouldReturnTrains_WhenApiReturnsData()
    {
        // Arrange
        var expectedJson = """
        {
            "result": [
                {"_key": "1", "number": "ICE 123", "currentSpeed": 250, "maxSpeed": 300, "delayed": false},
                {"_key": "2", "number": "RE 456", "currentSpeed": 120, "maxSpeed": 160, "delayed": true}
            ]
        }
        """;

        _mockHttpHandler.Protected()
            .Setup<Task<HttpResponseMessage>>(
                "SendAsync",
                ItExpr.IsAny<HttpRequestMessage>(),
                ItExpr.IsAny<CancellationToken>())
            .ReturnsAsync(new HttpResponseMessage
            {
                StatusCode = HttpStatusCode.OK,
                Content = new StringContent(expectedJson)
            });

        // Act
        var result = await _service.GetTrainsAsync();

        // Assert
        result.Should().NotBeNull();
        result.Should().HaveCount(2);
        result.First().Number.Should().Be("ICE 123");
        result.First().CurrentSpeed.Should().Be(250);
        result.Last().Delayed.Should().BeTrue();
    }

    [Fact]
    public async Task GetTrainsAsync_ShouldThrowException_WhenApiFails()
    {
        // Arrange
        _mockHttpHandler.Protected()
            .Setup<Task<HttpResponseMessage>>(
                "SendAsync",
                ItExpr.IsAny<HttpRequestMessage>(),
                ItExpr.IsAny<CancellationToken>())
            .ReturnsAsync(new HttpResponseMessage
            {
                StatusCode = HttpStatusCode.InternalServerError
            });

        // Act & Assert
        await _service.Invoking(s => s.GetTrainsAsync())
            .Should().ThrowAsync<HttpRequestException>();
    }

    [Fact]
    public async Task GetDelayedTrainsAsync_ShouldReturnOnlyDelayedTrains()
    {
        // Arrange
        var expectedJson = """
        {
            "result": [
                {"_key": "2", "number": "RE 456", "currentSpeed": 120, "maxSpeed": 160, "delayed": true}
            ]
        }
        """;

        _mockHttpHandler.Protected()
            .Setup<Task<HttpResponseMessage>>(
                "SendAsync",
                ItExpr.Is<HttpRequestMessage>(r => r.RequestUri!.ToString().Contains("delayed=true")),
                ItExpr.IsAny<CancellationToken>())
            .ReturnsAsync(new HttpResponseMessage
            {
                StatusCode = HttpStatusCode.OK,
                Content = new StringContent(expectedJson)
            });

        // Act
        var result = await _service.GetDelayedTrainsAsync();

        // Assert
        result.Should().NotBeNull();
        result.Should().HaveCount(1);
        result.First().Delayed.Should().BeTrue();
    }

    [Theory]
    [InlineData("ICE")]
    [InlineData("RE")]
    [InlineData("S")]
    public async Task GetTrainsByTypeAsync_ShouldFilterByTrainType(string trainType)
    {
        // Arrange
        var expectedJson = $"""
        {{
            "result": [
                {{"_key": "1", "number": "{trainType} 123", "type": "{trainType}", "currentSpeed": 200, "maxSpeed": 300, "delayed": false}}
            ]
        }}
        """;

        _mockHttpHandler.Protected()
            .Setup<Task<HttpResponseMessage>>(
                "SendAsync",
                ItExpr.Is<HttpRequestMessage>(r => r.RequestUri!.ToString().Contains($"type={trainType}")),
                ItExpr.IsAny<CancellationToken>())
            .ReturnsAsync(new HttpResponseMessage
            {
                StatusCode = HttpStatusCode.OK,
                Content = new StringContent(expectedJson)
            });

        // Act
        var result = await _service.GetTrainsByTypeAsync(trainType);

        // Assert
        result.Should().NotBeNull();
        result.Should().HaveCount(1);
        result.First().Number.Should().StartWith(trainType);
    }
}
