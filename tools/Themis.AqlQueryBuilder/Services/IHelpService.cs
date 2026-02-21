/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IHelpService.cs                                    ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:18:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
