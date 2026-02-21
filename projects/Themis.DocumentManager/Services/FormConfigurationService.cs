/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            FormConfigurationService.cs                        ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Themis.DocumentManager.Services;

public interface IFormConfigurationLoader
{
    Task<FormTemplate?> LoadFromJsonAsync(string filePath);
    Task<FormTemplate?> LoadFromYamlAsync(string filePath);
    Task<List<FormTemplate>> LoadAllTemplatesFromDirectoryAsync(string directoryPath);
    Task SaveTemplateAsJsonAsync(FormTemplate template, string filePath);
    Task SaveTemplateAsYamlAsync(FormTemplate template, string filePath);
}

public class FormConfigurationLoader : IFormConfigurationLoader
{
    public Task<FormTemplate?> LoadFromJsonAsync(string filePath)
    {
        try
        {
            var json = System.IO.File.ReadAllText(filePath);
            var template = System.Text.Json.JsonSerializer.Deserialize<FormTemplate>(json);
            return Task.FromResult(template);
        }
        catch
        {
            return Task.FromResult<FormTemplate?>(null);
        }
    }

    public async Task<FormTemplate?> LoadFromYamlAsync(string filePath)
    {
        try
        {
            var yaml = System.IO.File.ReadAllText(filePath);
            // Simplified YAML parsing - in production, use YamlDotNet
            var template = ParseSimpleYaml(yaml);
            return await Task.FromResult(template);
        }
        catch
        {
            return null;
        }
    }

    public async Task<List<FormTemplate>> LoadAllTemplatesFromDirectoryAsync(string directoryPath)
    {
        var templates = new List<FormTemplate>();
        if (!System.IO.Directory.Exists(directoryPath))
            return templates;

        foreach (var jsonFile in System.IO.Directory.GetFiles(directoryPath, "*.json"))
        {
            var template = await LoadFromJsonAsync(jsonFile);
            if (template != null)
                templates.Add(template);
        }

        foreach (var yamlFile in System.IO.Directory.GetFiles(directoryPath, "*.yaml"))
        {
            var template = await LoadFromYamlAsync(yamlFile);
            if (template != null)
                templates.Add(template);
        }

        return templates;
    }

    public Task SaveTemplateAsJsonAsync(FormTemplate template, string filePath)
    {
        var json = System.Text.Json.JsonSerializer.Serialize(template, new System.Text.Json.JsonSerializerOptions { WriteIndented = true });
        System.IO.File.WriteAllText(filePath, json);
        return Task.CompletedTask;
    }

    public Task SaveTemplateAsYamlAsync(FormTemplate template, string filePath)
    {
        var yaml = ConvertToSimpleYaml(template);
        System.IO.File.WriteAllText(filePath, yaml);
        return Task.CompletedTask;
    }

    private FormTemplate? ParseSimpleYaml(string yaml)
    {
        var template = new FormTemplate();
        var lines = yaml.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None);

        foreach (var line in lines)
        {
            if (line.Contains("id:"))
                template.Id = line.Split(":")[1].Trim().Trim('"', '\'');
            if (line.Contains("name:"))
                template.Name = line.Split(":")[1].Trim().Trim('"', '\'');
            if (line.Contains("category:"))
                template.Category = line.Split(":")[1].Trim().Trim('"', '\'');
        }

        return template;
    }

    private string ConvertToSimpleYaml(FormTemplate template)
    {
        var yaml = new System.Text.StringBuilder();
        yaml.AppendLine("form:");
        yaml.AppendLine($"  id: {template.Id}");
        yaml.AppendLine($"  name: \"{template.Name}\"");
        yaml.AppendLine($"  category: {template.Category}");
        yaml.AppendLine($"  version: {template.Version}");
        yaml.AppendLine("  sections:");

        foreach (var section in template.Sections)
        {
            yaml.AppendLine($"    - id: {section.Id}");
            yaml.AppendLine($"      title: \"{section.Title}\"");
            yaml.AppendLine($"      order: {section.Order}");
            yaml.AppendLine("      fields:");

            foreach (var field in section.Fields)
            {
                yaml.AppendLine($"        - id: {field.Id}");
                yaml.AppendLine($"          name: {field.Name}");
                yaml.AppendLine($"          label: \"{field.Label}\"");
                yaml.AppendLine($"          type: {field.Type}");
                yaml.AppendLine($"          required: {field.IsRequired.ToString().ToLower()}");

                if (field.MaxLength.HasValue)
                    yaml.AppendLine($"          maxLength: {field.MaxLength}");
                if (field.MinLength.HasValue)
                    yaml.AppendLine($"          minLength: {field.MinLength}");
            }
        }

        return yaml.ToString();
    }
}

public interface IFormTestDataService
{
    Task<FormSubmissionData> GetSamplePDVSubmissionAsync();
    Task<List<FormSubmissionData>> GetAllSampleSubmissionsAsync();
    Task<FormSubmissionData> GenerateRandomSubmissionAsync(FormTemplate template);
}

public class FormTestDataService : IFormTestDataService
{
    public Task<FormSubmissionData> GetSamplePDVSubmissionAsync()
    {
        return Task.FromResult(new FormSubmissionData
        {
            FormId = "pdv-vis5-document",
            SubmittedBy = "TestUser",
            Status = "Valid",
            FieldValues = new Dictionary<string, object>
            {
                { "doc-number", "MU123456" },
                { "doc-title", "Test Dokumentenverwaltung" },
                { "doc-type", "1" },
                { "author", "Max Mustermann" },
                { "created-date", DateTime.Now.ToString("yyyy-MM-dd") },
                { "retention-period", 10 },
                { "classification", "internal" },
                { "priority", "normal" }
            }
        });
    }

    public async Task<List<FormSubmissionData>> GetAllSampleSubmissionsAsync()
    {
        var submissions = new List<FormSubmissionData>();
        submissions.Add(await GetSamplePDVSubmissionAsync());
        return submissions;
    }

    public Task<FormSubmissionData> GenerateRandomSubmissionAsync(FormTemplate template)
    {
        var submission = new FormSubmissionData { FormId = template.Id };
        foreach (var section in template.Sections)
        {
            foreach (var field in section.Fields)
            {
                submission.FieldValues[field.Id] = field.Type switch
                {
                    FormFieldType.Number => new Random().Next(0, 100),
                    FormFieldType.Date => DateTime.Now.AddDays(new Random().Next(-30, 30)),
                    FormFieldType.Checkbox => new Random().Next(2) == 0,
                    _ => $"Sample_{Guid.NewGuid().ToString().Substring(0, 8)}"
                };
            }
        }
        return Task.FromResult(submission);
    }
}
