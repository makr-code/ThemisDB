using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Cosigning.Messages;

namespace Themis.DocumentManager.Application.Cosigning.Commands.UpdateCosigning;

public record UpdateCosigningCommand : IRequest<Result<CosigningDto>>
{
    public string Id { get; init; } = string.Empty;
    public CosigningStatus? Status { get; init; }
    public string? SignatureData { get; init; }
    public string? Comment { get; init; }
    public string? RejectionReason { get; init; }
    public int? SignOrder { get; init; }
}
