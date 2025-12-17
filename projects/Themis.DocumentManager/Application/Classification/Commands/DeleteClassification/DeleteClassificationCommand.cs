using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Commands;

namespace Themis.DocumentManager.Application.Classification.Commands.DeleteClassification;

public record DeleteClassificationCommand : IDeleteCommand
{
    public string Id { get; init; } = string.Empty;
}
