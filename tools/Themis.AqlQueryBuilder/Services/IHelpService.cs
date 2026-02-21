/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            IHelpService.cs                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     29                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
