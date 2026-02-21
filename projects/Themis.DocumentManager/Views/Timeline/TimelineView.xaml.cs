/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineView.xaml.cs                               ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     77                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
