/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentNotFoundException.cs                       ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 77d774278  2025-12-10  Phase 1 Sprint 1: Clean Architecture Foundation mit CQRS ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

namespace Themis.DocumentManager.Domain.Exceptions;

/// <summary>
/// Exception thrown when a document is not found
/// </summary>
public class DocumentNotFoundException : Exception
{
    public string DocumentId { get; }

    public DocumentNotFoundException(string documentId)
        : base($"Document with ID '{documentId}' was not found.")
    {
        DocumentId = documentId;
    }

    public DocumentNotFoundException(string documentId, Exception innerException)
        : base($"Document with ID '{documentId}' was not found.", innerException)
    {
        DocumentId = documentId;
    }
}
