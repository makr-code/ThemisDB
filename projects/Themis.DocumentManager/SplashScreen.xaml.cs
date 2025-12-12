using System.Windows;
using System.Windows.Media.Imaging;
using System.IO;

namespace Themis.DocumentManager;

public partial class SplashScreen : Window
{
    public SplashScreen()
    {
        InitializeComponent();
        LoadLogoImage();
    }

    private void LoadLogoImage()
    {
        try
        {
            // Lade ThemisDB Logo
            var uri = new Uri("pack://application:,,,/Resources/themisdb_80.png");
            var bitmap = new BitmapImage(uri);
            
            LogoImage.Source = bitmap;
            LogoImage.Visibility = Visibility.Visible;
            EmojiLogo.Visibility = Visibility.Collapsed;
        }
        catch
        {
            // Fallback auf Emoji wenn Datei nicht existiert
            LogoImage.Visibility = Visibility.Collapsed;
            EmojiLogo.Visibility = Visibility.Visible;
        }
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
