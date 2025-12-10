using System;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Extensions.Logging;
using Moq;
using Themis.DocumentManager.Application.Collaboration.Commands;
using Themis.DocumentManager.Application.Collaboration.Handlers;
using Themis.DocumentManager.Domain.Collaboration;
using Themis.DocumentManager.Services;
using Xunit;

namespace Themis.DocumentManager.Tests.Collaboration;

/// <summary>
/// Unit Tests für Check-in/Check-out Commands und Handlers.
/// Phase 2 Sprint 5-6 Tests.
/// </summary>
public class DocumentLockingTests
{
    private readonly Mock<IDocumentLockingService> _mockLockingService;
    private readonly Mock<ILogger<CheckOutDocumentHandler>> _mockLogger;

    public DocumentLockingTests()
    {
        _mockLockingService = new Mock<IDocumentLockingService>();
        _mockLogger = new Mock<ILogger<CheckOutDocumentHandler>>();
    }

    [Fact]
    public async Task CheckOutDocument_Success_ReturnsLock()
    {
        // Arrange
        var command = new CheckOutDocumentCommand(
            DocumentId: "doc123",
            UserId: "user456",
            UserName: "Test User",
            LockType: LockType.Write,
            TimeoutMinutes: 30
        );

        var expectedLock = new DocumentLock
        {
            DocumentId = "doc123",
            UserId = "user456",
            UserName = "Test User",
            Type = LockType.Write
        };

        _mockLockingService
            .Setup(x => x.GetDocumentLockAsync("doc123"))
            .ReturnsAsync((DocumentLock?)null);

        _mockLockingService
            .Setup(x => x.AcquireLockAsync(It.IsAny<DocumentLock>(), It.IsAny<CancellationToken>()))
            .ReturnsAsync(true);

        var handler = new CheckOutDocumentHandler(_mockLockingService.Object, _mockLogger.Object);

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.True(result.Success);
        Assert.NotNull(result.Value);
        Assert.Equal("doc123", result.Value.DocumentId);
        Assert.Equal("user456", result.Value.UserId);
        
        _mockLockingService.Verify(x => x.AcquireLockAsync(
            It.Is<DocumentLock>(l => l.DocumentId == "doc123"), 
            It.IsAny<CancellationToken>()), 
            Times.Once);
    }

    [Fact]
    public async Task CheckOutDocument_AlreadyLocked_ReturnsError()
    {
        // Arrange
        var command = new CheckOutDocumentCommand(
            DocumentId: "doc123",
            UserId: "user456",
            UserName: "Test User"
        );

        var existingLock = new DocumentLock
        {
            DocumentId = "doc123",
            UserId = "otheruser",
            UserName = "Other User",
            LockedAt = DateTime.UtcNow
        };

        _mockLockingService
            .Setup(x => x.GetDocumentLockAsync("doc123"))
            .ReturnsAsync(existingLock);

        var handler = new CheckOutDocumentHandler(_mockLockingService.Object, _mockLogger.Object);

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.False(result.Success);
        Assert.Contains("bereits", result.ErrorMessage);
        
        _mockLockingService.Verify(x => x.AcquireLockAsync(
            It.IsAny<DocumentLock>(), 
            It.IsAny<CancellationToken>()), 
            Times.Never);
    }

    [Fact]
    public async Task CheckInDocument_Success_ReleasesLock()
    {
        // Arrange
        var command = new CheckInDocumentCommand(
            DocumentId: "doc123",
            UserId: "user456"
        );

        var existingLock = new DocumentLock
        {
            DocumentId = "doc123",
            UserId = "user456",
            UserName = "Test User"
        };

        _mockLockingService
            .Setup(x => x.GetDocumentLockAsync("doc123"))
            .ReturnsAsync(existingLock);

        _mockLockingService
            .Setup(x => x.ReleaseLockAsync("doc123", It.IsAny<CancellationToken>()))
            .ReturnsAsync(true);

        var mockLogger = new Mock<ILogger<CheckInDocumentHandler>>();
        var handler = new CheckInDocumentHandler(_mockLockingService.Object, mockLogger.Object);

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.True(result.Success);
        
        _mockLockingService.Verify(x => x.ReleaseLockAsync(
            "doc123", 
            It.IsAny<CancellationToken>()), 
            Times.Once);
    }

    [Fact]
    public async Task CheckInDocument_NoLock_ReturnsError()
    {
        // Arrange
        var command = new CheckInDocumentCommand(
            DocumentId: "doc123",
            UserId: "user456"
        );

        _mockLockingService
            .Setup(x => x.GetDocumentLockAsync("doc123"))
            .ReturnsAsync((DocumentLock?)null);

        var mockLogger = new Mock<ILogger<CheckInDocumentHandler>>();
        var handler = new CheckInDocumentHandler(_mockLockingService.Object, mockLogger.Object);

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.False(result.Success);
        Assert.Contains("Keine aktive Sperre", result.ErrorMessage);
    }

    [Fact]
    public async Task CheckInDocument_WrongUser_ReturnsError()
    {
        // Arrange
        var command = new CheckInDocumentCommand(
            DocumentId: "doc123",
            UserId: "user456"
        );

        var existingLock = new DocumentLock
        {
            DocumentId = "doc123",
            UserId = "otheruser",
            UserName = "Other User"
        };

        _mockLockingService
            .Setup(x => x.GetDocumentLockAsync("doc123"))
            .ReturnsAsync(existingLock);

        var mockLogger = new Mock<ILogger<CheckInDocumentHandler>>();
        var handler = new CheckInDocumentHandler(_mockLockingService.Object, mockLogger.Object);

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.False(result.Success);
        Assert.Contains("eigenen Sperren", result.ErrorMessage);
    }
}

/// <summary>
/// Unit Tests für DocumentLock Domain Entity.
/// </summary>
public class DocumentLockDomainTests
{
    [Fact]
    public void DocumentLock_IsExpired_ReturnsTrueWhenExpired()
    {
        // Arrange
        var lock = new DocumentLock
        {
            DocumentId = "doc123",
            UserId = "user456",
            ExpiresAt = DateTime.UtcNow.AddMinutes(-5) // Expired 5 minutes ago
        };

        // Act
        var isExpired = @lock.IsExpired();

        // Assert
        Assert.True(isExpired);
    }

    [Fact]
    public void DocumentLock_IsActive_ReturnsFalseWhenExpired()
    {
        // Arrange
        var lock = new DocumentLock
        {
            DocumentId = "doc123",
            UserId = "user456",
            ExpiresAt = DateTime.UtcNow.AddMinutes(-5)
        };

        // Act
        var isActive = @lock.IsActive();

        // Assert
        Assert.False(isActive);
    }

    [Fact]
    public void DocumentLock_IsActive_ReturnsTrueWhenNotExpired()
    {
        // Arrange
        var lock = new DocumentLock
        {
            DocumentId = "doc123",
            UserId = "user456",
            ExpiresAt = DateTime.UtcNow.AddMinutes(30) // Expires in 30 minutes
        };

        // Act
        var isActive = @lock.IsActive();

        // Assert
        Assert.True(isActive);
    }

    [Fact]
    public void DocumentLock_NoExpiration_IsAlwaysActive()
    {
        // Arrange
        var lock = new DocumentLock
        {
            DocumentId = "doc123",
            UserId = "user456",
            ExpiresAt = null // No expiration
        };

        // Act
        var isActive = @lock.IsActive();
        var isExpired = @lock.IsExpired();

        // Assert
        Assert.True(isActive);
        Assert.False(isExpired);
    }
}
