/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ApplicationStateService.cs                         ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     249                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Linq;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service zur Verwaltung des Anwendungszustands (Persistierung)
/// </summary>
public interface IApplicationStateService
{
    ApplicationState LoadState();
    void SaveState(ApplicationState state);
    string GetStateFilePath();
}

/// <summary>
/// Default-Implementierung des Application State Service
/// </summary>
public class ApplicationStateService : IApplicationStateService
{
    private readonly string _stateFilePath;
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
    };

    public ApplicationStateService()
    {
        var appDataPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
            "Themis", "DocumentManager"
        );
        
        if (!Directory.Exists(appDataPath))
        {
            Directory.CreateDirectory(appDataPath);
        }

        _stateFilePath = Path.Combine(appDataPath, "appstate.json");
    }

    public ApplicationState LoadState()
    {
        try
        {
            if (!File.Exists(_stateFilePath))
            {
                return new ApplicationState();
            }

            var json = File.ReadAllText(_stateFilePath);
            var state = JsonSerializer.Deserialize<ApplicationState>(json, JsonOptions);
            return state ?? new ApplicationState();
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Fehler beim Laden des Anwendungszustands: {ex.Message}");
            return new ApplicationState();
        }
    }

    public void SaveState(ApplicationState state)
    {
        try
        {
            var json = JsonSerializer.Serialize(state, JsonOptions);
            File.WriteAllText(_stateFilePath, json);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Fehler beim Speichern des Anwendungszustands: {ex.Message}");
        }
    }

    public string GetStateFilePath() => _stateFilePath;
}

/// <summary>
/// Datenklasse für den Anwendungszustand
/// </summary>
public class ApplicationState
{
    [JsonPropertyName("windowState")]
    public WindowStateInfo? WindowState { get; set; }

    [JsonPropertyName("theme")]
    public string Theme { get; set; } = "System";

    [JsonPropertyName("sidebarWidths")]
    public SidebarWidths? SidebarWidths { get; set; }

    [JsonPropertyName("openTabs")]
    public List<TabInfo> OpenTabs { get; set; } = new();

    [JsonPropertyName("selectedTabIndex")]
    public int SelectedTabIndex { get; set; } = 0;

    [JsonPropertyName("lastUser")]
    public string? LastUser { get; set; }

    [JsonPropertyName("preferences")]
    public UserPreferences? Preferences { get; set; }

    [JsonPropertyName("lastOpenedItems")]
    public List<string> LastOpenedItems { get; set; } = new();

    [JsonPropertyName("savedFilters")]
    public Dictionary<string, string> SavedFilters { get; set; } = new();

    [JsonPropertyName("favoriteTabs")]
    public List<string> FavoriteTabs { get; set; } = new();

    [JsonPropertyName("lastPreviewItem")]
    public string? LastPreviewItem { get; set; }
}

/// <summary>
/// Fenster-Zustands-Information
/// </summary>
public class WindowStateInfo
{
    [JsonPropertyName("isMaximized")]
    public bool IsMaximized { get; set; }

    [JsonPropertyName("isFullscreen")]
    public bool IsFullscreen { get; set; }

    [JsonPropertyName("left")]
    public double Left { get; set; } = 100;

    [JsonPropertyName("top")]
    public double Top { get; set; } = 100;

    [JsonPropertyName("width")]
    public double Width { get; set; } = 1200;

    [JsonPropertyName("height")]
    public double Height { get; set; } = 800;
}

/// <summary>
/// Sidebar-Breiten
/// </summary>
public class SidebarWidths
{
    [JsonPropertyName("leftSidebarWidth")]
    public double LeftSidebarWidth { get; set; } = 250;

    [JsonPropertyName("rightSidebarWidth")]
    public double RightSidebarWidth { get; set; } = 350;

    [JsonPropertyName("isLeftSidebarVisible")]
    public bool IsLeftSidebarVisible { get; set; } = true;

    [JsonPropertyName("isRightSidebarVisible")]
    public bool IsRightSidebarVisible { get; set; } = true;
}

/// <summary>
/// Tab-Information
/// </summary>
public class TabInfo
{
    [JsonPropertyName("name")]
    public string Name { get; set; } = "";

    [JsonPropertyName("header")]
    public string Header { get; set; } = "";

    [JsonPropertyName("contentType")]
    public string ContentType { get; set; } = "";

    [JsonPropertyName("contentData")]
    public string? ContentData { get; set; }

    [JsonPropertyName("isFavorite")]
    public bool IsFavorite { get; set; }

    [JsonPropertyName("isCloseable")]
    public bool IsCloseable { get; set; } = true;

    [JsonPropertyName("lastAccessed")]
    public DateTime LastAccessed { get; set; } = DateTime.Now;
}

/// <summary>
/// Benutzer-Präferenzen
/// </summary>
public class UserPreferences
{
    [JsonPropertyName("autoSave")]
    public bool AutoSave { get; set; } = true;

    [JsonPropertyName("autoSaveInterval")]
    public int AutoSaveIntervalSeconds { get; set; } = 60;

    [JsonPropertyName("showNotifications")]
    public bool ShowNotifications { get; set; } = true;

    [JsonPropertyName("defaultPreviewMode")]
    public string DefaultPreviewMode { get; set; } = "Split";

    [JsonPropertyName("enableAnimations")]
    public bool EnableAnimations { get; set; } = true;

    [JsonPropertyName("language")]
    public string Language { get; set; } = "de-DE";

    [JsonPropertyName("dateFormat")]
    public string DateFormat { get; set; } = "dd.MM.yyyy";

    [JsonPropertyName("timeFormat")]
    public string TimeFormat { get; set; } = "HH:mm:ss";

    [JsonPropertyName("rememberLastSession")]
    public bool RememberLastSession { get; set; } = true;
}
