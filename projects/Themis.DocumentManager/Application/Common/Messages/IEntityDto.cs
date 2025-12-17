namespace Themis.DocumentManager.Application.Common.Messages;

/// <summary>
/// Base interface for all DTOs (Data Transfer Objects)
/// </summary>
public interface IEntityDto
{
    /// <summary>
    /// Unique identifier of the entity
    /// </summary>
    string Id { get; init; }
}
