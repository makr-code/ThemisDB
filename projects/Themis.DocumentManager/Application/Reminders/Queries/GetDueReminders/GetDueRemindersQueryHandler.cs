/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDueRemindersQueryHandler.cs                     ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-03-02 03:55:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
