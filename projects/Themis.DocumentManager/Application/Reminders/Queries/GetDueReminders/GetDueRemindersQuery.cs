/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GetDueRemindersQuery.cs                            ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     32                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using MediatR;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Application.Reminders.Queries.GetDueReminders;

/// <summary>
/// Query to get due reminders (Fällige Wiedervorlagen)
/// Based on PDV VIS Fristenmanagement requirements
/// </summary>
public record GetDueRemindersQuery : IRequest<List<Reminder>>
{
    public DateTime? UpToDate { get; init; }
}
