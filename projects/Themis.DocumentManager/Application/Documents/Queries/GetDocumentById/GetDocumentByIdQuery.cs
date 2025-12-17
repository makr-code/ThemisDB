using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Documents.Messages;

namespace Themis.DocumentManager.Application.Documents.Queries.GetDocumentById;

/// <summary>
/// Query to get a document by ID
/// </summary>
public record GetDocumentByIdQuery : IGetByIdQuery<DocumentDto>
{
    public string Id { get; init; } = string.Empty;
}
