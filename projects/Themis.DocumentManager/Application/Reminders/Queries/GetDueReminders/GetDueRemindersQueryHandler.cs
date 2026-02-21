/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDueRemindersQueryHandler.cs                     ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     50                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
