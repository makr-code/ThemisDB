/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UserSwitcherViewModel.cs                           ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Themis.DocumentManager.Models;
using Themis.DocumentManager.Services;

namespace Themis.DocumentManager.ViewModels
{
    public partial class UserSwitcherViewModel : ObservableObject
    {
        private readonly IAuthenticationService _authService;

        [ObservableProperty]
        private ObservableCollection<UserInfo> availableUsers = new();

        [ObservableProperty]
        private UserInfo? currentUser;

        [ObservableProperty]
        private bool isLoading;

        public UserSwitcherViewModel(IAuthenticationService authService)
        {
            _authService = authService;
            InitializeUsers();
            LoadCurrentUser();
        }

        private void InitializeUsers()
        {
            // Hardcoded test users from seeded data
            AvailableUsers = new ObservableCollection<UserInfo>
            {
                new UserInfo { Username = "max.mustermann", DisplayName = "Max Mustermann", Role = "Admin", Department = "Legal" },
                new UserInfo { Username = "anna.schmidt", DisplayName = "Anna Schmidt", Role = "Editor", Department = "Legal" },
                new UserInfo { Username = "thomas.mueller", DisplayName = "Thomas Mueller", Role = "Editor", Department = "Operations" },
                new UserInfo { Username = "lisa.weber", DisplayName = "Lisa Weber", Role = "Viewer", Department = "Compliance" },
                new UserInfo { Username = "michael.braun", DisplayName = "Michael Braun", Role = "Viewer", Department = "Compliance" }
            };
        }

        private void LoadCurrentUser()
        {
            var currentUsername = _authService.CurrentUserName ?? "System";
            CurrentUser = AvailableUsers.FirstOrDefault(u => u.Username == currentUsername)
                       ?? new UserInfo { Username = currentUsername, DisplayName = currentUsername, Role = "System" };
        }

        [RelayCommand]
        public async Task SwitchUserAsync(UserInfo user)
        {
            if (user == null) return;

            try
            {
                IsLoading = true;

                // Switch user: logout current + login with new user
                await _authService.LogoutAsync();
                await _authService.LoginAsync(user.Username, "password");

                CurrentUser = user;
            }
            finally
            {
                IsLoading = false;
            }
        }

        [RelayCommand]
        public async Task RefreshUsersAsync()
        {
            IsLoading = true;
            try
            {
                // Could load from ThemisDB in future
                InitializeUsers();
                LoadCurrentUser();
            }
            finally
            {
                IsLoading = false;
            }
        }
    }

    public class UserInfo
    {
        public string Username { get; set; } = string.Empty;
        public string DisplayName { get; set; } = string.Empty;
        public string Role { get; set; } = string.Empty;
        public string Department { get; set; } = string.Empty;
    }
}
