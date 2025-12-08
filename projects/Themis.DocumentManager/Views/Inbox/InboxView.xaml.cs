using System.Windows;
using System.Windows.Controls;

namespace Themis.DocumentManager.Views.Inbox
{
    public partial class InboxView : UserControl
    {
        public InboxView()
        {
            InitializeComponent();
        }

        private void OnCreateNewClick(object sender, RoutedEventArgs e)
        {
            MessageBox.Show("Neuer Posteingangseintrag erstellen");
        }

        private void OnSettingsClick(object sender, RoutedEventArgs e)
        {
            // Settings
        }

        private void OnRefreshClick(object sender, RoutedEventArgs e)
        {
            // Refresh
        }

        private void OnStatisticsClick(object sender, RoutedEventArgs e)
        {
            // Statistics
        }
    }
}
