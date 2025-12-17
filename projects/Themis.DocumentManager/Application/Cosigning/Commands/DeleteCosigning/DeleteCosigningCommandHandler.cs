using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Cosigning.Commands.CreateCosigning;

namespace Themis.DocumentManager.Application.Cosigning.Commands.DeleteCosigning;

public class DeleteCosigningCommandHandler : IRequestHandler<DeleteCosigningCommand, Result<bool>>
{
    private static readonly Dictionary<string, object> _cosignings = CreateCosigningCommandHandler_GetStorage();

    private static Dictionary<string, object> CreateCosigningCommandHandler_GetStorage()
    {
        var field = typeof(CreateCosigningCommandHandler).GetField("_cosignings", 
            System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static);
        return (Dictionary<string, object>)field!.GetValue(null)!;
    }

    public async Task<Result<bool>> Handle(DeleteCosigningCommand request, CancellationToken cancellationToken)
    {
        if (!_cosignings.ContainsKey(request.Id))
        {
            return await Task.FromResult(Result<bool>.Fail("Mitunterzeichnung nicht gefunden"));
        }

        _cosignings.Remove(request.Id);
        return await Task.FromResult(Result<bool>.Ok(true));
    }
}
