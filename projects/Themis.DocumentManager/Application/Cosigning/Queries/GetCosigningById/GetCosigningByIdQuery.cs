using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Cosigning.Messages;

namespace Themis.DocumentManager.Application.Cosigning.Queries.GetCosigningById;

public record GetCosigningByIdQuery(string Id) : IRequest<Result<CosigningDto>>;
