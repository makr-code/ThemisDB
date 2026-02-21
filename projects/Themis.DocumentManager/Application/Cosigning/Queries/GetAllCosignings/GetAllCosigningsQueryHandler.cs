/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllCosigningsQueryHandler.cs                    ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     100                                            ║
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

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Cosigning.Messages;
using Themis.DocumentManager.Application.Cosigning.Commands.CreateCosigning;

namespace Themis.DocumentManager.Application.Cosigning.Queries.GetAllCosignings;

public class GetAllCosigningsQueryHandler : IRequestHandler<GetAllCosigningsQuery, Result<PagedResult<CosigningDto>>>
{
    private static readonly Dictionary<string, CreateCosigningCommandHandler.CosigningItem> _cosignings = CreateCosigningCommandHandler.Cosignings;

    public async Task<Result<PagedResult<CosigningDto>>> Handle(GetAllCosigningsQuery request, CancellationToken cancellationToken)
    {
        var query = _cosignings.Values.AsQueryable();

        if (!string.IsNullOrWhiteSpace(request.DocumentId))
            query = query.Where(c => c.DocumentId == request.DocumentId);

        if (!string.IsNullOrWhiteSpace(request.SignerId))
            query = query.Where(c => c.SignerId == request.SignerId);

        if (request.Status.HasValue)
            query = query.Where(c => c.Status == request.Status.Value);

        if (request.Type.HasValue)
            query = query.Where(c => c.Type == request.Type.Value);

        if (request.IsPending.HasValue && request.IsPending.Value)
            query = query.Where(c => c.Status == CosigningStatus.Pending);

        var totalCount = query.Count();
        var items = query
            .OrderBy(c => c.SignOrder)
            .ThenBy(c => c.RequestedAt)
            .Skip((request.PageNumber - 1) * request.PageSize)
            .Take(request.PageSize)
            .Select(c => new CosigningDto
            {
                Id = c.Id ?? string.Empty,
                DocumentId = c.DocumentId ?? string.Empty,
                DocumentName = c.DocumentName ?? string.Empty,
                SignerId = c.SignerId ?? string.Empty,
                SignerName = c.SignerName ?? string.Empty,
                SignerEmail = c.SignerEmail,
                Status = c.Status,
                SignOrder = c.SignOrder,
                SignatureData = c.SignatureData,
                SignedAt = c.SignedAt,
                RequestedAt = c.RequestedAt,
                ReminderSentAt = c.ReminderSentAt,
                Comment = c.Comment,
                RejectionReason = c.RejectionReason,
                RequiresComment = c.RequiresComment,
                Type = c.Type,
                Metadata = c.Metadata ?? new(),
                CreatedAt = c.CreatedAt,
                CreatedBy = c.CreatedBy ?? "System",
                UpdatedAt = c.UpdatedAt ?? c.CreatedAt,
                UpdatedBy = c.UpdatedBy ?? c.CreatedBy ?? "System"
            })
            .ToList();

        var result = new PagedResult<CosigningDto>
        {
            Items = items,
            TotalCount = totalCount,
            PageNumber = request.PageNumber,
            PageSize = request.PageSize
        };

        return await Task.FromResult(Result<PagedResult<CosigningDto>>.Ok(result));
    }
}
