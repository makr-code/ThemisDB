/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineView.xaml.cs                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:40:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     77                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 085cb30ce  2025-12-11  Integrate DSM services and add benchmark automation ║
    • 36820014e  2025-12-08  Refactor: move Themis.DocumentManager to projects dir ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Windows;
using System.Windows.Controls;

namespace Themis.DocumentManager.Views.Timeline;

public partial class TimelineView : UserControl
{
    public TimelineView()
    {
        InitializeComponent();
    }

    /// <summary>
    /// Handler für View-Mode-Umschaltung zwischen Liste und Gantt
    /// </summary>
    private void ViewMode_Changed(object sender, RoutedEventArgs e)
    {
        if (sender == ContentViewModeList && ContentViewModeList.IsChecked == true)
        {
            // Liste-Modus: deaktiviere Gantt
            ContentViewModeGantt.IsChecked = false;
            ListViewContent.Visibility = Visibility.Visible;
            GanttOnlyPlaceholder.Visibility = Visibility.Collapsed;
            // Gantt-Balken können trotzdem angezeigt werden
        }
        else if (sender == ContentViewModeGantt && ContentViewModeGantt.IsChecked == true)
        {
            // Gantt-Modus: deaktiviere Liste
            ContentViewModeList.IsChecked = false;
            ListViewContent.Visibility = Visibility.Collapsed;
            GanttOnlyPlaceholder.Visibility = Visibility.Visible;
        }
    }

    /// <summary>
    /// Handler für Gantt-Balken Ein-/Ausblenden
    /// </summary>
    private void ShowGanttBars_Changed(object sender, RoutedEventArgs e)
    {
        if (ShowGanttBars.IsChecked == true)
        {
            // Zeige Gantt-Bars
            GanttBarsContainer.Visibility = Visibility.Visible;
        }
        else
        {
            // Verstecke Gantt-Bars
            GanttBarsContainer.Visibility = Visibility.Collapsed;
        }
    }
}
