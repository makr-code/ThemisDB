/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CommentTests.cs                                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     315                                            ║
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

using System;
using System.Collections.Generic;
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
/// Unit Tests für Comment Commands und Handlers.
/// Phase 2 Sprint 5-6 Tests.
/// </summary>
public class CommentTests
{
    private readonly Mock<ICommentService> _mockCommentService;
    private readonly Mock<ILogger<AddCommentHandler>> _mockLogger;

    public CommentTests()
    {
        _mockCommentService = new Mock<ICommentService>();
        _mockLogger = new Mock<ILogger<AddCommentHandler>>();
    }

    [Fact]
    public async Task AddComment_Success_ReturnsComment()
    {
        // Arrange
        var command = new AddCommentCommand(
            DocumentId: "doc123",
            AuthorId: "user456",
            AuthorName: "Test User",
            Content: "This is a test comment"
        );

        _mockCommentService
            .Setup(x => x.AddCommentAsync(It.IsAny<Comment>(), It.IsAny<CancellationToken>()))
            .ReturnsAsync(true);

        var handler = new AddCommentHandler(_mockCommentService.Object, _mockLogger.Object);

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.True(result.Success);
        Assert.NotNull(result.Value);
        Assert.Equal("doc123", result.Value.DocumentId);
        Assert.Equal("user456", result.Value.AuthorId);
        Assert.Equal("This is a test comment", result.Value.Content);

        _mockCommentService.Verify(x => x.AddCommentAsync(
            It.Is<Comment>(c => c.DocumentId == "doc123"),
            It.IsAny<CancellationToken>()),
            Times.Once);
    }

    [Fact]
    public async Task AddComment_WithMentions_SetsMentionedUsers()
    {
        // Arrange
        var mentionedUsers = new List<string> { "user789", "user101" };
        var command = new AddCommentCommand(
            DocumentId: "doc123",
            AuthorId: "user456",
            AuthorName: "Test User",
            Content: "@JohnDoe please review",
            MentionedUserIds: mentionedUsers
        );

        _mockCommentService
            .Setup(x => x.AddCommentAsync(It.IsAny<Comment>(), It.IsAny<CancellationToken>()))
            .ReturnsAsync(true);

        var handler = new AddCommentHandler(_mockCommentService.Object, _mockLogger.Object);

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.True(result.Success);
        Assert.NotNull(result.Value);
        Assert.Equal(2, result.Value.MentionedUserIds.Count);
        Assert.Contains("user789", result.Value.MentionedUserIds);
        Assert.Contains("user101", result.Value.MentionedUserIds);
    }

    [Fact]
    public async Task AddComment_WithParent_SetsThreadId()
    {
        // Arrange
        var parentComment = new Comment
        {
            Id = "parent123",
            ThreadId = "thread123",
            DocumentId = "doc123"
        };

        var command = new AddCommentCommand(
            DocumentId: "doc123",
            AuthorId: "user456",
            AuthorName: "Test User",
            Content: "Reply to parent",
            ParentCommentId: "parent123"
        );

        _mockCommentService
            .Setup(x => x.GetCommentAsync("parent123"))
            .ReturnsAsync(parentComment);

        _mockCommentService
            .Setup(x => x.AddCommentAsync(It.IsAny<Comment>(), It.IsAny<CancellationToken>()))
            .ReturnsAsync(true);

        var handler = new AddCommentHandler(_mockCommentService.Object, _mockLogger.Object);

        // Act
        var result = await handler.Handle(command, CancellationToken.None);

        // Assert
        Assert.True(result.Success);
        Assert.NotNull(result.Value);
        Assert.Equal("parent123", result.Value.ParentCommentId);
        Assert.Equal("thread123", result.Value.ThreadId);
    }
}

/// <summary>
/// Unit Tests für Comment Domain Entity.
/// </summary>
public class CommentDomainTests
{
    [Fact]
    public void Comment_IsEdited_ReturnsTrueWhenUpdated()
    {
        // Arrange
        var comment = new Comment
        {
            CreatedAt = DateTime.UtcNow.AddHours(-1),
            UpdatedAt = DateTime.UtcNow
        };

        // Act
        var isEdited = comment.IsEdited;

        // Assert
        Assert.True(isEdited);
    }

    [Fact]
    public void Comment_IsEdited_ReturnsFalseWhenNotUpdated()
    {
        // Arrange
        var comment = new Comment
        {
            CreatedAt = DateTime.UtcNow,
            UpdatedAt = null
        };

        // Act
        var isEdited = comment.IsEdited;

        // Assert
        Assert.False(isEdited);
    }

    [Fact]
    public void Comment_Reactions_CanBeAdded()
    {
        // Arrange
        var comment = new Comment();

        // Act
        comment.Reactions.Add(new CommentReaction
        {
            UserId = "user123",
            Type = "👍"
        });
        comment.Reactions.Add(new CommentReaction
        {
            UserId = "user456",
            Type = "❤️"
        });

        // Assert
        Assert.Equal(2, comment.Reactions.Count);
    }

    [Fact]
    public void Comment_Attachments_CanBeAdded()
    {
        // Arrange
        var comment = new Comment();

        // Act
        comment.Attachments.Add(new CommentAttachment
        {
            FileName = "screenshot.png",
            ContentType = "image/png",
            Size = 12345
        });

        // Assert
        Assert.Single(comment.Attachments);
        Assert.Equal("screenshot.png", comment.Attachments[0].FileName);
    }

    [Fact]
    public void Comment_Position_CanBeSet()
    {
        // Arrange
        var comment = new Comment();

        // Act
        comment.Position = new DocumentPosition
        {
            Page = 5,
            StartOffset = 100,
            EndOffset = 200
        };

        // Assert
        Assert.NotNull(comment.Position);
        Assert.Equal(5, comment.Position.Page);
        Assert.Equal(100, comment.Position.StartOffset);
        Assert.Equal(200, comment.Position.EndOffset);
    }
}

/// <summary>
/// Unit Tests für UserPresence Domain Entity.
/// </summary>
public class UserPresenceTests
{
    [Fact]
    public void UserPresence_IsActive_ReturnsTrueWithinTimeout()
    {
        // Arrange
        var presence = new UserPresence
        {
            LastActivityAt = DateTime.UtcNow.AddMinutes(-2)
        };

        // Act
        var isActive = presence.IsActive(TimeSpan.FromMinutes(5));

        // Assert
        Assert.True(isActive);
    }

    [Fact]
    public void UserPresence_IsActive_ReturnsFalseAfterTimeout()
    {
        // Arrange
        var presence = new UserPresence
        {
            LastActivityAt = DateTime.UtcNow.AddMinutes(-10)
        };

        // Act
        var isActive = presence.IsActive(TimeSpan.FromMinutes(5));

        // Assert
        Assert.False(isActive);
    }

    [Fact]
    public void UserPresence_UpdateActivity_UpdatesTimestamp()
    {
        // Arrange
        var presence = new UserPresence
        {
            LastActivityAt = DateTime.UtcNow.AddMinutes(-5)
        };
        var oldActivity = presence.LastActivityAt;

        // Act
        System.Threading.Thread.Sleep(100); // Small delay
        presence.UpdateActivity();

        // Assert
        Assert.True(presence.LastActivityAt > oldActivity);
    }
}
