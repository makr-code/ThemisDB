/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DocumentDto.cs                                     ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     61                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Themis.DocumentManager.Application.Common.Messages;

namespace Themis.DocumentManager.Application.Documents.Messages;

/// <summary>
/// Document DTO for data transfer
/// </summary>
public record DocumentDto : BaseEntityDto
{
    public string Title { get; init; } = string.Empty;
    public string Description { get; init; } = string.Empty;
    public string MimeType { get; init; } = string.Empty;
    public string Filename { get; init; } = string.Empty;
    public long SizeBytes { get; init; }
    public string Author { get; init; } = string.Empty;
    public Dictionary<string, object> Metadata { get; init; } = new();
    public List<string> Tags { get; init; } = new();
    public GeoLocationDto? Location { get; init; }
    public string? Classification { get; init; }
    public string? Category { get; init; }
    public string? ContentPreview { get; init; }
    public string? BlobPath { get; init; }
}

/// <summary>
/// Geographic location DTO
/// </summary>
public record GeoLocationDto
{
    public double Latitude { get; init; }
    public double Longitude { get; init; }
    public string? Address { get; init; }
    public string? Country { get; init; }
    public string? City { get; init; }
}
