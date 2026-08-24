#include <fstream>
#include <sstream>
#include <string>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <yaml-cpp/yaml.h>

namespace {
constexpr int kGeneralTabIndex = 0;
constexpr int kStorageTabIndex = 1;
constexpr int kNetworkTabIndex = 2;
constexpr int kRawTabIndex = 3;
}

class ConfigFrame final : public wxFrame {
public:
    ConfigFrame()
        : wxFrame(nullptr, wxID_ANY, "ThemisDB Config Editor (wxWidgets)",
                  wxDefaultPosition, wxSize(1050, 720)) {
        BuildMenu();
        BuildNotebook();
        CreateStatusBar();
        SetStatusText("Ready");
        NewConfig();
    }

private:
    void BuildMenu() {
        auto* fileMenu = new wxMenu();
        fileMenu->Append(wxID_NEW, "&New\tCtrl+N");
        fileMenu->Append(wxID_OPEN, "&Open...\tCtrl+O");
        fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S");
        fileMenu->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S");
        fileMenu->AppendSeparator();
        fileMenu->Append(wxID_EXIT, "E&xit");

        auto* helpMenu = new wxMenu();
        helpMenu->Append(wxID_ABOUT, "&About");

        auto* menuBar = new wxMenuBar();
        menuBar->Append(fileMenu, "&File");
        menuBar->Append(helpMenu, "&Help");
        SetMenuBar(menuBar);

        Bind(wxEVT_MENU, &ConfigFrame::OnNew, this, wxID_NEW);
        Bind(wxEVT_MENU, &ConfigFrame::OnOpen, this, wxID_OPEN);
        Bind(wxEVT_MENU, &ConfigFrame::OnSave, this, wxID_SAVE);
        Bind(wxEVT_MENU, &ConfigFrame::OnSaveAs, this, wxID_SAVEAS);
        Bind(wxEVT_MENU, &ConfigFrame::OnExit, this, wxID_EXIT);
        Bind(wxEVT_MENU, &ConfigFrame::OnAbout, this, wxID_ABOUT);
    }

    void BuildNotebook() {
        auto* root = new wxBoxSizer(wxVERTICAL);
        notebook_ = new wxNotebook(this, wxID_ANY);

        auto* generalPanel = new wxPanel(notebook_);
        auto* generalSizer = new wxFlexGridSizer(2, 10, 10);
        generalSizer->AddGrowableCol(1, 1);

        generalSizer->Add(new wxStaticText(generalPanel, wxID_ANY, "Server Host"),
                          0, wxALIGN_CENTER_VERTICAL);
        serverHostCtrl_ = new wxTextCtrl(generalPanel, wxID_ANY);
        generalSizer->Add(serverHostCtrl_, 1, wxEXPAND);

        generalSizer->Add(new wxStaticText(generalPanel, wxID_ANY, "Server Port"),
                          0, wxALIGN_CENTER_VERTICAL);
        serverPortCtrl_ = new wxSpinCtrl(generalPanel, wxID_ANY);
        serverPortCtrl_->SetRange(1, 65535);
        generalSizer->Add(serverPortCtrl_, 1, wxEXPAND);

        auto* generalWrap = new wxBoxSizer(wxVERTICAL);
        generalWrap->Add(generalSizer, 1, wxALL | wxEXPAND, 12);
        generalPanel->SetSizer(generalWrap);

        auto* storagePanel = new wxPanel(notebook_);
        auto* storageSizer = new wxFlexGridSizer(2, 10, 10);
        storageSizer->AddGrowableCol(1, 1);

        storageSizer->Add(new wxStaticText(storagePanel, wxID_ANY, "Data Directory"),
                          0, wxALIGN_CENTER_VERTICAL);
        dataDirCtrl_ = new wxTextCtrl(storagePanel, wxID_ANY);
        storageSizer->Add(dataDirCtrl_, 1, wxEXPAND);

        auto* storageWrap = new wxBoxSizer(wxVERTICAL);
        storageWrap->Add(storageSizer, 1, wxALL | wxEXPAND, 12);
        storagePanel->SetSizer(storageWrap);

        auto* networkPanel = new wxPanel(notebook_);
        auto* networkSizer = new wxFlexGridSizer(2, 10, 10);
        networkSizer->AddGrowableCol(1, 1);

        networkSizer->Add(new wxStaticText(networkPanel, wxID_ANY, "Bind Address"),
                          0, wxALIGN_CENTER_VERTICAL);
        bindAddressCtrl_ = new wxTextCtrl(networkPanel, wxID_ANY);
        networkSizer->Add(bindAddressCtrl_, 1, wxEXPAND);

        networkSizer->Add(new wxStaticText(networkPanel, wxID_ANY, "TLS Enabled"),
                          0, wxALIGN_CENTER_VERTICAL);
        tlsEnabledCtrl_ = new wxCheckBox(networkPanel, wxID_ANY, "Enable TLS");
        networkSizer->Add(tlsEnabledCtrl_, 0, wxALIGN_CENTER_VERTICAL);

        auto* networkWrap = new wxBoxSizer(wxVERTICAL);
        networkWrap->Add(networkSizer, 1, wxALL | wxEXPAND, 12);
        networkPanel->SetSizer(networkWrap);

        auto* rawPanel = new wxPanel(notebook_);
        auto* rawSizer = new wxBoxSizer(wxVERTICAL);
        rawYamlCtrl_ = new wxTextCtrl(rawPanel, wxID_ANY, "",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxTE_MULTILINE | wxTE_RICH2);
        rawSizer->Add(rawYamlCtrl_, 1, wxALL | wxEXPAND, 10);
        rawPanel->SetSizer(rawSizer);

        notebook_->AddPage(generalPanel, "General", true);
        notebook_->AddPage(storagePanel, "Storage", false);
        notebook_->AddPage(networkPanel, "Network", false);
        notebook_->AddPage(rawPanel, "Raw YAML", false);

        root->Add(notebook_, 1, wxEXPAND);
        SetSizer(root);

        serverHostCtrl_->Bind(wxEVT_TEXT, &ConfigFrame::OnFieldChanged, this);
        serverPortCtrl_->Bind(wxEVT_SPINCTRL, &ConfigFrame::OnFieldChanged, this);
        dataDirCtrl_->Bind(wxEVT_TEXT, &ConfigFrame::OnFieldChanged, this);
        bindAddressCtrl_->Bind(wxEVT_TEXT, &ConfigFrame::OnFieldChanged, this);
        tlsEnabledCtrl_->Bind(wxEVT_CHECKBOX, &ConfigFrame::OnFieldChanged, this);
        rawYamlCtrl_->Bind(wxEVT_TEXT, &ConfigFrame::OnFieldChanged, this);
        notebook_->Bind(wxEVT_NOTEBOOK_PAGE_CHANGING,
                        &ConfigFrame::OnNotebookPageChanging, this);
    }

    void OnFieldChanged(wxCommandEvent&) {
        isDirty_ = true;
        UpdateTitle();
    }

    void OnNotebookPageChanging(wxBookCtrlEvent& event) {
        const int oldIndex = event.GetOldSelection();
        const int newIndex = event.GetSelection();

        if (newIndex == kRawTabIndex) {
            CollectConfigFromForm();
            UpdateRawTabFromConfig();
            return;
        }

        if (oldIndex == kRawTabIndex) {
            if (!ParseRawTabIntoConfig()) {
                event.Veto();
                return;
            }
            PopulateFormFromConfig();
        }
    }

    void OnNew(wxCommandEvent&) {
        if (!ConfirmDiscardChanges()) {
            return;
        }
        NewConfig();
    }

    void OnOpen(wxCommandEvent&) {
        if (!ConfirmDiscardChanges()) {
            return;
        }

        wxFileDialog dialog(this, "Open ThemisDB config", wxEmptyString,
                            wxEmptyString,
                            "Config files (*.yaml;*.yml;*.json)|*.yaml;*.yml;*.json|All files (*.*)|*.*",
                            wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dialog.ShowModal() != wxID_OK) {
            return;
        }

        const wxString path = dialog.GetPath();
        if (LoadConfigFile(path.ToStdString())) {
            currentPath_ = path.ToStdString();
            isDirty_ = false;
            PopulateFormFromConfig();
            UpdateRawTabFromConfig();
            SetStatusText("Loaded: " + path);
            UpdateTitle();
        }
    }

    void OnSave(wxCommandEvent&) {
        if (currentPath_.empty()) {
            SaveAs();
            return;
        }
        SaveToPath(currentPath_);
    }

    void OnSaveAs(wxCommandEvent&) {
        SaveAs();
    }

    void OnExit(wxCommandEvent&) {
        Close(true);
    }

    void OnAbout(wxCommandEvent&) {
        wxMessageBox("ThemisDB Config Editor\nCross-platform wxWidgets prototype\nTabbed layout with raw YAML fallback.",
                     "About", wxOK | wxICON_INFORMATION, this);
    }

    bool ConfirmDiscardChanges() {
        if (!isDirty_) {
            return true;
        }

        const int answer = wxMessageBox(
            "You have unsaved changes. Continue and discard them?",
            "Unsaved changes",
            wxYES_NO | wxNO_DEFAULT | wxICON_WARNING,
            this);
        return answer == wxYES;
    }

    void SaveAs() {
        wxFileDialog dialog(this, "Save ThemisDB config", wxEmptyString,
                            "config.yaml",
                            "YAML files (*.yaml)|*.yaml|JSON files (*.json)|*.json|All files (*.*)|*.*",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dialog.ShowModal() != wxID_OK) {
            return;
        }

        currentPath_ = dialog.GetPath().ToStdString();
        SaveToPath(currentPath_);
    }

    void SaveToPath(const std::string& path) {
        if (notebook_->GetSelection() == kRawTabIndex) {
            if (!ParseRawTabIntoConfig()) {
                return;
            }
        } else {
            CollectConfigFromForm();
        }

        YAML::Emitter out;
        out << config_;
        if (!out.good()) {
            wxMessageBox("Could not serialize YAML config.",
                         "Save error", wxOK | wxICON_ERROR, this);
            return;
        }

        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) {
            wxMessageBox("Could not open file for writing:\n" + path,
                         "Save error", wxOK | wxICON_ERROR, this);
            return;
        }

        ofs << out.c_str();
        if (!ofs.good()) {
            wxMessageBox("Could not write config file:\n" + path,
                         "Save error", wxOK | wxICON_ERROR, this);
            return;
        }

        isDirty_ = false;
        UpdateTitle();
        SetStatusText("Saved: " + path);
    }

    bool LoadConfigFile(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            wxMessageBox("Could not open file:\n" + path,
                         "Open error", wxOK | wxICON_ERROR, this);
            return false;
        }

        std::ostringstream buffer;
        buffer << ifs.rdbuf();

        try {
            config_ = YAML::Load(buffer.str());
        } catch (const std::exception& ex) {
            wxMessageBox("Failed to parse config:\n" + wxString::FromUTF8(ex.what()),
                         "Parse error", wxOK | wxICON_ERROR, this);
            return false;
        }

        if (!config_ || !config_.IsMap()) {
            config_ = YAML::Node(YAML::NodeType::Map);
        }
        return true;
    }

    void NewConfig() {
        config_ = YAML::Node(YAML::NodeType::Map);
        config_["server"]["host"] = "127.0.0.1";
        config_["server"]["port"] = 9000;
        config_["storage"]["data_dir"] = "./data";
        config_["network"]["bind_address"] = "0.0.0.0";
        config_["network"]["tls_enabled"] = false;

        currentPath_.clear();
        isDirty_ = false;
        PopulateFormFromConfig();
        UpdateRawTabFromConfig();
        SetStatusText("New config");
        UpdateTitle();
    }

    void UpdateRawTabFromConfig() {
        YAML::Emitter out;
        out << config_;
        rawYamlCtrl_->ChangeValue(out.good() ? out.c_str() : "{}");
    }

    bool ParseRawTabIntoConfig() {
        const std::string raw = rawYamlCtrl_->GetValue().ToStdString();
        try {
            YAML::Node parsed = YAML::Load(raw);
            if (!parsed || !parsed.IsMap()) {
                wxMessageBox("Root element must be an object/map.",
                             "Validation error", wxOK | wxICON_ERROR, this);
                return false;
            }
            config_ = parsed;
            return true;
        } catch (const std::exception& ex) {
            wxMessageBox("Failed to parse YAML:\n" + wxString::FromUTF8(ex.what()),
                         "Parse error", wxOK | wxICON_ERROR, this);
            return false;
        }
    }

    void PopulateFormFromConfig() {
        serverHostCtrl_->ChangeValue(ReadString(config_, "server", "host", "127.0.0.1"));
        serverPortCtrl_->SetValue(ReadInt(config_, "server", "port", 9000));
        dataDirCtrl_->ChangeValue(ReadString(config_, "storage", "data_dir", "./data"));
        bindAddressCtrl_->ChangeValue(ReadString(config_, "network", "bind_address", "0.0.0.0"));
        tlsEnabledCtrl_->SetValue(ReadBool(config_, "network", "tls_enabled", false));
    }

    void CollectConfigFromForm() {
        config_["server"]["host"] = serverHostCtrl_->GetValue().ToStdString();
        config_["server"]["port"] = serverPortCtrl_->GetValue();
        config_["storage"]["data_dir"] = dataDirCtrl_->GetValue().ToStdString();
        config_["network"]["bind_address"] = bindAddressCtrl_->GetValue().ToStdString();
        config_["network"]["tls_enabled"] = tlsEnabledCtrl_->GetValue();
    }

    static std::string ReadString(const YAML::Node& root,
                                  const char* section,
                                  const char* key,
                                  const char* fallback) {
        try {
            const YAML::Node sectionNode = root[section];
            if (!sectionNode || !sectionNode.IsMap()) {
                return fallback;
            }
            const YAML::Node keyNode = sectionNode[key];
            if (!keyNode) {
                return fallback;
            }
            return keyNode.as<std::string>();
        } catch (...) {
            return fallback;
        }
    }

    static int ReadInt(const YAML::Node& root,
                       const char* section,
                       const char* key,
                       int fallback) {
        try {
            const YAML::Node sectionNode = root[section];
            if (!sectionNode || !sectionNode.IsMap()) {
                return fallback;
            }
            const YAML::Node keyNode = sectionNode[key];
            if (!keyNode) {
                return fallback;
            }
            return keyNode.as<int>();
        } catch (...) {
            return fallback;
        }
    }

    static bool ReadBool(const YAML::Node& root,
                         const char* section,
                         const char* key,
                         bool fallback) {
        try {
            const YAML::Node sectionNode = root[section];
            if (!sectionNode || !sectionNode.IsMap()) {
                return fallback;
            }
            const YAML::Node keyNode = sectionNode[key];
            if (!keyNode) {
                return fallback;
            }
            return keyNode.as<bool>();
        } catch (...) {
            return fallback;
        }
    }

    void UpdateTitle() {
        wxString title = "ThemisDB Config Editor (wxWidgets)";
        if (!currentPath_.empty()) {
            title += " - " + wxString::FromUTF8(currentPath_);
        }
        if (isDirty_) {
            title += " *";
        }
        SetTitle(title);
    }

    wxNotebook* notebook_{nullptr};
    wxTextCtrl* serverHostCtrl_{nullptr};
    wxSpinCtrl* serverPortCtrl_{nullptr};
    wxTextCtrl* dataDirCtrl_{nullptr};
    wxTextCtrl* bindAddressCtrl_{nullptr};
    wxCheckBox* tlsEnabledCtrl_{nullptr};
    wxTextCtrl* rawYamlCtrl_{nullptr};

    YAML::Node config_;
    std::string currentPath_;
    bool isDirty_{false};
};

class ConfigEditorApp final : public wxApp {
public:
    bool OnInit() override {
        auto* frame = new ConfigFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ConfigEditorApp);
