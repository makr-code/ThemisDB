/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            KeyboardShortcutService.cs                         ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     74                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Windows.Input;

namespace Themis.DocumentManager.Services
{
    public class KeyboardShortcutService
    {
        private static KeyboardShortcutService? _instance;
        public static KeyboardShortcutService Instance => _instance ??= new KeyboardShortcutService();
        public event EventHandler? ShortcutsChanged;

        public Dictionary<string, KeyGesture> Shortcuts { get; private set; } = new Dictionary<string, KeyGesture>();

        public KeyboardShortcutService()
        {
            LoadDefaults();
        }

        public void LoadDefaults()
        {
            Shortcuts["SaveMetadata"] = new KeyGesture(Key.S, ModifierKeys.Control);
            Shortcuts["ReloadMetadata"] = new KeyGesture(Key.F5);
            Shortcuts["FinalizeMetadata"] = new KeyGesture(Key.F, ModifierKeys.Control);
            Shortcuts["OpenSettings"] = new KeyGesture(Key.OemComma, ModifierKeys.Control);
            Shortcuts["NextTab"] = new KeyGesture(Key.Tab, ModifierKeys.Control);
            Shortcuts["PreviousTab"] = new KeyGesture(Key.Tab, ModifierKeys.Control | ModifierKeys.Shift);
            Shortcuts["OpenSearch"] = new KeyGesture(Key.F, ModifierKeys.Control);
            Shortcuts["ToggleTheme"] = new KeyGesture(Key.T, ModifierKeys.Control | ModifierKeys.Shift);
            Shortcuts["CloseTab"] = new KeyGesture(Key.W, ModifierKeys.Control);
            Shortcuts["DuplicateTab"] = new KeyGesture(Key.D, ModifierKeys.Control);
            Shortcuts["SwitchSidebarGraph"] = new KeyGesture(Key.G, ModifierKeys.Control | ModifierKeys.Shift);
            Shortcuts["SwitchSidebarMap"] = new KeyGesture(Key.M, ModifierKeys.Control | ModifierKeys.Shift);
            Shortcuts["OpenTabInNewWindow"] = new KeyGesture(Key.N, ModifierKeys.Control | ModifierKeys.Shift);
            Shortcuts["FavoriteAdd"] = new KeyGesture(Key.F, ModifierKeys.Control | ModifierKeys.Shift);
            Shortcuts["FavoriteRemove"] = new KeyGesture(Key.R, ModifierKeys.Control | ModifierKeys.Shift);
            Shortcuts["CloseOthers"] = new KeyGesture(Key.K, ModifierKeys.Control | ModifierKeys.Shift);
            Shortcuts["SearchTabs"] = new KeyGesture(Key.P, ModifierKeys.Control);
            Shortcuts["ShowShortcutsOverlay"] = new KeyGesture(Key.OemQuestion, ModifierKeys.Control);
        }

        public void UpdateShortcut(string name, KeyGesture gesture)
        {
            Shortcuts[name] = gesture;
            ShortcutsChanged?.Invoke(this, EventArgs.Empty);
        }
    }
}