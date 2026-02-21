/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MessengerModels.cs                                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:39:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     162                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
    • 94f72fb1c  2025-12-07  Add seamless messenger integration (WhatsApp, Jabber, Tea... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Models;

public class MessengerIntegration
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public MessengerPlatform Platform { get; set; }
    public MessengerConnectionSettings Settings { get; set; } = new();
    public bool IsConnected { get; set; }
    public DateTime? LastSync { get; set; }
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    public string CreatedBy { get; set; } = string.Empty;
}

public enum MessengerPlatform
{
    WhatsApp,
    Jabber,
    MicrosoftTeams,
    Slack,
    Signal,
    Telegram,
    Matrix,
    RocketChat,
    Mattermost
}

public class MessengerConnectionSettings
{
    public string? ApiKey { get; set; }
    public string? ApiSecret { get; set; }
    public string? WebhookUrl { get; set; }
    public string? BotToken { get; set; }
    public string? PhoneNumberId { get; set; }
    public string? ServerUrl { get; set; }
    public Dictionary<string, string> CustomSettings { get; set; } = new();
}

public class MessengerMessage
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public MessengerPlatform Platform { get; set; }
    public string ChatId { get; set; } = string.Empty;
    public string From { get; set; } = string.Empty;
    public string? FromName { get; set; }
    public string To { get; set; } = string.Empty;
    public string Body { get; set; } = string.Empty;
    public List<MessengerAttachment> Attachments { get; set; } = new();
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
    public MessageDirection Direction { get; set; }
    public string? ExtractedProcessId { get; set; }
    public string? ExtractedFileReference { get; set; }
    public bool AutoLinked { get; set; }
    public string? ThreadId { get; set; }
    public MessageStatus Status { get; set; }
}

public enum MessageDirection
{
    Inbound,
    Outbound
}

public enum MessageStatus
{
    Sent,
    Delivered,
    Read,
    Failed
}

public class MessengerAttachment
{
    public string Filename { get; set; } = string.Empty;
    public string MimeType { get; set; } = string.Empty;
    public long SizeBytes { get; set; }
    public string? Url { get; set; }
    public string? DocumentId { get; set; }
}

public class MessengerBot
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public MessengerPlatform Platform { get; set; }
    public string BotName { get; set; } = string.Empty;
    public List<BotCommand> Commands { get; set; } = new();
    public bool AutoRespond { get; set; }
    public string? DefaultResponse { get; set; }
}

public class BotCommand
{
    public string Command { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public BotCommandAction Action { get; set; }
    public List<string> Aliases { get; set; } = new();
}

public enum BotCommandAction
{
    ShowProcessStatus,
    SearchProcesses,
    CreateProcess,
    AssignProcess,
    SetReminder,
    UploadDocument,
    ShowTimeline,
    ShowHelp,
    SetNotification,
    GetStatistics
}

public class ChatThread
{
    public string Id { get; set; } = Guid.NewGuid().ToString();
    public MessengerPlatform Platform { get; set; }
    public string ChatId { get; set; } = string.Empty;
    public string? ProcessId { get; set; }
    public List<MessengerMessage> Messages { get; set; } = new();
    public DateTime StartedAt { get; set; } = DateTime.UtcNow;
    public DateTime? ClosedAt { get; set; }
}

public class MessengerStatistics
{
    public MessengerPlatform Platform { get; set; }
    public int TotalMessages { get; set; }
    public int InboundMessages { get; set; }
    public int OutboundMessages { get; set; }
    public int AutoLinkedMessages { get; set; }
    public int BotCommandsExecuted { get; set; }
    public TimeSpan AverageResponseTime { get; set; }
    public DateTime PeriodStart { get; set; }
    public DateTime PeriodEnd { get; set; }
}
