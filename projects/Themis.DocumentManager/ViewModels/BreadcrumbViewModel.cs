/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            BreadcrumbViewModel.cs                             ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     246                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace Themis.DocumentManager.ViewModels
{
    public partial class BreadcrumbViewModel : ObservableObject
    {
        [ObservableProperty]
        private ObservableCollection<BreadcrumbItem> breadcrumbPath = new();

        [ObservableProperty]
        private BreadcrumbItem? currentItem;

        public BreadcrumbViewModel()
        {
            InitializeDefaultPath();
        }

        private void InitializeDefaultPath()
        {
            BreadcrumbPath = new ObservableCollection<BreadcrumbItem>
            {
                new BreadcrumbItem 
                { 
                    Icon = "🏛️", 
                    Title = "Stadtverwaltung München", 
                    Level = BreadcrumbLevel.Authority,
                    SuggestedItems = new ObservableCollection<string> 
                    { 
                        "Landratsamt", 
                        "Bezirksregierung" 
                    }
                },
                new BreadcrumbItem 
                { 
                    Icon = "📁", 
                    Title = "Bauamt", 
                    Level = BreadcrumbLevel.Repository,
                    SuggestedItems = new ObservableCollection<string> 
                    { 
                        "Rechtsamt", 
                        "Ordnungsamt", 
                        "Umweltamt" 
                    }
                },
                new BreadcrumbItem 
                { 
                    Icon = "📂", 
                    Title = "Baugenehmigungen 2025", 
                    Level = BreadcrumbLevel.File,
                    SuggestedItems = new ObservableCollection<string> 
                    { 
                        "Baugenehmigungen 2024", 
                        "Abbruchgenehmigungen 2025" 
                    }
                },
                new BreadcrumbItem 
                { 
                    Icon = "📋", 
                    Title = "Antrag Mustermann", 
                    Level = BreadcrumbLevel.Process,
                    SuggestedItems = new ObservableCollection<string> 
                    { 
                        "Antrag Schmidt", 
                        "Antrag Weber" 
                    }
                },
                new BreadcrumbItem 
                { 
                    Icon = "📄", 
                    Title = "Bauplan_Entwurf_v2.pdf", 
                    Level = BreadcrumbLevel.Document,
                    SuggestedItems = new ObservableCollection<string> 
                    { 
                        "Bauplan_Entwurf_v1.pdf", 
                        "Statikberechnung.pdf" 
                    }
                }
            };
            
            CurrentItem = BreadcrumbPath.LastOrDefault();
        }

        [RelayCommand]
        public void NavigateTo(BreadcrumbItem item)
        {
            if (item == null) return;
            
            // Truncate breadcrumb to selected level
            var index = BreadcrumbPath.IndexOf(item);
            if (index >= 0)
            {
                while (BreadcrumbPath.Count > index + 1)
                {
                    BreadcrumbPath.RemoveAt(BreadcrumbPath.Count - 1);
                }
                CurrentItem = item;
            }
        }

        [RelayCommand]
        public void NavigateToSuggestion(string suggestion)
        {
            // In real implementation: navigate to suggested entity
            // For now: just show notification
        }

        public void UpdateBreadcrumb(string authority, string repository, string file, string process, string document)
        {
            BreadcrumbPath.Clear();
            
            if (!string.IsNullOrEmpty(authority))
                BreadcrumbPath.Add(new BreadcrumbItem { Icon = "🏛️", Title = authority, Level = BreadcrumbLevel.Authority });
            
            if (!string.IsNullOrEmpty(repository))
                BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📁", Title = repository, Level = BreadcrumbLevel.Repository });
            
            if (!string.IsNullOrEmpty(file))
                BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📂", Title = file, Level = BreadcrumbLevel.File });
            
            if (!string.IsNullOrEmpty(process))
                BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📋", Title = process, Level = BreadcrumbLevel.Process });
            
            if (!string.IsNullOrEmpty(document))
                BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📄", Title = document, Level = BreadcrumbLevel.Document });
            
            CurrentItem = BreadcrumbPath.LastOrDefault();
        }

        /// <summary>
        /// Setzt den Breadcrumb-Kontext basierend auf dem aktiven Tab
        /// </summary>
        public void SetContextForTab(string tabName)
        {
            BreadcrumbPath.Clear();
            
            switch (tabName)
            {
                case "TabDashboard":
                case "📊 Dashboard":
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "🏠", Title = "Startseite", Level = BreadcrumbLevel.Authority });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📊", Title = "Dashboard", Level = BreadcrumbLevel.Repository });
                    break;

                case "TabAIChat":
                case "🤖 AI Chat":
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "🏠", Title = "Startseite", Level = BreadcrumbLevel.Authority });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "🤖", Title = "AI Assistent", Level = BreadcrumbLevel.Repository });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "💬", Title = "Neue Unterhaltung", Level = BreadcrumbLevel.File });
                    break;

                case "TabTimeline":
                case "📅 Timeline":
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "🏠", Title = "Startseite", Level = BreadcrumbLevel.Authority });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📈", Title = "Projektmanagement", Level = BreadcrumbLevel.Repository });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📅", Title = "Timeline / Gantt", Level = BreadcrumbLevel.File });
                    break;

                case "TabTasks":
                case "✓ Aufgaben":
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "🏠", Title = "Startseite", Level = BreadcrumbLevel.Authority });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📋", Title = "Aufgabenverwaltung", Level = BreadcrumbLevel.Repository });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "✓", Title = "Meine Aufgaben", Level = BreadcrumbLevel.File });
                    break;

                case "TabPreview":
                case "👁 Vorschau":
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "🏠", Title = "Startseite", Level = BreadcrumbLevel.Authority });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📄", Title = "Dokumente", Level = BreadcrumbLevel.Repository });
                    BreadcrumbPath.Add(new BreadcrumbItem { Icon = "👁", Title = "Vorschau", Level = BreadcrumbLevel.File });
                    break;

                default:
                    // Prüfe ob es ein Formular-Tab ist
                    if (tabName.StartsWith("TabForm_"))
                    {
                        BreadcrumbPath.Add(new BreadcrumbItem { Icon = "🏠", Title = "Startseite", Level = BreadcrumbLevel.Authority });
                        BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📋", Title = "Formulare", Level = BreadcrumbLevel.Repository });
                        BreadcrumbPath.Add(new BreadcrumbItem { Icon = "📝", Title = "Formular", Level = BreadcrumbLevel.File });
                    }
                    else
                    {
                        // Fallback auf Default-Breadcrumb
                        InitializeDefaultPath();
                        return;
                    }
                    break;
            }
            
            CurrentItem = BreadcrumbPath.LastOrDefault();
        }

        /// <summary>
        /// Erweitert den aktuellen Breadcrumb-Pfad um ein weiteres Element
        /// </summary>
        public void AppendToBreadcrumb(string icon, string title, BreadcrumbLevel level)
        {
            BreadcrumbPath.Add(new BreadcrumbItem { Icon = icon, Title = title, Level = level });
            CurrentItem = BreadcrumbPath.LastOrDefault();
        }
    }

    public class BreadcrumbItem
    {
        public string Icon { get; set; } = string.Empty;
        public string Title { get; set; } = string.Empty;
        public BreadcrumbLevel Level { get; set; }
        public ObservableCollection<string> SuggestedItems { get; set; } = new();
    }

    public enum BreadcrumbLevel
    {
        Authority,   // Behörde
        Repository,  // Ablage
        File,        // Akte
        Process,     // Vorgang
        Document     // Dokument
    }
}
