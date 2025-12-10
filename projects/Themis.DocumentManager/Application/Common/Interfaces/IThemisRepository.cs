using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Common.Interfaces;

/// <summary>
/// Repository abstraction for ThemisDB operations
/// </summary>
public interface IThemisRepository
{
    Task<string> CreateDocumentAsync(Document document, CancellationToken cancellationToken = default);
    Task<Document?> GetDocumentAsync(string id, CancellationToken cancellationToken = default);
    Task<List<Document>> GetDocumentsAsync(int page, int pageSize, CancellationToken cancellationToken = default);
    Task<bool> UpdateDocumentAsync(Document document, CancellationToken cancellationToken = default);
    Task<bool> DeleteDocumentAsync(string id, CancellationToken cancellationToken = default);
}
