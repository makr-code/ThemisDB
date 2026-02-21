/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SettingsService.cs                                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     192                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8a8fc2f70  2025-12-17  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services
{
    /// <summary>
    /// Service für persistente Anwendungseinstellungen.
    /// Phase 28 - Settings & Persistence System.
    /// </summary>
    public class SettingsService : ISettingsService
    {
        private readonly string _settingsFilePath;
        private Dictionary<string, object> _settings;

        public SettingsService()
        {
            var appDataPath = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            var themisPath = Path.Combine(appDataPath, "ThemisDB", "DocumentManager");

            if (!Directory.Exists(themisPath))
            {
                Directory.CreateDirectory(themisPath);
            }

            _settingsFilePath = Path.Combine(themisPath, "settings.json");
            _settings = new Dictionary<string, object>();

            Load();
        }

        /// <summary>
        /// Lädt Einstellungen von Disk.
        /// </summary>
        public void Load()
        {
            try
            {
                if (File.Exists(_settingsFilePath))
                {
                    var json = File.ReadAllText(_settingsFilePath);
                    _settings = JsonSerializer.Deserialize<Dictionary<string, object>>(json)
                               ?? new Dictionary<string, object>();
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Fehler beim Laden der Einstellungen: {ex.Message}");
                _settings = new Dictionary<string, object>();
            }
        }

        /// <summary>
        /// Speichert Einstellungen auf Disk.
        /// </summary>
        public void Save()
        {
            try
            {
                var json = JsonSerializer.Serialize(_settings, new JsonSerializerOptions
                {
                    WriteIndented = true
                });

                File.WriteAllText(_settingsFilePath, json);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"Fehler beim Speichern der Einstellungen: {ex.Message}");
            }
        }

        /// <summary>
        /// Holt einen Setting-Wert (mit Default).
        /// </summary>
        public T GetSetting<T>(string key, T defaultValue)
        {
            if (_settings.TryGetValue(key, out var value))
            {
                try
                {
                    // Handle JsonElement conversion for deserialized values
                    if (value is JsonElement element)
                    {
                        return JsonSerializer.Deserialize<T>(element.GetRawText()) ?? defaultValue;
                    }

                    return (T)Convert.ChangeType(value, typeof(T));
                }
                catch
                {
                    return defaultValue;
                }
            }

            return defaultValue;
        }

        /// <summary>
        /// Setzt einen Setting-Wert.
        /// </summary>
        public void SetSetting<T>(string key, T value)
        {
            if (value == null)
            {
                _settings.Remove(key);
            }
            else
            {
                _settings[key] = value;
            }
        }

        /// <summary>
        /// Prüft, ob ein Setting existiert.
        /// </summary>
        public bool HasSetting(string key)
        {
            return _settings.ContainsKey(key);
        }

        /// <summary>
        /// Entfernt ein Setting.
        /// </summary>
        public void RemoveSetting(string key)
        {
            _settings.Remove(key);
        }

        /// <summary>
        /// Setzt alle Einstellungen zurück.
        /// </summary>
        public void Reset()
        {
            _settings.Clear();
            Save();
        }

        public TreeViewSettings? LoadTreeViewSettings()
        {
            return GetSetting<TreeViewSettings?>("TreeViewSettings", default);
        }

        public void SaveTreeViewSettings(TreeViewSettings settings)
        {
            SetSetting("TreeViewSettings", settings);
            Save();
        }
    }

    public interface ISettingsService
    {
        void Load();
        void Save();
        T GetSetting<T>(string key, T defaultValue);
        void SetSetting<T>(string key, T value);
        bool HasSetting(string key);
        void RemoveSetting(string key);
        void Reset();

        TreeViewSettings? LoadTreeViewSettings();
        void SaveTreeViewSettings(TreeViewSettings settings);
    }
}
