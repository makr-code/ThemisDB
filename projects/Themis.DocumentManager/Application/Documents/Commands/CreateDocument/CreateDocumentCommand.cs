using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;
using Themis.DocumentManager.Application.Documents.Messages;

namespace Themis.DocumentManager.Application.Documents.Commands.CreateDocument;

/// <summary>
/// Command to create a new document
/// </summary>
public record CreateDocumentCommand : ICreateCommand<DocumentDto>
{
    public string Title { get; init; } = string.Empty;
    public string Description { get; init; } = string.Empty;
    public string MimeType { get; init; } = string.Empty;
    public string Filename { get; init; } = string.Empty;
    public long SizeBytes { get; init; }
    public string Author { get; init; } = string.Empty;
    public string? ContentPreview { get; init; }
    public string? Category { get; init; }
    public string? Classification { get; init; }
    public string? BlobPath { get; init; }
    public Dictionary<string, object>? Metadata { get; init; }
    public List<string>? Tags { get; init; }
    public GeoLocationDto? Location { get; init; }
}
