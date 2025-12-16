using Themis.DocumentManager.Services;
using Themis.DocumentManager.Application.Common.Interfaces;
using Themis.DocumentManager.Features.Graph.Services;
using System.Windows.Controls;

namespace Themis.DocumentManager.Features.AIChat.Views
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


