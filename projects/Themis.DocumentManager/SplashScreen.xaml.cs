/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SplashScreen.xaml.cs                               ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
