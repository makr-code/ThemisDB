using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Cosigning.Messages;
using Themis.DocumentManager.Application.Cosigning.Commands.CreateCosigning;

namespace Themis.DocumentManager.Application.Cosigning.Queries.GetAllCosignings;

public class GetAllCosigningsQueryHandler : IRequestHandler<GetAllCosigningsQuery, Result<PagedResult<CosigningDto>>>
{
    private static readonly Dictionary<string, object> _cosignings = CreateCosigningCommandHandler_GetStorage();

    private static Dictionary<string, object> CreateCosigningCommandHandler_GetStorage()
    {
        var field = typeof(CreateCosigningCommandHandler).GetField("_cosignings", 
            System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static);
        return (Dictionary<string, object>)field!.GetValue(null)!;
    }

    public async Task<Result<PagedResult<CosigningDto>>> Handle(GetAllCosigningsQuery request, CancellationToken cancellationToken)
    {
        var allItems = _cosignings.Values.Select(x => (CosigningItem)x).ToList();
        var query = allItems.AsQueryable();

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
                Id = c.Id,
                DocumentId = c.DocumentId,
                DocumentName = c.DocumentName,
                SignerId = c.SignerId,
                SignerName = c.SignerName,
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
                Metadata = c.Metadata,
                CreatedAt = c.CreatedAt,
                CreatedBy = c.CreatedBy,
                UpdatedAt = c.UpdatedAt!,
                UpdatedBy = c.UpdatedBy
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

    private class CosigningItem
    {
        public string Id { get; set; } = string.Empty;
        public string DocumentId { get; set; } = string.Empty;
        public string DocumentName { get; set; } = string.Empty;
        public string SignerId { get; set; } = string.Empty;
        public string SignerName { get; set; } = string.Empty;
        public string? SignerEmail { get; set; }
        public CosigningStatus Status { get; set; }
        public int SignOrder { get; set; }
        public string? SignatureData { get; set; }
        public DateTime? SignedAt { get; set; }
        public DateTime? RequestedAt { get; set; }
        public DateTime? ReminderSentAt { get; set; }
        public string? Comment { get; set; }
        public string? RejectionReason { get; set; }
        public bool RequiresComment { get; set; }
        public CosigningType Type { get; set; }
        public Dictionary<string, object> Metadata { get; set; } = new();
        public DateTime CreatedAt { get; set; }
        public string? CreatedBy { get; set; }
        public DateTime? UpdatedAt { get; set; }
        public string? UpdatedBy { get; set; }
    }
}
