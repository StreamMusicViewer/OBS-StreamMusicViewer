#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <memory>
#include <gdiplus.h>
#include <shlwapi.h>
#include <vector>
#include "MediaTracker.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "windowsapp.lib")

using namespace Gdiplus;

// Constants
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SHOW 1002

// Global variables
HWND g_hWnd = NULL;
NOTIFYICONDATA g_nid = { 0 };
std::unique_ptr<MediaTracker> g_tracker;
MediaTracker::MediaInfo g_currentInfo;
GdiplusStartupInput g_gdiplusStartupInput;
ULONG_PTR g_gdiplusToken;

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateTrayIcon(HWND hWnd);
void ShowContextMenu(HWND hWnd);
void UpdateUI();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Single instance check
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"OSMV_Lite_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hWndExisting = FindWindow(L"OSMV_Lite_Class", NULL);
        if (hWndExisting) {
            ShowWindow(hWndExisting, SW_SHOW);
            ShowWindow(hWndExisting, SW_RESTORE);
            SetForegroundWindow(hWndExisting);
        }
        return 0;
    }

    // Initialize WinRT
    init_apartment(apartment_type::multi_threaded);

    // Initialize GDI+
    GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);

    // Register Window Class
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OSMV_Lite_Class";
    wc.hIcon = (HICON)LoadImage(NULL, L"OSMV_logo.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!wc.hIcon) {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(24, 24, 26)); // #18181A
    
    RegisterClassEx(&wc);

    // Create Window
    g_hWnd = CreateWindowEx(
        0, L"OSMV_Lite_Class", L"OBS Stream Music Viewer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 150,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) return 0;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    CreateTrayIcon(g_hWnd);

    try {
        // Initialize Tracker
        g_tracker = std::make_unique<MediaTracker>([](const MediaTracker::MediaInfo& info) {
            g_currentInfo = info;
            PostMessage(g_hWnd, WM_USER + 2, 0, 0); // Trigger UI update
        });
    } catch (winrt::hresult_error const& ex) {
        MessageBox(NULL, ex.message().c_str(), L"WinRT Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    // Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_USER + 2: // UI Update
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        Graphics graphics(hdc);
        
        // Font settings
        FontFamily fontFamily(L"Segoe UI");
        Gdiplus::Font titleFont(&fontFamily, 16, FontStyleBold, UnitPixel);
        Gdiplus::Font artistFont(&fontFamily, 14, FontStyleRegular, UnitPixel);
        Gdiplus::Font statusFont(&fontFamily, 12, FontStyleRegular, UnitPixel);
        
        SolidBrush whiteBrush(Color(255, 255, 255));
        SolidBrush grayBrush(Color(176, 176, 176));
        SolidBrush dimBrush(Color(96, 96, 96));

        // Draw Text
        RectF titleRect(110, 20, 250, 30);
        RectF artistRect(110, 50, 250, 25);
        RectF statusRect(110, 85, 250, 20);

        graphics.DrawString(g_currentInfo.active ? g_currentInfo.title.c_str() : L"Waiting for music...", -1, &titleFont, titleRect, NULL, &whiteBrush);
        graphics.DrawString(g_currentInfo.active ? g_currentInfo.artist.c_str() : L"---", -1, &artistFont, artistRect, NULL, &grayBrush);
        
        std::wstring statusText = L"Status: " + std::wstring(g_currentInfo.status.begin(), g_currentInfo.status.end());
        graphics.DrawString(statusText.c_str(), -1, &statusFont, statusRect, NULL, &dimBrush);

        // Draw Album Art
        if (g_currentInfo.active && !g_currentInfo.thumbnailData.empty()) {
            IStream* pStream = SHCreateMemStream(g_currentInfo.thumbnailData.data(), static_cast<UINT>(g_currentInfo.thumbnailData.size()));
            if (pStream) {
                Image image(pStream);
                graphics.DrawImage(&image, 10, 20, 90, 90);
                pStream->Release();
            }
        } else {
            Pen borderPen(Color(40, 40, 42), 1);
            graphics.DrawRectangle(&borderPen, 10, 20, 90, 90);
            
            // Draw default music icon or placeholder
            graphics.DrawString(L"🎵", -1, &titleFont, RectF(40, 50, 40, 40), NULL, &dimBrush);
        }

        EndPaint(hWnd, &ps);
    } break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            ShowWindow(hWnd, SW_HIDE);
        }
        break;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowContextMenu(hWnd);
        } else if (lParam == WM_LBUTTONDBLCLK) {
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_EXIT:
            Shell_NotifyIcon(NIM_DELETE, &g_nid);
            PostQuitMessage(0);
            break;
        case ID_TRAY_SHOW:
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            break;
        }
        break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void CreateTrayIcon(HWND hWnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    
    // Try to load our custom icon
    HICON hIcon = (HICON)LoadImage(NULL, L"OSMV_logo.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    
    if (hIcon) {
        g_nid.hIcon = hIcon;
    } else {
        g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    wcscpy_s(g_nid.szTip, ARRAYSIZE(g_nid.szTip), L"OBS Stream Music Viewer");
    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

void ShowContextMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, L"Afficher");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"Quitter");
    
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}
