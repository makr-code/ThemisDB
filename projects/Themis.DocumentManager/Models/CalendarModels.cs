/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            CalendarModels.cs                                  ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     152                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

public class CalendarIntegration
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public CalendarPlatform Platform { get; set; }
    public bool AutoSync { get; set; } = true;
    public CalendarSyncSettings Settings { get; set; } = new();
    public DateTime? LastSync { get; set; }
    public string UserId { get; set; } = string.Empty;
}

public enum CalendarPlatform
{
    OutlookCalendar,
    GoogleCalendar,
    ICloudCalendar,
    Exchange,
    CalDAV
}

public class CalendarSyncSettings
{
    public SyncDirection SyncDirection { get; set; } = SyncDirection.Bidirectional;
    public int SyncFrequencyMinutes { get; set; } = 5;
    public bool CreateEventsForDeadlines { get; set; } = true;
    public bool CreateEventsForMeetings { get; set; } = true;
    public bool ProcessReferencesInSubject { get; set; } = true;
    public bool SyncReminders { get; set; } = true;
    public bool SyncTasks { get; set; } = true;
}

public enum SyncDirection
{
    OneWayToCalendar,
    OneWayFromCalendar,
    Bidirectional
}

public class CalendarEvent
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public string Subject { get; set; } = string.Empty;
    public string Body { get; set; } = string.Empty;
    public DateTime StartTime { get; set; }
    public DateTime EndTime { get; set; }
    public string? Location { get; set; }
    public List<string> Attendees { get; set; } = new();
    public string? Organizer { get; set; }
    public string? ProcessId { get; set; }
    public string? FileReference { get; set; }
    public EventStatus Status { get; set; }
    public List<EventReminder> Reminders { get; set; } = new();
    public bool IsAllDay { get; set; }
    public RecurrencePattern? Recurrence { get; set; }
    public List<MeetingResponse> Responses { get; set; } = new();
}

public enum EventStatus
{
    Tentative,
    Confirmed,
    Cancelled
}

public class EventReminder
{
    public int MinutesBeforeStart { get; set; }
    public ReminderMethod Method { get; set; }
}

public enum ReminderMethod
{
    Popup,
    Email,
    SMS
}

public class RecurrencePattern
{
    public RecurrenceType Type { get; set; } = RecurrenceType.Once;
    public int Interval { get; set; } = 1;
    public List<DayOfWeek> DaysOfWeek { get; set; } = new();
    public int? DayOfMonth { get; set; }
    public DateTime? EndDate { get; set; }
    public int? MaxOccurrences { get; set; }
}

public enum RecurrenceType
{
    Once,
    Daily,
    Weekly,
    Monthly,
    Yearly
}

public class MeetingResponse
{
    public string AttendeeEmail { get; set; } = string.Empty;
    public string? AttendeeName { get; set; }
    public ResponseStatus Response { get; set; }
    public DateTime ResponseTime { get; set; }
    public string? Comment { get; set; }
}

public enum ResponseStatus
{
    None,
    Accepted,
    Declined,
    Tentative
}

public class CalendarSyncStatus
{
    public DateTime LastSync { get; set; }
    public int SyncedEvents { get; set; }
    public int SyncedTasks { get; set; }
    public int Conflicts { get; set; }
    public DateTime NextSync { get; set; }
    public List<string> Errors { get; set; } = new();
}
