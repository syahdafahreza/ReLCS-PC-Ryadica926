#include "CheatManager.h"
#include "CrashLogger.h"

namespace ReLCS
{
    std::vector<CheatInfo> CheatManager::s_cheatList;
    std::atomic<bool> CheatManager::s_bNeverWanted(false);
    std::atomic<bool> CheatManager::s_bUnlimitedAmmo(false);
    std::atomic<bool> CheatManager::s_bUpsideDown(false);
    std::string CheatManager::s_lastActivatedName = "";
    float CheatManager::s_notificationAlpha = 0.0f;
    std::chrono::steady_clock::time_point CheatManager::s_notificationTime;

    uintptr_t CheatManager::s_pTimeScale = 0;
    uintptr_t CheatManager::s_pWeather = 0;
    uintptr_t CheatManager::s_pScriptSpace = 0;
    HWND CheatManager::s_hGameWnd = nullptr;

    void CheatManager::Init()
    {
        s_cheatList = {
            // Player Cheats
            { LCSCheat::FullHealth, "Full Health", "Player", "Restores player health to 100%", "GRABEMPILLS" },
            { LCSCheat::FullArmor, "Full Armor", "Player", "Restores body armor to 100%", "JUSTKEVLAR" },
            { LCSCheat::HealthArmorMoney250K, "$250,000 + Health + Armor", "Player", "Gives $250k cash and full restoration", "NOTEVENRICH" },
            { LCSCheat::WeaponSet1, "Weapon Set 1", "Player", "Brass knuckles, knife, Molotov, 9mm, shotgun, etc.", "TERRORIST" },
            { LCSCheat::WeaponSet2, "Weapon Set 2", "Player", "Katana, grenades, .357, Uzi, M4, rocket launcher", "PALETO" },
            { LCSCheat::WeaponSet3, "Weapon Set 3", "Player", "Chainsaw, grenades, laser sniper, spaz, minigun", "ICANCU" },
            { LCSCheat::ClearWantedLevel, "Clear Wanted Level", "Player", "Removes all wanted stars immediately", "POLITICALIMUNITY" },
            { LCSCheat::RaiseWantedLevel, "Raise Wanted Level (+2)", "Player", "Increases wanted level by 2 stars", "CROOKEDTONI" },
            { LCSCheat::NeverWanted, "Never Wanted [Toggle]", "Player", "Keeps wanted level permanently at zero", "POLITICALIMUNITY" },
            { LCSCheat::UnlimitedAmmo, "Unlimited Ammo [Toggle]", "Player", "Infinite ammo and no reload for all weapons", "" },
            { LCSCheat::CommitSuicide, "Commit Suicide", "Player", "Instantly eliminates the player character", "KYSMATE" },

            // Vehicle Cheats
            { LCSCheat::SpawnRhino, "Spawn Rhino Tank", "Vehicle", "Spawns a Rhino military tank near player", "TOOLOFPEACE" },
            { LCSCheat::SpawnTrashmaster, "Spawn Trashmaster", "Vehicle", "Spawns a city garbage truck", "UNTOUCHABLE" },
            { LCSCheat::SpawnDeaddodo, "Spawn Deaddodo", "Vehicle", "Spawns an airplane near player", "AIRSHIP" },
            { LCSCheat::DestroyAllCars, "Blow Up All Cars", "Vehicle", "Detonates all vehicles in area", "ALTRIGHT" },
            { LCSCheat::PerfectHandling, "Perfect Traction & Handling", "Vehicle", "Maximizes vehicle grip and cornering", "BETTERBIKES" },
            { LCSCheat::AggressiveDrivers, "Aggressive Drivers", "Vehicle", "Drivers ram into other cars aggressively", "MAXMAD" },
            { LCSCheat::ChromeCars, "White / Chrome Cars", "Vehicle", "Repaints all city vehicles white/chrome", "SOUTHTRAFFIC" },
            { LCSCheat::BlackCars, "Black Cars", "Vehicle", "Repaints all city vehicles black", "BCARSMATTER" },
            { LCSCheat::CarsDriveOnWater, "Cars Drive On Water", "Vehicle", "Vehicles float and drive across water", "JESUSCAR" },

            // World Cheats
            { LCSCheat::SunnyWeather, "Sunny Weather", "World", "Sets weather to clear sunshine", "DAYOFSUN" },
            { LCSCheat::ClearWeather, "Clear Weather", "World", "Sets weather to pleasant clear skies", "IAMVAMPIRE" },
            { LCSCheat::OvercastWeather, "Overcast Weather", "World", "Sets weather to overcast clouds", "CATSANDDOGS" },
            { LCSCheat::RainyWeather, "Rainy Weather", "World", "Sets weather to heavy rainfall", "RAINDROPLETS" },
            { LCSCheat::FoggyWeather, "Foggy Weather", "World", "Sets weather to thick low-visibility fog", "FOGGINWEATHER" },
            { LCSCheat::FasterClock, "Faster Clock", "World", "Accelerates clock and day/night speed", "LIFEISPASSINGMEBY" },
            { LCSCheat::TrafficLightsGreen, "Green Traffic Lights", "World", "Turns all traffic lights green", "SUNDAYDRIVE" },
            { LCSCheat::TrafficLightsRed, "Red Traffic Lights", "World", "Turns all traffic lights red", "LUCKYDAY" },
            { LCSCheat::PedRiot, "Pedestrian Riot", "World", "Pedestrians riot and fight in streets", "SMARTWORLD" },
            { LCSCheat::PedAttackYou, "Pedestrians Attack You", "World", "All pedestrians become hostile to player", "ANOTHERFAF" },
            { LCSCheat::PedHaveWeapons, "Armed Pedestrians", "World", "Pedestrians carry dangerous firearms", "NEXTPRESIDENT" },
            { LCSCheat::PedFollowYou, "Pedestrians Follow You", "World", "Nearby pedestrians follow the player", "HOPINWEMAGA" },

            // Visual & Gameplay Cheats
            { LCSCheat::SlowMotion, "Slow Motion", "Visual/Gameplay", "Slows game speed down", "NOTTURBO" },
            { LCSCheat::FastMotion, "Fast Motion", "Visual/Gameplay", "Speeds game motion up", "TURBOBUTTON" },
            { LCSCheat::UpsideDownMode, "Upside Down Screen", "Visual/Gameplay", "Flips viewport upside down", "" },
            { LCSCheat::NormalOrientationMode, "Normal Screen Orientation", "Visual/Gameplay", "Restores normal viewport orientation", "" }
        };

        FindGameOffsets();
        CrashLogger::LogMessage("[CheatManager] Initialized with 36 Re:LCS PC cheats.");
    }

    void CheatManager::FindGameOffsets()
    {
        // Find main game window
        s_hGameWnd = FindWindowA("Grand theft auto 3", nullptr);
        if (!s_hGameWnd)
            s_hGameWnd = FindWindowA("Grand Theft Auto", nullptr);
        if (!s_hGameWnd)
            s_hGameWnd = FindWindowA(nullptr, "GTA: Liberty City Stories");
        if (!s_hGameWnd)
            s_hGameWnd = GetActiveWindow();
    }

    static void SafeCallAddCheat(void* pPad, char c)
    {
        typedef void(__thiscall* AddToPCCheatString_t)(void*, char);
        AddToPCCheatString_t pfnAddCheat = reinterpret_cast<AddToPCCheatString_t>(0x4ABD20);
        __try
        {
            pfnAddCheat(pPad, c);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeAddMoneyAndRestore(uintptr_t pPlayerInfo, int amount)
    {
        __try
        {
            HMODULE hReLCS = GetModuleHandleA("ReLCS.dll");
            if (hReLCS)
            {
                typedef void (*ReLCSCheatFn_t)();
                ReLCSCheatFn_t pfnMoneyCheat = reinterpret_cast<ReLCSCheatFn_t>(reinterpret_cast<uintptr_t>(hReLCS) + 0x25af0);
                pfnMoneyCheat();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        __try
        {
            int* pMoney = reinterpret_cast<int*>(pPlayerInfo + 0xA0);
            int* pDisplayMoney = reinterpret_cast<int*>(pPlayerInfo + 0xA4);
            *pMoney += amount;
            *pDisplayMoney += amount;

            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (pPed)
            {
                *reinterpret_cast<float*>(pPed + 0x354) = 100.0f; // Health
                *reinterpret_cast<float*>(pPed + 0x358) = 100.0f; // Armour
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeSetHealth(uintptr_t pPlayerInfo, float health)
    {
        __try
        {
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (pPed)
                *reinterpret_cast<float*>(pPed + 0x354) = health;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeSetArmour(uintptr_t pPlayerInfo, float armour)
    {
        __try
        {
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (pPed)
                *reinterpret_cast<float*>(pPed + 0x358) = armour;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafePatchUnlimitedAmmo(bool enable)
    {
        __try
        {
            uintptr_t addrs[] = { 0x5d4abe, 0x5d4af5, 0x5d4546, 0x5d457d };
            uint8_t nopBytes[3] = { 0x90, 0x90, 0x90 };
            uint8_t origBytes[4][3] = {
                { 0xFF, 0x4E, 0x08 },
                { 0xFF, 0x4E, 0x0C },
                { 0xFF, 0x4E, 0x08 },
                { 0xFF, 0x4E, 0x0C }
            };

            for (int i = 0; i < 4; i++)
            {
                DWORD oldProtect;
                if (VirtualProtect(reinterpret_cast<void*>(addrs[i]), 3, PAGE_EXECUTE_READWRITE, &oldProtect))
                {
                    if (enable)
                        memcpy(reinterpret_cast<void*>(addrs[i]), nopBytes, 3);
                    else
                        memcpy(reinterpret_cast<void*>(addrs[i]), origBytes[i], 3);

                    VirtualProtect(reinterpret_cast<void*>(addrs[i]), 3, oldProtect, &oldProtect);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeRefillPlayerAmmo(uintptr_t pPlayerInfo)
    {
        __try
        {
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (pPed)
            {
                for (int slot = 0; slot < 10; slot++)
                {
                    uintptr_t pWeapon = pPed + 0x408 + (slot * 0x18);
                    int weaponType = *reinterpret_cast<int*>(pWeapon);
                    if (weaponType > 0)
                    {
                        *reinterpret_cast<int*>(pWeapon + 0x08) = 9999; // m_nAmmoInClip
                        *reinterpret_cast<int*>(pWeapon + 0x0C) = 9999; // m_nAmmoTotal
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeSetWantedLevel(uintptr_t pPlayerInfo, int level, bool freezeZero = false)
    {
        __try
        {
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (pPed)
            {
                uintptr_t pWanted = *reinterpret_cast<uintptr_t*>(pPed + 0x5F4);
                if (pWanted)
                {
                    *reinterpret_cast<int*>(pWanted + 0x00) = (level <= 0) ? 0 : (level * 150);
                    *reinterpret_cast<int*>(pWanted + 0x20) = level;
                    *reinterpret_cast<int*>(pWanted + 0x24) = level;

                    if (freezeZero)
                    {
                        *reinterpret_cast<bool*>(pWanted + 0x28) = true; // m_bIgnoredByPolice
                        *reinterpret_cast<bool*>(pWanted + 0x29) = true; // m_bIgnoredByEveryone
                    }
                    else
                    {
                        *reinterpret_cast<bool*>(pWanted + 0x28) = false;
                        *reinterpret_cast<bool*>(pWanted + 0x29) = false;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeRaiseWantedLevel(uintptr_t pPlayerInfo, int addLevels)
    {
        __try
        {
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (pPed)
            {
                uintptr_t pWanted = *reinterpret_cast<uintptr_t*>(pPed + 0x5F4);
                if (pWanted)
                {
                    int cur = *reinterpret_cast<int*>(pWanted + 0x20);
                    if (cur < 6)
                    {
                        cur += addLevels;
                        if (cur > 6) cur = 6;
                        *reinterpret_cast<int*>(pWanted + 0x20) = cur;
                        *reinterpret_cast<int*>(pWanted + 0x00) = cur * 150;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void CheatManager::SimulateCheatCode(const std::string& code)
    {
        if (code.empty())
            return;

        void* pPad = reinterpret_cast<void*>(0x7DBCB0);
        for (char c : code)
        {
            char upperChar = static_cast<char>(toupper(static_cast<unsigned char>(c)));
            SafeCallAddCheat(pPad, upperChar);
        }
    }

    bool CheatManager::ActivateCheat(LCSCheat cheat)
    {
        const CheatInfo* info = GetCheatInfo(cheat);
        if (!info)
            return false;

        const uintptr_t pPlayerInfo = 0x94AD28;

        switch (cheat)
        {
        case LCSCheat::NeverWanted:
            s_bNeverWanted = !s_bNeverWanted;
            if (s_bNeverWanted)
            {
                SimulateCheatCode("POLITICALIMUNITY");
                SafeSetWantedLevel(pPlayerInfo, 0, true);
            }
            else
            {
                SafeSetWantedLevel(pPlayerInfo, 0, false);
            }
            break;

        case LCSCheat::UnlimitedAmmo:
            s_bUnlimitedAmmo = !s_bUnlimitedAmmo;
            SafePatchUnlimitedAmmo(s_bUnlimitedAmmo);
            if (s_bUnlimitedAmmo)
            {
                SafeRefillPlayerAmmo(pPlayerInfo);
            }
            break;

        case LCSCheat::ClearWantedLevel:
            SimulateCheatCode("POLITICALIMUNITY");
            SafeSetWantedLevel(pPlayerInfo, 0, false);
            break;

        case LCSCheat::RaiseWantedLevel:
            SimulateCheatCode("CROOKEDTONI");
            SafeRaiseWantedLevel(pPlayerInfo, 2);
            break;

        case LCSCheat::UpsideDownMode:
            s_bUpsideDown = true;
            break;

        case LCSCheat::NormalOrientationMode:
            s_bUpsideDown = false;
            break;

        case LCSCheat::HealthArmorMoney250K:
            SimulateCheatCode("NOTEVENRICH");
            SafeAddMoneyAndRestore(pPlayerInfo, 250000);
            break;

        case LCSCheat::FullHealth:
            SimulateCheatCode("GRABEMPILLS");
            SafeSetHealth(pPlayerInfo, 100.0f);
            break;

        case LCSCheat::FullArmor:
            SimulateCheatCode("JUSTKEVLAR");
            SafeSetArmour(pPlayerInfo, 100.0f);
            break;

        case LCSCheat::CommitSuicide:
            SimulateCheatCode("KYSMATE");
            SafeSetHealth(pPlayerInfo, 0.0f);
            break;

        default:
            SimulateCheatCode(info->codeString);
            break;
        }

        s_lastActivatedName = info->name;
        s_notificationAlpha = 1.0f;
        s_notificationTime = std::chrono::steady_clock::now();

        std::string logMsg = "[CheatManager] Activated cheat: " + info->name + " (" + info->category + ")";
        CrashLogger::LogMessage(logMsg);

        return true;
    }

    void CheatManager::Update()
    {
        // Fade notification
        if (s_notificationAlpha > 0.0f)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_notificationTime).count();
            if (elapsed > 2500)
            {
                s_notificationAlpha = (3000 - elapsed) / 500.0f;
                if (s_notificationAlpha < 0.0f)
                    s_notificationAlpha = 0.0f;
            }
        }

        // Maintain Never Wanted if toggled on
        if (s_bNeverWanted)
        {
            SafeSetWantedLevel(0x94AD28, 0, true);
        }

        // Maintain Unlimited Ammo if toggled on
        if (s_bUnlimitedAmmo)
        {
            SafeRefillPlayerAmmo(0x94AD28);
        }
    }

    const std::vector<CheatInfo>& CheatManager::GetCheats()
    {
        return s_cheatList;
    }

    const CheatInfo* CheatManager::GetCheatInfo(LCSCheat cheat)
    {
        for (const auto& item : s_cheatList)
        {
            if (item.id == cheat)
                return &item;
        }
        return nullptr;
    }
}
