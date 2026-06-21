#pragma once

#include <windows.h>
#include <cstdint>

class ISteamFriends;
class ISteamUser;

static constexpr int kSteamErrMsgSize = 1024;

enum SteamAPIInitResult {
    kSteamAPI_OK = 0,
    kSteamAPI_FailedGeneric = 1,
    kSteamAPI_NoSteamClient = 2,
    kSteamAPI_VersionMismatch = 3,
};

// Dynamic loader for steam_api.dll via LoadLibrary + GetProcAddress (no MinGW
// import library needed). Uses the modern flat-API exports.
class SteamAPILoader {
public:
    SteamAPILoader() = default;
    ~SteamAPILoader();

    bool Load();
    void Unload();

    bool IsLoaded() const { return m_module != nullptr; }

    int  (*SteamAPI_InitFlat)(char* pOutErrMsg) = nullptr;
    void (*SteamAPI_Shutdown)() = nullptr;
    void (*SteamAPI_RunCallbacks)() = nullptr;

    ISteamFriends* (*SteamAPI_SteamFriends_v018)() = nullptr;
    bool (*SteamAPI_ISteamFriends_SetRichPresence)(ISteamFriends* self, const char* key, const char* value) = nullptr;
    const char* (*SteamAPI_ISteamFriends_GetPersonaName)(ISteamFriends* self) = nullptr;

    ISteamUser* (*SteamAPI_SteamUser_v023)() = nullptr;
    uint64_t    (*SteamAPI_ISteamUser_GetSteamID)(ISteamUser* self) = nullptr;

private:
    HMODULE m_module = nullptr;
};
