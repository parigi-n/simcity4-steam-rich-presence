#include "RichPresenceService.h"

#include "Log.h"

bool RichPresenceService::Init() {
    if (!m_steam.SteamAPI_InitFlat) {
        Log("[Presence] steam loader not ready");
        return false;
    }

    char errMsg[kSteamErrMsgSize] = {0};
    int result = m_steam.SteamAPI_InitFlat(errMsg);
    if (result != kSteamAPI_OK) {
        Logf("[Presence] SteamAPI_InitFlat failed, result=%d msg=\"%s\"", result, errMsg);
        return false;
    }

    if (m_steam.SteamAPI_SteamFriends_v018) {
        m_friends = m_steam.SteamAPI_SteamFriends_v018();
    }
    m_initialized = true;
    Logf("[Presence] init OK, friends=%p", (void*)m_friends);
    return true;
}

void RichPresenceService::Shutdown() {
    if (!m_initialized) return;
    SetStatus("");
    if (m_steam.SteamAPI_Shutdown) m_steam.SteamAPI_Shutdown();
    m_initialized = false;
    m_friends = nullptr;
    Log("[Presence] shutdown");
}

void RichPresenceService::SetStatus(const std::string& status) {
    if (!m_initialized || !m_friends) return;
    // "status" key shows in Steam's "View Game Info" dialog. Main friend list
    // line needs "steam_display" + backend tokens (only the AppID owner can
    // register those, SC4 owner EA has none).
    m_steam.SteamAPI_ISteamFriends_SetRichPresence(m_friends, "status", status.c_str());
    if (m_steam.SteamAPI_RunCallbacks) m_steam.SteamAPI_RunCallbacks();
    Logf("[Presence] set status \"%s\"", status.c_str());
}

void RichPresenceService::OnCityEntered(const std::string& cityName,
                                       const std::string& regionName,
                                       int32_t population) {
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "%s, %s (pop. %d)",
                  regionName.c_str(), cityName.c_str(), population);
    SetStatus(buf);
}

void RichPresenceService::OnCityLeft() {
    SetStatus("");
}

void RichPresenceService::OnMonthPassed(const std::string& cityName, int32_t residentialPop) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s, pop. %d", cityName.c_str(), residentialPop);
    SetStatus(buf);
}

const char* RichPresenceService::GetPersonaName() const {
    if (!m_initialized || !m_friends || !m_steam.SteamAPI_ISteamFriends_GetPersonaName)
        return "";
    const char* name = m_steam.SteamAPI_ISteamFriends_GetPersonaName(m_friends);
    return name ? name : "";
}

uint64_t RichPresenceService::GetSteamID64() const {
    if (!m_initialized || !m_steam.SteamAPI_SteamUser_v023 || !m_steam.SteamAPI_ISteamUser_GetSteamID)
        return 0;
    ISteamUser* user = m_steam.SteamAPI_SteamUser_v023();
    if (!user) return 0;
    return m_steam.SteamAPI_ISteamUser_GetSteamID(user);
}
