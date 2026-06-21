#include "SteamAPI.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>

#include "Log.h"

namespace {
// Address anchor — a function whose address is known to live in our own DLL.
// Used with GetModuleHandleEx to locate the DLL's path at runtime.
void SelfAnchor() {}
}

SteamAPILoader::~SteamAPILoader() {
    Unload();
}

bool SteamAPILoader::Load() {
    if (m_module) return true;

    // Try loading steam_api.dll from the same directory as our own DLL first
    // (so the user can drop both in any Plugins/ folder — UserDir, coop, etc.).
    char path[MAX_PATH] = {0};
    HMODULE selfMod = nullptr;
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&SelfAnchor),
            &selfMod) &&
        GetModuleFileNameA(selfMod, path, MAX_PATH) > 0) {
        if (char* lastSep = std::strrchr(path, '\\')) {
            *(lastSep + 1) = '\0';
            std::strncat(path, "steam_api.dll", MAX_PATH - std::strlen(path) - 1);
            m_module = LoadLibraryA(path);
            if (m_module) Logf("[Steam] loaded from \"%s\"", path);
        }
    }

    // Fallback: default Windows DLL search order (looks next to SimCity 4.exe).
    if (!m_module) {
        m_module = LoadLibraryA("steam_api.dll");
        if (m_module) Log("[Steam] loaded via default search order");
    }

    if (!m_module) {
        Logf("[Steam] LoadLibrary failed, GetLastError=%lu", GetLastError());
        return false;
    }

    auto resolve = [this](const char* name, void** out) -> bool {
        FARPROC p = GetProcAddress(m_module, name);
        if (!p) {
            Logf("[Steam] GetProcAddress(%s) failed, GetLastError=%lu", name, GetLastError());
            return false;
        }
        *out = reinterpret_cast<void*>(p);
        return true;
    };

    bool ok = true;
    ok &= resolve("SteamAPI_InitFlat",                       reinterpret_cast<void**>(&SteamAPI_InitFlat));
    ok &= resolve("SteamAPI_Shutdown",                       reinterpret_cast<void**>(&SteamAPI_Shutdown));
    ok &= resolve("SteamAPI_RunCallbacks",                   reinterpret_cast<void**>(&SteamAPI_RunCallbacks));
    ok &= resolve("SteamAPI_SteamFriends_v018",              reinterpret_cast<void**>(&SteamAPI_SteamFriends_v018));
    ok &= resolve("SteamAPI_ISteamFriends_SetRichPresence",  reinterpret_cast<void**>(&SteamAPI_ISteamFriends_SetRichPresence));
    ok &= resolve("SteamAPI_ISteamFriends_GetPersonaName",   reinterpret_cast<void**>(&SteamAPI_ISteamFriends_GetPersonaName));
    ok &= resolve("SteamAPI_SteamUser_v023",                 reinterpret_cast<void**>(&SteamAPI_SteamUser_v023));
    ok &= resolve("SteamAPI_ISteamUser_GetSteamID",          reinterpret_cast<void**>(&SteamAPI_ISteamUser_GetSteamID));

    if (!ok) {
        Unload();
        return false;
    }
    return true;
}

void SteamAPILoader::Unload() {
    if (m_module) {
        FreeLibrary(m_module);
        m_module = nullptr;
        SteamAPI_InitFlat = nullptr;
        SteamAPI_Shutdown = nullptr;
        SteamAPI_RunCallbacks = nullptr;
        SteamAPI_SteamFriends_v018 = nullptr;
        SteamAPI_ISteamFriends_SetRichPresence = nullptr;
        SteamAPI_ISteamFriends_GetPersonaName = nullptr;
        SteamAPI_SteamUser_v023 = nullptr;
        SteamAPI_ISteamUser_GetSteamID = nullptr;
    }
}
