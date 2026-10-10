#include "CheatManager.h"
#include "CrashLogger.h"

namespace ReLCS
{
    std::vector<CheatInfo> CheatManager::s_cheatList;
    std::atomic<bool> CheatManager::s_bNeverWanted(false);
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
            // Player Cheats (1-10)
            { LCSCheat::FullHealth, "Full Health", "Player", "Restores player health to 100%", "ASPIRINE" },
            { LCSCheat::FullArmor, "Full Armor", "Player", "Restores body armor to 100%", "PRECIOUSPROTECTION" },
            { LCSCheat::HealthArmorMoney250K, "$250,000 + Health + Armor", "Player", "Gives $250k cash and full restoration", "ASPIRINE" },
            { LCSCheat::WeaponSet1, "Weapon Set 1", "Player", "Knuckles, knife, Molotov, 9mm, shotgun, etc.", "THUGSTOOLS" },
            { LCSCheat::WeaponSet2, "Weapon Set 2", "Player", "Katana, grenades, .357, Uzi, M4, launcher, etc.", "PROFESSIONALTOOLS" },
            { LCSCheat::WeaponSet3, "Weapon Set 3", "Player", "Chainsaw, grenades, .357, spaz, MP5, minigun", "NUTTERTOOLS" },
            { LCSCheat::ClearWantedLevel, "Clear Wanted Level", "Player", "Removes all wanted stars immediately", "LEAVEMEALONE" },
            { LCSCheat::RaiseWantedLevel, "Raise Wanted Level (+2)", "Player", "Increases wanted level by 2 stars", "YOUWONTTAKEMEALIVE" },
            { LCSCheat::NeverWanted, "Never Wanted [Toggle]", "Player", "Keeps wanted level permanently at zero", "LEAVEMEALONE" },
            { LCSCheat::CommitSuicide, "Commit Suicide", "Player", "Instantly eliminates the player character", "ICANTTAKEITANYMORE" },

            // Vehicle Cheats (11-19)
            { LCSCheat::SpawnRhino, "Spawn Rhino Tank", "Vehicle", "Spawns a Rhino military tank near player", "PANZER" },
            { LCSCheat::SpawnTrashmaster, "Spawn Trashmaster", "Vehicle", "Spawns a city garbage truck", "RUBBISHCAR" },
            { LCSCheat::SpawnDeaddodo, "Spawn Deaddodo", "Vehicle", "Spawns an airplane near player", "AIRSHIP" },
            { LCSCheat::DestroyAllCars, "Blow Up All Cars", "Vehicle", "Detonates all vehicles in area", "BIGBANG" },
            { LCSCheat::PerfectHandling, "Perfect Traction & Handling", "Vehicle", "Maximizes vehicle grip and cornering", "GRIPISEVERYTHING" },
            { LCSCheat::AggressiveDrivers, "Aggressive Drivers", "Vehicle", "Drivers ram into other cars aggressively", "MIAMITRAFFIC" },
            { LCSCheat::ChromeCars, "White / Chrome Cars", "Vehicle", "Repaints all city vehicles white/chrome", "AHAIRDRESSERSCAR" },
            { LCSCheat::BlackCars, "Black Cars", "Vehicle", "Repaints all city vehicles black", "IWANTITPAINTEDBLACK" },
            { LCSCheat::CarsDriveOnWater, "Cars Drive On Water", "Vehicle", "Vehicles float and drive across water", "SEAWAYS" },

            // World Cheats (20-31)
            { LCSCheat::SunnyWeather, "Sunny Weather", "World", "Sets weather to clear sunshine", "ALOVELYDAY" },
            { LCSCheat::ClearWeather, "Clear Weather", "World", "Sets weather to pleasant clear skies", "PLEASANTLYWARM" },
            { LCSCheat::OvercastWeather, "Overcast Weather", "World", "Sets weather to overcast clouds", "ABITDRIEG" },
            { LCSCheat::RainyWeather, "Rainy Weather", "World", "Sets weather to heavy rainfall", "CATSANDDOGS" },
            { LCSCheat::FoggyWeather, "Foggy Weather", "World", "Sets weather to thick low-visibility fog", "CANTSEEATHING" },
            { LCSCheat::FasterClock, "Faster Clock", "World", "Accelerates clock and day/night speed", "LIFEISPASSINGMEBY" },
            { LCSCheat::TrafficLightsGreen, "Green Traffic Lights", "World", "Turns all traffic lights green", "GREENLIGHT" },
            { LCSCheat::TrafficLightsRed, "Red Traffic Lights", "World", "Turns all traffic lights red", "REDLIGHT" },
            { LCSCheat::PedRiot, "Pedestrian Riot", "World", "Pedestrians riot and fight in streets", "FIGHTFIGHTFIGHT" },
            { LCSCheat::PedAttackYou, "Pedestrians Attack You", "World", "All pedestrians become hostile to player", "NOBODYLIKESME" },
            { LCSCheat::PedHaveWeapons, "Armed Pedestrians", "World", "Pedestrians carry dangerous firearms", "OURGODGIVENRIGHTTOBEARARMS" },
            { LCSCheat::PedFollowYou, "Pedestrians Follow You", "World", "Nearby pedestrians follow the player", "FANNYMAGNET" },

            // Visual & Gameplay Cheats (32-35)
            { LCSCheat::SlowMotion, "Slow Motion", "Visual/Gameplay", "Slows game speed down", "BOOOOOORING" },
            { LCSCheat::FastMotion, "Fast Motion", "Visual/Gameplay", "Speeds game motion up", "ONSPEED" },
            { LCSCheat::UpsideDownMode, "Upside Down Screen", "Visual/Gameplay", "Flips viewport upside down", "" },
            { LCSCheat::NormalOrientationMode, "Normal Screen Orientation", "Visual/Gameplay", "Restores normal viewport orientation", "" }
        };

        FindGameOffsets();
        CrashLogger::LogMessage("[CheatManager] Initialized with 35 LCS cheats.");
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

    void CheatManager::SimulateCheatCode(const std::string& code)
    {
        if (code.empty())
            return;

        // Directly call the game's native CPad::AddToPCCheatString at 0x4ABD20 with CPad::Pads[0] at 0x7DBCB0
        typedef void(__thiscall* AddToPCCheatString_t)(void* pPad, char c);
        AddToPCCheatString_t pfnAddCheat = reinterpret_cast<AddToPCCheatString_t>(0x4ABD20);
        void* pPad = reinterpret_cast<void*>(0x7DBCB0);

        for (char c : code)
        {
            char upperChar = static_cast<char>(toupper(static_cast<unsigned char>(c)));
            __try
            {
                pfnAddCheat(pPad, upperChar);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                break;
            }
        }
    }

    bool CheatManager::ActivateCheat(LCSCheat cheat)
    {
        const CheatInfo* info = GetCheatInfo(cheat);
        if (!info)
            return false;

        uintptr_t pPlayerInfo = 0x94AD28;

        switch (cheat)
        {
        case LCSCheat::NeverWanted:
            s_bNeverWanted = !s_bNeverWanted;
            if (s_bNeverWanted)
            {
                SimulateCheatCode("LEAVEMEALONE");
                __try
                {
                    uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
                    if (pPed)
                    {
                        uintptr_t pWanted = *reinterpret_cast<uintptr_t*>(pPed + 0x5F4);
                        if (pWanted)
                        {
                            *reinterpret_cast<int*>(pWanted + 0x00) = 0;
                            *reinterpret_cast<int*>(pWanted + 0x20) = 0;
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
            }
            break;

        case LCSCheat::ClearWantedLevel:
            SimulateCheatCode("LEAVEMEALONE");
            __try
            {
                uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
                if (pPed)
                {
                    uintptr_t pWanted = *reinterpret_cast<uintptr_t*>(pPed + 0x5F4);
                    if (pWanted)
                    {
                        *reinterpret_cast<int*>(pWanted + 0x00) = 0;
                        *reinterpret_cast<int*>(pWanted + 0x20) = 0;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            break;

        case LCSCheat::RaiseWantedLevel:
            SimulateCheatCode("YOUWONTTAKEMEALIVE");
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
                            cur += 2;
                            if (cur > 6) cur = 6;
                            *reinterpret_cast<int*>(pWanted + 0x20) = cur;
                            *reinterpret_cast<int*>(pWanted + 0x00) = cur * 150;
                        }
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            break;

        case LCSCheat::UpsideDownMode:
            s_bUpsideDown = true;
            break;

        case LCSCheat::NormalOrientationMode:
            s_bUpsideDown = false;
            break;

        case LCSCheat::HealthArmorMoney250K:
            // Native cheats
            SimulateCheatCode("ASPIRINE");
            SimulateCheatCode("PRECIOUSPROTECTION");

            // If ReLCS.dll is loaded, call its native $250k cheat function
            {
                HMODULE hReLCS = GetModuleHandleA("ReLCS.dll");
                if (hReLCS)
                {
                    typedef void (*ReLCSCheatFn_t)();
                    ReLCSCheatFn_t pfnMoneyCheat = reinterpret_cast<ReLCSCheatFn_t>(reinterpret_cast<uintptr_t>(hReLCS) + 0x25af0);
                    __try { pfnMoneyCheat(); } __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
            }

            // Direct memory money addition ($250,000) & health/armor
            __try
            {
                int* pMoney = reinterpret_cast<int*>(pPlayerInfo + 0xA0);
                int* pDisplayMoney = reinterpret_cast<int*>(pPlayerInfo + 0xA4);
                *pMoney += 250000;
                *pDisplayMoney += 250000;

                uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
                if (pPed)
                {
                    *reinterpret_cast<float*>(pPed + 0x354) = 100.0f; // Health
                    *reinterpret_cast<float*>(pPed + 0x358) = 100.0f; // Armour
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            break;

        case LCSCheat::FullHealth:
            SimulateCheatCode("ASPIRINE");
            __try
            {
                uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
                if (pPed)
                    *reinterpret_cast<float*>(pPed + 0x354) = 100.0f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            break;

        case LCSCheat::FullArmor:
            SimulateCheatCode("PRECIOUSPROTECTION");
            __try
            {
                uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
                if (pPed)
                    *reinterpret_cast<float*>(pPed + 0x358) = 100.0f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            break;

        case LCSCheat::CommitSuicide:
            SimulateCheatCode("ICANTTAKEITANYMORE");
            __try
            {
                uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
                if (pPed)
                    *reinterpret_cast<float*>(pPed + 0x354) = 0.0f;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
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
            __try
            {
                uintptr_t pPlayerInfo = 0x94AD28;
                uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
                if (pPed)
                {
                    uintptr_t pWanted = *reinterpret_cast<uintptr_t*>(pPed + 0x5F4);
                    if (pWanted)
                    {
                        *reinterpret_cast<int*>(pWanted + 0x00) = 0;
                        *reinterpret_cast<int*>(pWanted + 0x20) = 0;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
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
