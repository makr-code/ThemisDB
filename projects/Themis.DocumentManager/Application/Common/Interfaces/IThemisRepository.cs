/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IThemisRepository.cs                               ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     41                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
