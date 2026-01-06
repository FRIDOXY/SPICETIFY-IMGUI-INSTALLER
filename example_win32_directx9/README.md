# 🎵 Spicetify Manager

<div align="center">
  
![Spicetify Manager](https://img.shields.io/badge/version-1.0.0-purple)
![Platform](https://img.shields.io/badge/platform-Windows-blue)
![DirectX](https://img.shields.io/badge/DirectX-9-green)
![License](https://img.shields.io/badge/license-MIT-orange)

A modern GUI application to manage Spotify and Spicetify installations on Windows.

[Features](#features) • [Installation](#installation) • [Usage](#usage) • [Building](#building) • [Contributing](#contributing)

![Spicetify Manager Screenshot](screenshot.png)

</div>

---

## 🌟 Features

### 📊 Dashboard
- **Real-time Status Monitoring**: Check if Spotify MS Store, Spotify Web, and Spicetify are installed
- **Quick Actions**: One-click installation buttons for common tasks
- **Visual Status Indicators**: Color-coded status display (Green = Installed, Red = Not installed)

### 🎧 Spotify Management
- **Install Spotify Web**: Download and install the web version of Spotify
- **Migrate from MS Store**: Seamlessly migrate from Microsoft Store version to Web version
- **Uninstall Options**: Remove Spotify MS Store or Web versions
- **Feature Information**: Learn about Spotify Web features and benefits

### 🎨 Spicetify Management
- **Install Spicetify**: One-click installation of Spicetify CLI
- **Update Spicetify**: Keep your Spicetify installation up to date
- **Apply Changes**: Apply your customizations to Spotify
- **Restore Spotify**: Revert Spotify to its original state
- **Quick Access**: Open Spicetify config folder directly
- **Resources**: Direct links to GitHub repo and Marketplace

### ⚙️ Settings
- **Always on Top**: Keep the application window above other windows
- **Notifications**: Enable/disable system notifications
- **Auto-refresh**: Automatically check installation status on startup

### ❓ Help & Support
- **Discord Integration**: Join the community Discord server
- **Documentation**: Access official Spicetify documentation
- **Quick Guide**: Step-by-step instructions for beginners

---

## 🚀 Installation

### Option 1: Download Pre-built Binary (Recommended)

1. Go to the [Releases](https://github.com/yourusername/spicetify-manager/releases) page
2. Download the latest `SpicetifyManager.exe`
3. Run the executable - no installation required!

### Option 2: Build from Source

See [Building from Source](#building-from-source) section below.

---

## 💻 Usage

1. **Launch the Application**
   - Double-click `SpicetifyManager.exe`
   - No console window will appear

2. **Check Status**
   - The Dashboard tab shows current installation status
   - Green = Installed ✅
   - Red = Not Installed ❌

3. **Install Spotify Web** (Required for Spicetify)
   - Navigate to **Spotify** tab
   - Click **Install Web Version**
   - Wait for PowerShell to complete the installation

4. **Install Spicetify**
   - Navigate to **Spicetify** tab
   - Click **Install Spicetify**
   - Follow the PowerShell instructions

5. **Apply Customizations**
   - After installing themes or extensions
   - Click **Apply Changes** to activate them

---

## 🛠️ Building from Source

### Prerequisites

- **Visual Studio 2019** or newer
- **Windows SDK 10.0** or newer
- **DirectX 9 SDK** (June 2010)
- **C++17** compiler support

### Dependencies

The project includes:
- **Dear ImGui** (v1.89+)
- **DirectX 9** backend
- **Win32** platform backend
- Custom GUI framework
- Font Awesome icons

### Build Steps

1. **Clone the Repository**