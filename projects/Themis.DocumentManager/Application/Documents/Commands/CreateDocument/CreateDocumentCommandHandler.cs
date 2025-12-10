using MediatR;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Domain.Events;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Documents.Commands.CreateDocument;

/// <summary>
/// Handler for CreateDocumentCommand
/// </summary>
public class CreateDocumentCommandHandler : IRequestHandler<CreateDocumentCommand, string>
{
    private readonly IThemisRepository _repository;
    private readonly IMediator _mediator;

    public CreateDocumentCommandHandler(IThemisRepository repository, IMediator mediator)
    {
        _repository = repository;
        _mediator = mediator;
    }

    public async Task<string> Handle(CreateDocumentCommand request, CancellationToken cancellationToken)
    {
        var document = new Document
        {
            Id = Guid.NewGuid().ToString(),
            Title = request.Title,
            Description = request.Description,
            MimeType = request.MimeType,
            Filename = request.Filename,
            SizeBytes = request.SizeBytes,
            Author = request.Author,
            ContentPreview = request.ContentPreview,
            Category = request.Category,
            CreatedAt = DateTime.UtcNow,
            ModifiedAt = DateTime.UtcNow,
            Metadata = request.Metadata ?? new Dictionary<string, object>(),
            Tags = request.Tags ?? new List<string>()
        };

        var documentId = await _repository.CreateDocumentAsync(document, cancellationToken);

        // Publish domain event
        await _mediator.Publish(new DocumentCreatedEvent(documentId, document.Title, document.CreatedAt), cancellationToken);

        return documentId;
    }
}
