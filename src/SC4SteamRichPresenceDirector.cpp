#include <windows.h>
#include <cstdio>
#include <cstdint>

#include "Log.h"
#include "RichPresenceService.h"
#include "SteamAPI.h"

#include "cIGZApp.h"
#include "cIGZCheatCodeManager.h"
#include "cIGZFrameWork.h"
#include "cIGZMessage2.h"
#include "cIGZMessage2Standard.h"
#include "cIGZMessageServer2.h"
#include "cIGZString.h"
#include "cIGZWinMgr.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISC4Region.h"
#include "cISC4ResidentialSimulator.h"
#include "cRZBaseString.h"
#include "cRZMessage2COMDirector.h"
#include "GZCLSIDDefs.h"
#include "GZServPtrs.h"
#include "SC4String.h"

// Unique director ID. Generated random 32-bit value.
static const uint32_t kSC4SteamRichPresenceDirectorID = 0x6A3D9F71;

// SC4 message IDs.
static const uint32_t kGZMSG_CityInited   = 0x26d31ec1;
static const uint32_t kGZMSG_CityShutdown = 0x26d31ec2;
static const uint32_t kGZMSG_MonthPassed  = 0x66956816;
static const uint32_t kGZMSG_CheatIssued  = 0x230e27ac;

// GZIID_cISC4App comes from cISC4App.h.
static const uint32_t kGZIID_cIGZCheatCodeManager = 0xa1085722;

static const uint32_t kDebugCheatID = 0x6A3DD0B6;
static const char*    kDebugCheatStr = "sc4srp_debug";

class SC4SteamRichPresenceDirector : public cRZMessage2COMDirector {
public:
    SC4SteamRichPresenceDirector() : m_presence(m_steam) {}

    uint32_t GetDirectorID() const override {
        return kSC4SteamRichPresenceDirectorID;
    }

    // Override upstream cRZMessage2COMDirector::QueryInterface which recurses
    // into itself instead of falling back to cRZCOMDllDirector. Without this
    // override, any QI for an IID other than cIGZMessageTarget2 stack-overflows.
    bool QueryInterface(uint32_t riid, void** ppvObj) override {
        if (riid == GZCLSID::kcIGZMessageTarget2) {
            *ppvObj = static_cast<cIGZMessageTarget2*>(this);
            AddRef();
            return true;
        }
        return cRZCOMDllDirector::QueryInterface(riid, ppvObj);
    }

    bool OnStart(cIGZCOM*) {
        cIGZFrameWork* fw = RZGetFrameWork();
        if (fw->GetState() < cIGZFrameWork::kStatePreAppInit) {
            fw->AddHook(this);
        } else {
            PreAppInit();
        }
        return true;
    }

    bool PostAppInit() {
        InitLogging();
        Log("[Director] PostAppInit");

        if (m_steam.Load() && m_presence.Init()) {
            Log("[Director] Steam Rich Presence ready");
        } else {
            Log("[Director] Rich Presence disabled (Steam not loaded or init failed)");
        }

        RegisterDebugCheat();

        cIGZMessageServer2Ptr pMsgServ;
        if (pMsgServ) {
            pMsgServ->AddNotification(this, kGZMSG_CityInited);
            pMsgServ->AddNotification(this, kGZMSG_CityShutdown);
            pMsgServ->AddNotification(this, kGZMSG_MonthPassed);
        } else {
            Log("[Director] ERROR: cIGZMessageServer2 not available");
        }
        return true;
    }

    bool PreAppShutdown() {
        Log("[Director] PreAppShutdown");
        cIGZMessageServer2Ptr pMsgServ;
        if (pMsgServ) {
            pMsgServ->RemoveNotification(this, kGZMSG_CityInited);
            pMsgServ->RemoveNotification(this, kGZMSG_CityShutdown);
            pMsgServ->RemoveNotification(this, kGZMSG_MonthPassed);
        }
        m_presence.Shutdown();
        m_steam.Unload();
        return true;
    }

    bool DoMessage(cIGZMessage2* pMessage) override {
        const uint32_t type = pMessage->GetType();
        if (type == kGZMSG_CityInited) {
            OnCityEntered();
        } else if (type == kGZMSG_CityShutdown) {
            OnCityLeft();
        } else if (type == kGZMSG_MonthPassed) {
            OnMonthPassed();
        } else if (type == kGZMSG_CheatIssued) {
            cIGZMessage2Standard* std = static_cast<cIGZMessage2Standard*>(pMessage);
            if (std->GetData1() == kDebugCheatID) {
                OnDebugCheat();
            }
        }
        return true;
    }

private:
    cISC4App* GetSC4App() {
        cIGZFrameWork* fw = RZGetFrameWork();
        if (!fw) return nullptr;
        cIGZApp* app = fw->Application();
        if (!app) return nullptr;
        cISC4App* sc4App = nullptr;
        if (!app->QueryInterface(GZIID_cISC4App, (void**)&sc4App)) return nullptr;
        return sc4App;
    }

    struct CityInfo {
        const char* city = "(none)";
        const char* region = "(none)";
        int32_t residentialPop = 0;
        cRZBaseString cityNameBuf;
    };

    CityInfo Gather() {
        CityInfo info;
        cISC4App* sc4App = GetSC4App();
        if (!sc4App) return info;

        if (cISC4City* city = sc4App->GetCity()) {
            if (city->GetCityName(info.cityNameBuf)) info.city = info.cityNameBuf.ToChar();
            if (cISC4ResidentialSimulator* sim = city->GetResidentialSimulator()) {
                info.residentialPop = sim->GetPopulation();
            }
        }
        if (cISC4Region* region = sc4App->GetRegion()) {
            if (SC4String* sc4Str = region->GetName()) {
                if (const cIGZString* igzStr = sc4Str->AsIGZString()) {
                    info.region = igzStr->ToChar();
                }
            }
        }
        return info;
    }

    void OnCityEntered() {
        CityInfo info = Gather();
        Logf("[Director] CityEntered city=\"%s\" region=\"%s\" pop=%d",
             info.city, info.region, info.residentialPop);
        m_presence.OnCityEntered(info.city, info.region, info.residentialPop);
    }

    void OnCityLeft() {
        Log("[Director] CityShutdown");
        m_presence.OnCityLeft();
    }

    void OnMonthPassed() {
        CityInfo info = Gather();
        Logf("[Director] MonthPassed city=\"%s\" pop=%d", info.city, info.residentialPop);
        m_presence.OnMonthPassed(info.city, info.residentialPop);
    }

    void RegisterDebugCheat() {
        cISC4App* sc4App = GetSC4App();
        if (!sc4App) return;
        cIGZCheatCodeManager* pCheatMgr = sc4App->GetCheatCodeManager();
        if (!pCheatMgr) return;
        if (!pCheatMgr->QueryInterface(kGZIID_cIGZCheatCodeManager, (void**)&pCheatMgr)) return;
        pCheatMgr->AddNotification2(this, 0);
        cRZBaseString name(kDebugCheatStr);
        pCheatMgr->RegisterCheatCode(kDebugCheatID, name);
        Logf("[Director] cheat registered: type \"%s\" in console (Ctrl+X)", kDebugCheatStr);
    }

    void OnDebugCheat() {
        CityInfo info = Gather();

        const char* personaName = m_presence.GetPersonaName();
        uint64_t steamId = m_presence.GetSteamID64();

        char buf[1024];
        std::snprintf(buf, sizeof(buf),
                      "City        : %s\n"
                      "Region      : %s\n"
                      "Population  : %d\n"
                      "\n"
                      "Steam name  : %s\n"
                      "SteamID64   : %llu\n"
                      "Steam state : %s\n"
                      "\n"
                      "(See sc4srp.log for full details)",
                      info.city, info.region, info.residentialPop,
                      personaName && *personaName ? personaName : "(n/a)",
                      (unsigned long long)steamId,
                      m_presence.IsReady() ? "connected" : "unavailable");

        cIGZWinMgrPtr pWinMgr;
        if (!pWinMgr) {
            Log("[Director] dashboard: WinMgr unavailable");
            return;
        }
        // gzcom-dll header says (title, message) but SC4 actually expects
        // (message, title). Matches terrain-extensions usage.
        cRZBaseString title("Plugin Dashboard");
        cRZBaseString msg(buf);
        pWinMgr->GZMsgBox(msg, title, 0, true, 0);
    }

    SteamAPILoader m_steam;
    RichPresenceService m_presence;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
    static SC4SteamRichPresenceDirector sDirector;
    return &sDirector;
}
