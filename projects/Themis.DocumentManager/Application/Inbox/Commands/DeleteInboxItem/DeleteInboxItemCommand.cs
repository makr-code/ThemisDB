using MediatR;
using Themis.DocumentManager.Application.Common;

namespace Themis.DocumentManager.Application.Inbox.Commands.DeleteInboxItem;

public record DeleteInboxItemCommand(string Id) : IRequest<Result<bool>>;
