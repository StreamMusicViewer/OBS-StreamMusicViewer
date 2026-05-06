#pragma once

#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <functional>
#include <shlwapi.h>
#include <vector>
#include "Base64.hpp"

#pragma comment(lib, "shlwapi.lib")

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;

class MediaTracker {
public:
    struct MediaInfo {
        std::wstring title;
        std::wstring artist;
        std::wstring album;
        std::string status;
        std::string thumbnailBase64;
        std::vector<unsigned char> thumbnailData;
        bool active = false;
    };

    typedef std::function<void(const MediaInfo&)> UpdateCallback;

    MediaTracker(UpdateCallback callback) : m_callback(callback) {
        InitAsync();
    }

    ~MediaTracker() {
        if (m_sessionManager) {
            m_sessionManager.CurrentSessionChanged(m_sessionChangedToken);
        }
        UnregisterSessionEvents();
    }

private:
    GlobalSystemMediaTransportControlsSessionManager m_sessionManager{ nullptr };
    GlobalSystemMediaTransportControlsSession m_currentSession{ nullptr };
    event_token m_sessionChangedToken;
    event_token m_propsChangedToken;
    event_token m_playbackChangedToken;
    UpdateCallback m_callback;
    std::recursive_mutex m_mutex;

    fire_and_forget InitAsync() {
        m_sessionManager = co_await GlobalSystemMediaTransportControlsSessionManager::RequestAsync();
        
        m_sessionChangedToken = m_sessionManager.CurrentSessionChanged([this](auto&&...) {
            UpdateCurrentSession();
        });

        UpdateCurrentSession();
    }

    void UpdateCurrentSession() {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        
        UnregisterSessionEvents();

        m_currentSession = m_sessionManager.GetCurrentSession();

        if (m_currentSession) {
            m_propsChangedToken = m_currentSession.MediaPropertiesChanged([this](auto&&...) {
                RefreshMediaInfo();
            });
            m_playbackChangedToken = m_currentSession.PlaybackInfoChanged([this](auto&&...) {
                RefreshMediaInfo();
            });
        }

        RefreshMediaInfo();
    }

    void UnregisterSessionEvents() {
        if (m_currentSession) {
            m_currentSession.MediaPropertiesChanged(m_propsChangedToken);
            m_currentSession.PlaybackInfoChanged(m_playbackChangedToken);
            m_currentSession = nullptr;
        }
    }

    fire_and_forget RefreshMediaInfo() {
        GlobalSystemMediaTransportControlsSession session{ nullptr };
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            session = m_currentSession;
        }

        MediaInfo info;
        if (!session) {
            info.active = false;
            info.status = "closed";
        } else {
            info.active = true;
            try {
                auto playback = session.GetPlaybackInfo();
                if (playback) {
                    switch (playback.PlaybackStatus()) {
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing: info.status = "playing"; break;
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:  info.status = "paused";  break;
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped: info.status = "stopped"; break;
                        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Closed:  info.status = "closed";  break;
                        default: info.status = "unknown"; break;
                    }
                }

                auto props = co_await session.TryGetMediaPropertiesAsync();
                if (props) {
                    info.title = props.Title();
                    info.artist = props.Artist();
                    info.album = props.AlbumTitle();

                    auto thumbnail = props.Thumbnail();
                    if (thumbnail) {
                        auto stream = co_await thumbnail.OpenReadAsync();
                        auto size = stream.Size();
                        Buffer buffer(static_cast<uint32_t>(size));
                        co_await stream.ReadAsync(buffer, static_cast<uint32_t>(size), InputStreamOptions::None);
                        
                        // Accessing raw data from WinRT buffer
                        info.thumbnailData.assign(buffer.data(), buffer.data() + buffer.Length());
                        info.thumbnailBase64 = Base64::Encode(buffer.data(), buffer.Length());
                    }
                }
            } catch (...) {
                info.active = false;
                info.status = "error";
            }
        }

        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            ExportToJson(info);
            if (m_callback) m_callback(info);
        }
    }

    void ExportToJson(const MediaInfo& info) {
        std::wstring finalPath = L"current_song.json";
        
        if (!PathFileExists(L"index.html") && PathFileExists(L"..\\index.html")) {
            finalPath = L"..\\current_song.json";
        }

        std::wstring tempPath = finalPath + L".tmp";
        {
            // Use std::ofstream for UTF-8
            std::ofstream file(tempPath);
            if (!file.is_open()) return;

            if (!info.active || (info.title.empty() && info.artist.empty())) {
                file << "null";
            } else {
                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                struct tm timeinfo;
                localtime_s(&timeinfo, &in_time_t);
                std::stringstream ss;
                ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");

                file << "{\n";
                file << "  \"title\": \"" << EscapeJson(WideToUTF8(info.title)) << "\",\n";
                file << "  \"artist\": \"" << EscapeJson(WideToUTF8(info.artist)) << "\",\n";
                file << "  \"album\": \"" << EscapeJson(WideToUTF8(info.album)) << "\",\n";
                file << "  \"thumbnail\": \"" << info.thumbnailBase64 << "\",\n";
                file << "  \"status\": \"" << info.status << "\",\n";
                file << "  \"timestamp\": \"" << ss.str() << "\"\n";
                file << "}";
            }
            file.flush();
            file.close();
        }
        MoveFileEx(tempPath.c_str(), finalPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }

    std::string WideToUTF8(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    std::string EscapeJson(const std::string& s) {
        std::string out;
        for (auto c : s) {
            if (c == '\"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\b') out += "\\b";
            else if (c == '\f') out += "\\f";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    }
};
