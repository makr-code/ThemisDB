using FluentValidation;

namespace Themis.DocumentManager.Application.Documents.Queries.GetAllDocuments;

/// <summary>
/// Validator for GetAllDocumentsQuery
/// </summary>
public class GetAllDocumentsQueryValidator : AbstractValidator<GetAllDocumentsQuery>
{
    public GetAllDocumentsQueryValidator()
    {
        RuleFor(x => x.PageNumber)
            .GreaterThan(0).WithMessage("Seitennummer muss größer als 0 sein");

        RuleFor(x => x.PageSize)
            .GreaterThan(0).WithMessage("Seitengröße muss größer als 0 sein")
            .LessThanOrEqualTo(1000).WithMessage("Seitengröße darf maximal 1000 sein");
    }
}
