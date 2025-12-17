using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Inbox.Messages;

namespace Themis.DocumentManager.Application.Inbox.Queries.GetInboxItemById;

public record GetInboxItemByIdQuery(string Id) : IRequest<Result<InboxItemDto>>;
