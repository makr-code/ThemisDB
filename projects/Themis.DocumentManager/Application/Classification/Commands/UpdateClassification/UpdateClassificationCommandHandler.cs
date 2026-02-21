/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UpdateClassificationCommandHandler.cs              ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Application.Classification.Commands.CreateClassification;

namespace Themis.DocumentManager.Application.Classification.Commands.UpdateClassification;

public class UpdateClassificationCommandHandler : IRequestHandler<UpdateClassificationCommand, Result>
{
    private static readonly Dictionary<string, ClassificationItem> _classifications = new();

    public async Task<Result> Handle(UpdateClassificationCommand request, CancellationToken cancellationToken)
    {
        try
        {
            if (!_classifications.ContainsKey(request.Id))
            {
                return Result.Fail($"Classification mit ID {request.Id} wurde nicht gefunden");
            }

            var classification = _classifications[request.Id];

            // Update only provided fields
            if (request.Name != null) classification.Name = request.Name;
            if (request.Description != null) classification.Description = request.Description;
            if (request.Code != null) classification.Code = request.Code;
            if (request.Level.HasValue) classification.Level = request.Level.Value;
            if (request.Color != null) classification.Color = request.Color;
            if (request.SortOrder.HasValue) classification.SortOrder = request.SortOrder.Value;
            if (request.IsActive.HasValue) classification.IsActive = request.IsActive.Value;
            if (request.AllowedRoles != null) classification.AllowedRoles = request.AllowedRoles;
            if (request.Metadata != null) classification.Metadata = request.Metadata;

            classification.ModifiedAt = DateTime.UtcNow;

            return await Task.FromResult(Result.Ok());
        }
        catch (Exception ex)
        {
            return Result.Fail($"Fehler beim Aktualisieren der Classification: {ex.Message}");
        }
    }
}
