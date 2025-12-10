using MediatR;

namespace Themis.DocumentManager.Application.Documents.Commands.CreateDocument;

/// <summary>
/// Command to create a new document
/// </summary>
public record CreateDocumentCommand : IRequest<string>
{
    public string Title { get; init; } = string.Empty;
    public string Description { get; init; } = string.Empty;
    public string MimeType { get; init; } = string.Empty;
    public string Filename { get; init; } = string.Empty;
    public long SizeBytes { get; init; }
    public string Author { get; init; } = string.Empty;
    public string? ContentPreview { get; init; }
    public string? Category { get; init; }
    public Dictionary<string, object>? Metadata { get; init; }
    public List<string>? Tags { get; init; }
}
