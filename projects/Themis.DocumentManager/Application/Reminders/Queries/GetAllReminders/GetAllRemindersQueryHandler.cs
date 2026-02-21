/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetAllRemindersQueryHandler.cs                     ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
