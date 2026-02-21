/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DataPipelineServiceTests.cs                        ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   89.0/100                                       ║
    • Total Lines:     125                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Xunit;
using FluentAssertions;
using Moq;
using RailwayMonitor.WPF.Services;
using System;
using System.Threading.Tasks;

namespace RailwayMonitor.WPF.Tests.Services
{
    public class DataPipelineServiceTests
    {
        private readonly DataPipelineService _service;
        private readonly Mock<IIntelligentCacheService> _mockCache;

        public DataPipelineServiceTests()
        {
            _mockCache = new Mock<IIntelligentCacheService>();
            _service = new DataPipelineService(_mockCache.Object);
        }

        [Fact]
        public void Constructor_ShouldInitialize_WithValidCache()
        {
            // Assert
            _service.Should().NotBeNull();
        }

        [Fact]
        public async Task DownloadAsync_ShouldRetry_OnFailure()
        {
            // Arrange
            var dataSource = new DataSource
            {
                Name = "EU-DEM",
                Url = "http://example.com/data",
                RetryAttempts = 3
            };

            var attempts = 0;
            _mockCache.Setup(c => c.StoreAsync(It.IsAny<string>(), It.IsAny<byte[]>()))
                .Callback(() => attempts++)
                .ThrowsAsync(new Exception("Network error"));

            // Act & Assert
            await Assert.ThrowsAsync<Exception>(() => _service.DownloadAsync(dataSource));
            attempts.Should().BeGreaterOrEqualTo(1);
        }

        [Fact]
        public async Task BulkDownloadGermanyAsync_ShouldDownload_MultipleDataSources()
        {
            // Arrange
            _mockCache.Setup(c => c.StoreAsync(It.IsAny<string>(), It.IsAny<byte[]>()))
                .Returns(Task.CompletedTask);

            // Act
            await _service.BulkDownloadGermanyAsync();

            // Assert
            _mockCache.Verify(c => c.StoreAsync(It.IsAny<string>(), It.IsAny<byte[]>()), 
                Times.AtLeastOnce());
        }

        [Theory]
        [InlineData("OSM", 30)]
        [InlineData("EU-DEM", 365)]
        [InlineData("Weather", 1)]
        public void GetUpdateInterval_ShouldReturn_CorrectDays(string sourceName, int expectedDays)
        {
            // Act
            var interval = _service.GetUpdateInterval(sourceName);

            // Assert
            interval.TotalDays.Should().Be(expectedDays);
        }

        [Fact]
        public void ValidateChecksum_ShouldReturn_TrueForValidData()
        {
            // Arrange
            var data = new byte[] { 1, 2, 3, 4, 5 };
            var checksum = _service.CalculateChecksum(data);

            // Act
            var isValid = _service.ValidateChecksum(data, checksum);

            // Assert
            isValid.Should().BeTrue();
        }

        [Fact]
        public void ValidateChecksum_ShouldReturn_FalseForCorruptedData()
        {
            // Arrange
            var data = new byte[] { 1, 2, 3, 4, 5 };
            var checksum = _service.CalculateChecksum(data);
            data[0] = 99; // Corrupt data

            // Act
            var isValid = _service.ValidateChecksum(data, checksum);

            // Assert
            isValid.Should().BeFalse();
        }
    }
}
