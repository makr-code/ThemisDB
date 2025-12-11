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
