/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllRemindersQueryHandler.cs                     ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Application.Common;
using Themis.DocumentManager.Application.Common.Queries;
using Themis.DocumentManager.Application.Reminders.Messages;
using Themis.DocumentManager.Application.Reminders.Queries.GetAllReminders;

namespace Themis.DocumentManager.Application.Reminders.Queries.GetAllReminders;

public class GetAllRemindersQueryHandler : IRequestHandler<GetAllRemindersQuery, Result<PagedResult<ReminderDto>>>
{
    public Task<Result<PagedResult<ReminderDto>>> Handle(GetAllRemindersQuery request, CancellationToken cancellationToken)
    {
        try
        {
            // In-memory retrieval with pagination
            var reminders = new List<ReminderDto>
            {
                new()
                {
                    Id = Guid.NewGuid().ToString(),
                    ProcessId = "PROC-001",
                    Subject = "Reminder 1",
                    DueDate = DateTime.Now.AddDays(7),
                    CreatedAt = DateTime.UtcNow
                }
            };

            var totalCount = reminders.Count;
            var skipCount = (request.PageNumber - 1) * request.PageSize;
            var pagedReminders = reminders
                .Skip(skipCount)
                .Take(request.PageSize)
                .ToList();

            var result = new PagedResult<ReminderDto>
            {
                Items = pagedReminders,
                TotalCount = totalCount,
                PageNumber = request.PageNumber,
                PageSize = request.PageSize
            };

            return Task.FromResult(Result<PagedResult<ReminderDto>>.Ok(result));
        }
        catch (Exception ex)
        {
            return Task.FromResult(Result<PagedResult<ReminderDto>>.Fail($"Error retrieving reminders: {ex.Message}"));
        }
    }
}
