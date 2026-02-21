/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IThemisRepository.cs                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     15                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
