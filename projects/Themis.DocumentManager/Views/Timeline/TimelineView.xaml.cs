/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            TimelineView.xaml.cs                               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     70                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
