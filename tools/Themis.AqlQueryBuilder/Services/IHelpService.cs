/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IHelpService.cs                                    ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-21 18:23:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows.Controls;

namespace Themis.AqlQueryBuilder.Services;

/// <summary>
/// Service for providing context-sensitive help and tutorials
/// </summary>
public interface IHelpService
{
    /// <summary>
    /// Get help content for a specific context
    /// </summary>
    string GetContextHelp(string context);

    /// <summary>
    /// Show tutorial overlay for a specific feature
    /// </summary>
    void ShowTutorial(string tutorialId);

    /// <summary>
    /// Create a rich tooltip with title and content
    /// </summary>
    ToolTip CreateRichToolTip(string title, string content, string? shortcut = null);

    /// <summary>
    /// Get keyboard shortcut for an action
    /// </summary>
    string GetShortcut(string action);
}
