/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDueRemindersQueryHandler.cs                     ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     48                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 5de9cc853  2025-12-10  Integrate VIS features (Inbox, Reminders, Cosigning) into... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.Application.Reminders.Queries.GetDueReminders;

/// <summary>
/// Handler for GetDueRemindersQuery
/// </summary>
public class GetDueRemindersQueryHandler : IRequestHandler<GetDueRemindersQuery, List<Reminder>>
{
    private readonly IReminderService _reminderService;

    public GetDueRemindersQueryHandler(IReminderService reminderService)
    {
        _reminderService = reminderService;
    }

    public async Task<List<Reminder>> Handle(GetDueRemindersQuery request, CancellationToken cancellationToken)
    {
        var reminders = await _reminderService.GetDueRemindersAsync(request.UpToDate);
        return reminders.ToList();
    }
}
