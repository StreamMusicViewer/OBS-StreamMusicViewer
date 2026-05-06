# 🎵 Now Playing Widget for OBS (C++ Edition)

![Status](https://img.shields.io/badge/status-working-success)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B20-orange)
![Performance](https://img.shields.io/badge/RAM-%3C5MB-green)

A high-performance, real-time "Now Playing" widget for OBS. This version is written in native C++ for **minimum resource consumption** (RAM and CPU). It tracks music from Apple Music, Spotify, and other media players using the native Windows Media API.

## ✨ Features

- ⚡ **Ultra-Lightweight** - Uses < 10MB of RAM (vs 150MB+ for .NET versions).
- 🔄 **Event-Driven** - Zero CPU usage when idle; updates only when the track changes.
- 🖼️ **Real-time Album Art** - Displays high-quality artwork via GDI+.
- 🎨 **Modern OBS Frontend** - Sleek, transparent glassmorphism design for your stream.
- 🎯 **Native Windows Integration** - Uses C++/WinRT for reliable media detection.
- 📦 **Zero Dependencies** - Single native executable, no runtime required.

## 🚀 Quick Start

1. Download the latest release (or compile it yourself).
2. Place `OSMV-Lite-cpp.exe`, `index.html`, and `style.css` in the same folder.
3. Launch `OSMV-Lite-cpp.exe`. It will stay in your system tray.
4. Add `index.html` as a **Browser Source** in OBS.

## 🔧 Compiling from Source

### Requirements
- **Windows 10/11**
- **Visual Studio 2022** (with "Desktop development with C++" workload)

### Compilation Steps
1. Clone the repository.
2. Open a **Developer Command Prompt for VS 2022**.
3. Run `build.bat`.
4. The standalone `OSMV-Lite-cpp.exe` will be generated.

## 📺 Configure OBS

1. Add a new **Browser** source.
2. Check **Local file** and select `index.html`.
3. Dimensions: **Width: 500**, **Height: 140**.

## 🏗️ How It Works

```
Music Player (Spotify/Apple Music)
    ↓
Windows Media Control API (C++/WinRT Events)
    ↓
OSMV-Lite-cpp.exe (Native C++)
    ↓
current_song.json (Atomic UTF-8 Write)
    ↓
index.html (OBS Frontend)
```

## 📄 License
MIT License
