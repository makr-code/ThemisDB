/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            LlmQueryAssistant.cs                               ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:24:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     223                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.ObjectModel;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows.Controls;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace Themis.ImpactAnalysisViewer.Controls
{
    /// <summary>
    /// LLM-powered query assistant for natural language to AQL conversion
    /// </summary>
    public partial class LlmQueryAssistant : UserControl
    {
        private readonly HttpClient _httpClient;
        private string _apiKey;
        private string _apiEndpoint = "https://api.openai.com/v1/chat/completions";

        public ObservableCollection<ChatMessage> Messages { get; set; }
        public string UserInput { get; set; }
        public string GeneratedQuery { get; set; }
        public bool IsProcessing { get; set; }

        public LlmQueryAssistant()
        {
            _httpClient = new HttpClient();
            Messages = new ObservableCollection<ChatMessage>();
            
            // Add welcome message
            Messages.Add(new ChatMessage
            {
                Role = "assistant",
                Content = "Hallo! Ich kann Ihnen helfen, AQL-Abfragen für Impact-Analysen zu erstellen. " +
                          "Beschreiben Sie einfach, was Sie analysieren möchten.\n\n" +
                          "Beispiele:\n" +
                          "- 'Zeige alle APIs die von Datenbank-Änderungen betroffen sind'\n" +
                          "- 'Finde kritische Prozesse mit hohem Impact-Score'\n" +
                          "- 'Analysiere Cross-Layer-Abhängigkeiten von API zu UI'"
            });

            InitializeComponent();
        }

        private void InitializeComponent()
        {
            // WPF initialization - implemented in XAML
        }

        public void SetApiKey(string apiKey)
        {
            _apiKey = apiKey;
            _httpClient.DefaultRequestHeaders.Clear();
            _httpClient.DefaultRequestHeaders.Add("Authorization", $"Bearer {apiKey}");
        }

        [RelayCommand]
        private async Task GenerateQueryAsync()
        {
            if (string.IsNullOrWhiteSpace(UserInput))
                return;

            IsProcessing = true;

            // Add user message
            Messages.Add(new ChatMessage { Role = "user", Content = UserInput });

            try
            {
                var systemPrompt = @"You are an expert in ArangoDB Query Language (AQL) for impact analysis systems.
The database has the following structure:
- Collection: impact_nodes (fields: _id, _layer, impact_score, _layer_metadata)
- Collection: impact_edges (graph connections between nodes)
- Graph: impact_graph

Layers: document, process, api, database, ui, infrastructure, custom

Generate valid AQL queries based on user requests. Return only the query, no explanations.";

                var request = new
                {
                    model = "gpt-4",
                    messages = new[]
                    {
                        new { role = "system", content = systemPrompt },
                        new { role = "user", content = UserInput }
                    },
                    temperature = 0.3,
                    max_tokens = 500
                };

                var json = JsonSerializer.Serialize(request);
                var content = new StringContent(json, Encoding.UTF8, "application/json");

                var response = await _httpClient.PostAsync(_apiEndpoint, content);
                var responseJson = await response.Content.ReadAsStringAsync();

                var result = JsonSerializer.Deserialize<JsonElement>(responseJson);
                var generatedQuery = result.GetProperty("choices")[0]
                    .GetProperty("message")
                    .GetProperty("content")
                    .GetString();

                GeneratedQuery = generatedQuery;

                // Add assistant response
                Messages.Add(new ChatMessage
                {
                    Role = "assistant",
                    Content = $"Hier ist Ihre AQL-Abfrage:\n\n```aql\n{generatedQuery}\n```"
                });
            }
            catch (Exception ex)
            {
                Messages.Add(new ChatMessage
                {
                    Role = "assistant",
                    Content = $"Fehler bei der Query-Generierung: {ex.Message}"
                });
            }
            finally
            {
                IsProcessing = false;
                UserInput = string.Empty;
            }
        }

        [RelayCommand]
        private async Task RefineQueryAsync()
        {
            if (string.IsNullOrWhiteSpace(GeneratedQuery))
                return;

            UserInput = "Optimiere diese Abfrage für bessere Performance und füge Kommentare hinzu.";
            await GenerateQueryAsync();
        }

        [RelayCommand]
        private async Task ExplainQueryAsync()
        {
            if (string.IsNullOrWhiteSpace(GeneratedQuery))
                return;

            IsProcessing = true;

            try
            {
                var request = new
                {
                    model = "gpt-4",
                    messages = new[]
                    {
                        new { role = "system", content = "Explain AQL queries in simple German language." },
                        new { role = "user", content = $"Erkläre diese AQL-Abfrage:\n\n{GeneratedQuery}" }
                    },
                    temperature = 0.5,
                    max_tokens = 300
                };

                var json = JsonSerializer.Serialize(request);
                var content = new StringContent(json, Encoding.UTF8, "application/json");

                var response = await _httpClient.PostAsync(_apiEndpoint, content);
                var responseJson = await response.Content.ReadAsStringAsync();

                var result = JsonSerializer.Deserialize<JsonElement>(responseJson);
                var explanation = result.GetProperty("choices")[0]
                    .GetProperty("message")
                    .GetProperty("content")
                    .GetString();

                Messages.Add(new ChatMessage
                {
                    Role = "assistant",
                    Content = explanation
                });
            }
            catch (Exception ex)
            {
                Messages.Add(new ChatMessage
                {
                    Role = "assistant",
                    Content = $"Fehler: {ex.Message}"
                });
            }
            finally
            {
                IsProcessing = false;
            }
        }
    }

    public class ChatMessage
    {
        public string Role { get; set; } // "user" or "assistant"
        public string Content { get; set; }
        public DateTime Timestamp { get; set; } = DateTime.Now;
    }
}
