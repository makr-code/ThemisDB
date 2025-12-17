using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Cosigning.Messages;
using Themis.DocumentManager.Application.Cosigning.Commands.CreateCosigning;

namespace Themis.DocumentManager.Application.Cosigning.Queries.GetCosigningById;

public class GetCosigningByIdQueryHandler : IRequestHandler<GetCosigningByIdQuery, Result<CosigningDto>>
{
    private static readonly Dictionary<string, object> _cosignings = CreateCosigningCommandHandler_GetStorage();

    private static Dictionary<string, object> CreateCosigningCommandHandler_GetStorage()
    {
        var field = typeof(CreateCosigningCommandHandler).GetField("_cosignings", 
            System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static);
        return (Dictionary<string, object>)field!.GetValue(null)!;
    }

    public async Task<Result<CosigningDto>> Handle(GetCosigningByIdQuery request, CancellationToken cancellationToken)
    {
        if (!_cosignings.TryGetValue(request.Id, out var item))
        {
            return await Task.FromResult(Result<CosigningDto>.Fail("Mitunterzeichnung nicht gefunden"));
        }

        var cosigning = (CosigningItem)item;

        var dto = new CosigningDto
        {
            Id = cosigning.Id,
            DocumentId = cosigning.DocumentId,
            DocumentName = cosigning.DocumentName,
            SignerId = cosigning.SignerId,
            SignerName = cosigning.SignerName,
            SignerEmail = cosigning.SignerEmail,
            Status = cosigning.Status,
            SignOrder = cosigning.SignOrder,
            SignatureData = cosigning.SignatureData,
            SignedAt = cosigning.SignedAt,
            RequestedAt = cosigning.RequestedAt,
            ReminderSentAt = cosigning.ReminderSentAt,
            Comment = cosigning.Comment,
            RejectionReason = cosigning.RejectionReason,
            RequiresComment = cosigning.RequiresComment,
            Type = cosigning.Type,
            Metadata = cosigning.Metadata,
            CreatedAt = cosigning.CreatedAt,
            CreatedBy = cosigning.CreatedBy,
            UpdatedAt = cosigning.UpdatedAt!,
            UpdatedBy = cosigning.UpdatedBy
        };

        return await Task.FromResult(Result<CosigningDto>.Ok(dto));
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
