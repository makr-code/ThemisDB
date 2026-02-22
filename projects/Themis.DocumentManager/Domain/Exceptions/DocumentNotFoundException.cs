/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentNotFoundException.cs                       ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     40                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
