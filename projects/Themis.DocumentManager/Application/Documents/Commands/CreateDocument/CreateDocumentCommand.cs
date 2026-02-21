/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateDocumentCommand.cs                           ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
