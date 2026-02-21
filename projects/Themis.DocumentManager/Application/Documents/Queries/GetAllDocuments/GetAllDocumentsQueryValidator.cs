/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllDocumentsQueryValidator.cs                   ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:22:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     45                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
