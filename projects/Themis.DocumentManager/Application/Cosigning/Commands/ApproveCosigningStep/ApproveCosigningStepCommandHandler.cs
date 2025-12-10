using MediatR;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Application.Cosigning.Commands.ApproveCosigningStep;

/// <summary>
/// Handler for ApproveCosigningStepCommand
/// Implements VIS Mitzeichnung functionality
/// </summary>
public class ApproveCosigningStepCommandHandler : IRequestHandler<ApproveCosigningStepCommand, bool>
{
    private readonly ICosigningService _cosigningService;

    public ApproveCosigningStepCommandHandler(ICosigningService cosigningService)
    {
        _cosigningService = cosigningService;
    }

    public async Task<bool> Handle(ApproveCosigningStepCommand request, CancellationToken cancellationToken)
    {
        return await _cosigningService.ApproveCosigningStepAsync(
            request.CosigningId,
            request.CosignerId,
            request.Comment ?? string.Empty);
    }
}
