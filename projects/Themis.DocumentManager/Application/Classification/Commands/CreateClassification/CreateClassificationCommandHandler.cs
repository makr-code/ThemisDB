/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CreateClassificationCommandHandler.cs              ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   87.0/100                                       ║
    • Total Lines:     108                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Classification.Messages;

namespace Themis.DocumentManager.Application.Classification.Commands.CreateClassification;

public class CreateClassificationCommandHandler : IRequestHandler<CreateClassificationCommand, Result<ClassificationDto>>
{
    // Temporary in-memory storage (replace with repository)
    private static readonly Dictionary<string, ClassificationItem> _classifications = new();

    public async Task<Result<ClassificationDto>> Handle(CreateClassificationCommand request, CancellationToken cancellationToken)
    {
        try
        {
            var classification = new ClassificationItem
            {
                Id = Guid.NewGuid().ToString(),
                Name = request.Name,
                Description = request.Description,
                Code = request.Code,
                Level = request.Level,
                Color = request.Color,
                SortOrder = request.SortOrder,
                IsActive = true,
                ParentId = request.ParentId,
                AllowedRoles = request.AllowedRoles ?? new List<string>(),
                Metadata = request.Metadata ?? new Dictionary<string, object>(),
                CreatedAt = DateTime.UtcNow,
                CreatedBy = "System"
            };

            _classifications[classification.Id] = classification;

            var dto = new ClassificationDto
            {
                Id = classification.Id,
                Name = classification.Name,
                Description = classification.Description,
                Code = classification.Code,
                Level = classification.Level,
                Color = classification.Color,
                SortOrder = classification.SortOrder,
                IsActive = classification.IsActive,
                ParentId = classification.ParentId,
                AllowedRoles = classification.AllowedRoles,
                Metadata = classification.Metadata,
                CreatedAt = classification.CreatedAt,
                CreatedBy = classification.CreatedBy ?? string.Empty,
                UpdatedAt = classification.ModifiedAt,
                UpdatedBy = classification.ModifiedBy ?? string.Empty
            };

            return await Task.FromResult(Result<ClassificationDto>.Ok(dto));
        }
        catch (Exception ex)
        {
            return Result<ClassificationDto>.Fail($"Fehler beim Erstellen der Classification: {ex.Message}");
        }
    }
}

// Temporary model (should be in Models namespace)
internal class ClassificationItem
{
    public string Id { get; set; } = string.Empty;
    public string Name { get; set; } = string.Empty;
    public string? Description { get; set; }
    public string? Code { get; set; }
    public ClassificationLevel Level { get; set; }
    public string? Color { get; set; }
    public int SortOrder { get; set; }
    public bool IsActive { get; set; }
    public string? ParentId { get; set; }
    public List<string> AllowedRoles { get; set; } = new();
    public Dictionary<string, object> Metadata { get; set; } = new();
    public DateTime CreatedAt { get; set; }
    public string? CreatedBy { get; set; }
    public DateTime ModifiedAt { get; set; }
    public string? ModifiedBy { get; set; }
}
