#pragma once

#include "SteamAPI.h"
#include <string>

class RichPresenceService {
public:
    explicit RichPresenceService(SteamAPILoader& steam) : m_steam(steam) {}

    // Calls SteamAPI_Init(). Returns false if Steam is not running or AppID
    // resolution failed. Service is no-op after failed Init.
    bool Init();

    // Clears rich presence + SteamAPI_Shutdown.
    void Shutdown();

    // High-level events: update presence based on game state.
    void OnCityEntered(const std::string& cityName,
                       const std::string& regionName,
                       int32_t population);

    void OnCityLeft();

    void OnMonthPassed(const std::string& cityName, int32_t residentialPop);

    bool IsReady() const { return m_initialized; }

    // Identity accessors. Return empty/0 if not initialized.
    const char* GetPersonaName() const;
    uint64_t    GetSteamID64() const;

private:
    void SetStatus(const std::string& status);

    SteamAPILoader& m_steam;
    ISteamFriends* m_friends = nullptr;
    bool m_initialized = false;
};
