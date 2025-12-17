using System.Windows.Controls;

namespace Themis.DocumentManager.Views
{
    /// <summary>
    /// AIChatView.xaml - VSCode-Style AI Chat Interface
    /// Displays messages with user/assistant role distinction
    /// Integrates with AIChatViewModel and Ollama backend
    /// </summary>
    public partial class AIChatView : UserControl
    {
        public AIChatView()
        {
            InitializeComponent();
        }
    }
}
