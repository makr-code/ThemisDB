/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetClassificationByIdQueryHandler.cs               ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     76                                             ║
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
using Themis.DocumentManager.Application.Classification.Commands.CreateClassification;

namespace Themis.DocumentManager.Application.Classification.Queries.GetClassificationById;

public class GetClassificationByIdQueryHandler : IRequestHandler<GetClassificationByIdQuery, Result<ClassificationDto>>
{
    private static readonly Dictionary<string, ClassificationItem> _classifications = new();

    public async Task<Result<ClassificationDto>> Handle(GetClassificationByIdQuery request, CancellationToken cancellationToken)
    {
        try
        {
            if (!_classifications.ContainsKey(request.Id))
            {
                return Result<ClassificationDto>.Fail($"Classification mit ID {request.Id} wurde nicht gefunden");
            }

            var classification = _classifications[request.Id];

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
            return Result<ClassificationDto>.Fail($"Fehler beim Abrufen der Classification: {ex.Message}");
        }
    }
}
