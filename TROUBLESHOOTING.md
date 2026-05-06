# 🔍 Troubleshooting Guide (C++ Edition)

## ✅ Checking Source Files

For the C++ version, ensure you have these core files:

```
OSMV-lite/
├── main.cpp             ← Entry point & Win32 UI
├── MediaTracker.hpp     ← Media logic (C++/WinRT)
├── Base64.hpp           ← Image utility
├── build.bat            ← Compilation script
├── index.html           ← OBS Frontend
└── style.css            ← OBS Styling
```

## 🐛 Common Issues & Solutions

### 1. "build.bat" fails to find Visual Studio
**Cause**: The script uses `vswhere.exe` to find MSVC. If you have a custom install path or an old version, it might fail.
**Solution**: Run `build.bat` directly from the **"Developer Command Prompt for VS 2022"** (search for it in your Start menu).

### 2. Application launches and closes immediately
**Cause**: Potential conflict with the Windows Media Session or missing system permissions.
**Solution**: 
- Check if another instance is already running in the system tray.
- Ensure your music player (Spotify/Apple Music) is open and has played at least one song.

### 3. OBS widget is blank or shows "Aucune lecture"
**Cause**: The `current_song.json` file is missing or not updated.
**Solution**:
- Ensure `OSMV-Lite-cpp.exe` is running (check system tray).
- Verify that `index.html` and `OSMV-Lite-cpp.exe` are in the **same folder**.
- Check if `current_song.json` is being created/updated in that folder.

### 4. Special characters (accents) are not showing correctly
**Solution**: This was fixed in the latest version by forcing **UTF-8** encoding. Ensure you have recompiled the app using the latest source code.

## 💡 Support

If issues persist, open an **Issue** on GitHub with:
- Your Windows version.
- The music player you are using.
- A screenshot of any error message shown by the application.
