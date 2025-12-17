using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;

namespace Themis.DocumentManager.Application.Documents.Commands.UpdateDocument;

/// <summary>
/// Command to update an existing document
/// </summary>
public record UpdateDocumentCommand : IUpdateCommand
{
    public string Id { get; init; } = string.Empty;
    public string? Title { get; init; }
    public string? Description { get; init; }
    public string? Category { get; init; }
    public string? Classification { get; init; }
    public Dictionary<string, object>? Metadata { get; init; }
    public List<string>? Tags { get; init; }
}
