/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SplashScreen.xaml.cs                               ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:59:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 362722340  2025-12-12  chore: workspace reorganization and build system consolid... ║
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
