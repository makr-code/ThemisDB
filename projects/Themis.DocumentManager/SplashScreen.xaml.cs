using System.Windows;

namespace Themis.DocumentManager;

public partial class SplashScreen : Window
{
    public SplashScreen()
    {
        InitializeComponent();
    }

    public void UpdateStatus(string status, double progress)
    {
        Dispatcher.Invoke(() =>
        {
            StatusText.Text = status;
            ProgressBar.Value = progress;
        });
    }
}
