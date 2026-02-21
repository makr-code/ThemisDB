/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DeleteTaskCommandHandler.cs                        ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Messages;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Tasks.Commands.DeleteTask;

public class DeleteTaskCommandHandler : IRequestHandler<DeleteTaskCommand, Result>
{
    // Temporary in-memory storage (replace with repository)
    private static readonly Dictionary<string, OutlookTask> _tasks = new();

    public async Task<Result> Handle(DeleteTaskCommand request, CancellationToken cancellationToken)
    {
        try
        {
            if (!_tasks.ContainsKey(request.Id))
            {
                return Result.Fail($"Task mit ID {request.Id} wurde nicht gefunden");
            }

            _tasks.Remove(request.Id);

            return await Task.FromResult(Result.Ok());
        }
        catch (Exception ex)
        {
            return Result.Fail($"Fehler beim Löschen der Task: {ex.Message}");
        }
    }
}
