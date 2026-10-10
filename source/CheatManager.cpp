#include "CheatManager.h"
#include "CrashLogger.h"
#include <cmath>

namespace ReLCS
{
    std::vector<CheatInfo> CheatManager::s_cheatList;
    std::atomic<bool> CheatManager::s_bNeverWanted(false);
    std::atomic<bool> CheatManager::s_bUnlimitedAmmo(false);
    std::atomic<bool> CheatManager::s_bInfiniteHealth(false);
    std::atomic<bool> CheatManager::s_bInfiniteArmor(false);
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
            { LCSCheat::InfiniteHealth, "Infinite Health [Toggle]", "Player", "God mode: immune to damage, health locked at max", "" },
            { LCSCheat::InfiniteArmor, "Infinite Armor [Toggle]", "Player", "Locks body armor permanently at 100%", "" },
            { LCSCheat::HealthArmorMoney250K, "$250,000 + Health + Armor", "Player", "Gives $250k cash and full restoration", "NOTEVENRICH" },
            { LCSCheat::WeaponSet1, "Weapon Set 1 (Street)", "Player", "Knuckles, knife, 9mm, shotgun, Tec-9, Ruger, sniper, Molotov, flame", "TERRORIST" },
            { LCSCheat::WeaponSet2, "Weapon Set 2 (Tactical)", "Player", "Katana, .357 Python, stubby, Uzi, M4, laser sniper, grenade, RPG", "PALETO" },
            { LCSCheat::WeaponSet3, "Weapon Set 3 (Heavy Rampage)", "Player", "Chainsaw, Spas-12, MP5, M60 heavy MG, teargas, minigun", "ICANCU" },
            { LCSCheat::ClearWantedLevel, "Clear Wanted Level", "Player", "Removes all wanted stars immediately", "POLITICALIMUNITY" },
            { LCSCheat::RaiseWantedLevel, "Raise Wanted Level (+2)", "Player", "Increases wanted level by 2 stars", "CROOKEDTONI" },
            { LCSCheat::NeverWanted, "Never Wanted [Toggle]", "Player", "Keeps wanted level permanently at zero", "POLITICALIMUNITY" },
            { LCSCheat::UnlimitedAmmo, "Unlimited Ammo [Toggle]", "Player", "Infinite ammo and no reload for all weapons", "" },
            { LCSCheat::CommitSuicide, "Commit Suicide", "Player", "Instantly eliminates the player character", "KYSMATE" },

            // Vehicle Cheats & Spawners
            { LCSCheat::SpawnInfernus, "Infernus", "Vehicle", "Spawns Infernus supercar", "", 142 },
            { LCSCheat::SpawnCheetah, "Cheetah", "Vehicle", "Spawns Cheetah supercar", "", 146 },
            { LCSCheat::SpawnBanshee, "Banshee", "Vehicle", "Spawns Banshee sports car", "", 160 },
            { LCSCheat::SpawnStinger, "Stinger", "Vehicle", "Spawns Stinger sports car", "", 133 },
            { LCSCheat::SpawnYakuzaStinger, "Yakuza Stinger", "Vehicle", "Spawns Yakuza gang stinger", "", 174 },
            { LCSCheat::SpawnDiabloStallion, "Diablo Stallion", "Vehicle", "Spawns Diablo gang muscle car", "", 175 },
            { LCSCheat::SpawnMafiaSentinel, "Mafia Sentinel", "Vehicle", "Spawns Leone Mafia sentinel", "", 172 },
            { LCSCheat::SpawnHellenbachGT, "Hellenbach GT", "Vehicle", "Spawns rare Hellenbach GT coupe", "", 185 },
            { LCSCheat::SpawnPhobosVT, "Phobos VT", "Vehicle", "Spawns Phobos VT muscle car", "", 186 },
            { LCSCheat::SpawnDeimosSP, "Deimos SP", "Vehicle", "Spawns Deimos SP sports car", "", 187 },
            { LCSCheat::SpawnThunderRodd, "Thunder-Rodd", "Vehicle", "Spawns Thunder-Rodd custom hotrod", "", 189 },
            { LCSCheat::SpawnPCJ600, "PCJ-600", "Vehicle", "Spawns PCJ-600 sport motorcycle", "", 205 },
            { LCSCheat::SpawnFreeway, "Freeway", "Vehicle", "Spawns Freeway chopper bike", "", 207 },
            { LCSCheat::SpawnSanchez, "Sanchez", "Vehicle", "Spawns Sanchez dirt bike", "", 210 },
            { LCSCheat::SpawnFaggio, "Faggio", "Vehicle", "Spawns Faggio scooter moped", "", 206 },
            { LCSCheat::SpawnPizzaBoy, "Pizza Boy", "Vehicle", "Spawns Pizza Boy delivery scooter", "", 203 },
            { LCSCheat::SpawnAngel, "Angel", "Vehicle", "Spawns Angel gang motorcycle", "", 202 },
            { LCSCheat::SpawnRhino, "Rhino Tank", "Vehicle", "Spawns Rhino military tank", "TOOLOFPEACE", 162 },
            { LCSCheat::SpawnBarracks, "Barracks OL", "Vehicle", "Spawns military transport truck", "", 163 },
            { LCSCheat::SpawnPatriot, "Patriot", "Vehicle", "Spawns military 4x4 Patriot SUV", "", 137 },
            { LCSCheat::SpawnPolice, "Police Cruiser", "Vehicle", "Spawns LCPD police cruiser", "", 157 },
            { LCSCheat::SpawnEnforcer, "Enforcer SWAT", "Vehicle", "Spawns SWAT armored enforcer", "", 158 },
            { LCSCheat::SpawnFiretruck, "Firetruck", "Vehicle", "Spawns municipal fire engine", "", 138 },
            { LCSCheat::SpawnAmbulance, "Ambulance", "Vehicle", "Spawns hospital ambulance", "", 147 },
            { LCSCheat::SpawnStretch, "Stretch Limousine", "Vehicle", "Spawns luxury stretch limo", "", 140 },
            { LCSCheat::SpawnTrashmaster, "Trashmaster", "Vehicle", "Spawns city garbage truck", "UNTOUCHABLE", 139 },
            { LCSCheat::SpawnBFInjection, "BF Injection", "Vehicle", "Spawns BF Injection offroad buggy", "", 155 },
            { LCSCheat::SpawnLandstalker, "Landstalker", "Vehicle", "Spawns Landstalker luxury SUV", "", 131 },
            { LCSCheat::SpawnBorgnine, "Borgnine Cabbie", "Vehicle", "Spawns Borgnine special taxi", "", 181 },
            { LCSCheat::SpawnDeaddodo, "Deaddodo Plane", "Vehicle", "Spawns airplane near player", "AIRSHIP", 201 },
            { LCSCheat::SpawnHunter, "Hunter", "Vehicle", "Spawns Hunter military attack helicopter", "", 213 },
            { LCSCheat::SpawnMaverick, "Maverick", "Vehicle", "Spawns Maverick civilian helicopter", "", 214 },
            { LCSCheat::CarsDriveOnWater, "Cars Drive On Water", "Vehicle", "Vehicles float and drive across water", "JESUSCAR" },
            { LCSCheat::DestroyAllCars, "Blow Up All Cars", "Vehicle", "Detonates all vehicles in area", "ALTRIGHT" },
            { LCSCheat::PerfectHandling, "Perfect Traction & Handling", "Vehicle", "Maximizes vehicle grip and cornering", "BETTERBIKES" },
            { LCSCheat::AggressiveDrivers, "Aggressive Drivers", "Vehicle", "Drivers ram into other cars aggressively", "MAXMAD" },
            { LCSCheat::ChromeCars, "White / Chrome Cars", "Vehicle", "Repaints all city vehicles white/chrome", "SOUTHTRAFFIC" },
            { LCSCheat::BlackCars, "Black Cars", "Vehicle", "Repaints all city vehicles black", "BCARSMATTER" },

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
        CrashLogger::LogMessage("[CheatManager] Initialized with Re:LCS PC cheats & car spawner.");
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

    static void SafeSpawnVehicle(int modelId)
    {
        __try
        {
            const uintptr_t pPlayerInfo = 0x94AD28;
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (!pPed) return;

            // 1. Request and load vehicle model synchronously via CStreaming
            typedef void(__cdecl* RequestModelFn_t)(int, int);
            typedef void(__cdecl* LoadAllRequestedModelsFn_t)(bool);
            typedef void(__cdecl* SetModelIsDeletableFn_t)(int);

            RequestModelFn_t pfnRequestModel = reinterpret_cast<RequestModelFn_t>(0x40E310);
            LoadAllRequestedModelsFn_t pfnLoadModels = reinterpret_cast<LoadAllRequestedModelsFn_t>(0x40B5F0);
            SetModelIsDeletableFn_t pfnSetDeletable = reinterpret_cast<SetModelIsDeletableFn_t>(0x40AEA0);

            pfnRequestModel(modelId, 1);
            pfnLoadModels(false);

            // 2. Read player position and forward facing direction from CMatrix at pPed + 0x04
            float px = *reinterpret_cast<float*>(pPed + 0x34);
            float py = *reinterpret_cast<float*>(pPed + 0x38);
            float pz = *reinterpret_cast<float*>(pPed + 0x3C);

            float fx = *reinterpret_cast<float*>(pPed + 0x14);
            float fy = *reinterpret_cast<float*>(pPed + 0x18);

            float len = sqrtf(fx * fx + fy * fy);
            float dir_x = (len > 0.001f) ? (fx / len) : 0.0f;
            float dir_y = (len > 0.001f) ? (fy / len) : 1.0f;

            // Heading angle in radians: in GTA coordinate system (forward.x = -sin(heading), forward.y = cos(heading))
            float heading = atan2f(-dir_x, dir_y);
            // Rotate vehicle 90 degrees sideways so driver seat (left door) faces directly towards Toni!
            float veh_heading = heading + 1.57079632679f;

            // 3. Determine vehicle class and exact allocation size
            // Bikes in LCS: 202 (Angel) through 210 (Sanchez)
            bool isBike = (modelId >= 202 && modelId <= 210);
            // Boats in LCS: 192 (Ferry) through 196 (Predator)
            bool isBoat = (modelId >= 192 && modelId <= 196);

            size_t allocSize = 0;
            int vehType = 2; // 0: Boat, 1: Bike, 2: Automobile
            if (isBoat)
            {
                allocSize = 0x4C0; // CBoat
                vehType = 0;
            }
            else if (isBike)
            {
                allocSize = 0x4EC; // CBike
                vehType = 1;
            }
            else
            {
                allocSize = 0x5DC; // CAutomobile
                vehType = 2;
            }

            // Calculate distance in front of Toni
            uintptr_t inVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3A8);
            float dist = 5.0f;
            if (inVeh)
                dist = 8.5f;
            else if (modelId == 162 || modelId == 139 || modelId == 163 || modelId == 161 || modelId == 165 || modelId == 213)
                dist = 8.5f; // Large vehicles (Rhino, Trashmaster, Barracks, Bus, Coach, Hunter)
            else if (isBoat)
                dist = 10.0f;
            else if (isBike)
                dist = 3.5f;

            float spawn_x = px + dir_x * dist;
            float spawn_y = py + dir_y * dist;

            // 4. Query exact ground elevation via CWorld::FindGroundZForCoord (0x4D5540)
            typedef float(__cdecl* FindGroundZFn_t)(float, float);
            FindGroundZFn_t pfnFindGroundZ = reinterpret_cast<FindGroundZFn_t>(0x4D5540);
            float ground_z = pfnFindGroundZ(spawn_x, spawn_y);

            float spawn_z = pz;
            if (ground_z > -50.0f && fabsf(ground_z - pz) < 12.0f)
                spawn_z = ground_z;

            float final_z = spawn_z + (isBike ? 0.25f : (isBoat ? 0.0f : 0.45f));

            // 5. Allocate vehicle memory using game operator new (0x5BAB20)
            typedef void*(__cdecl* OperatorNewFn_t)(size_t);
            OperatorNewFn_t pfnNew = reinterpret_cast<OperatorNewFn_t>(0x5BAB20);
            void* pVeh = pfnNew(allocSize);
            if (!pVeh)
                return;

            uintptr_t veh = reinterpret_cast<uintptr_t>(pVeh);

            // 6. Invoke proper C++ constructor based on vehicle type
            if (vehType == 0)
            {
                // CBoat::CBoat(veh, modelId, 2) at 0x5A6470
                typedef void(__thiscall* CBoatCtor_t)(void*, int, unsigned char);
                reinterpret_cast<CBoatCtor_t>(0x5A6470)(pVeh, modelId, 2);
            }
            else if (vehType == 1)
            {
                // CBike::CBike(veh, modelId, 2) at 0x615740
                typedef void(__thiscall* CBikeCtor_t)(void*, int, unsigned char);
                reinterpret_cast<CBikeCtor_t>(0x615740)(pVeh, modelId, 2);
            }
            else
            {
                // CAutomobile::CAutomobile(veh, modelId, 2) at 0x59E620
                typedef void(__thiscall* CAutoCtor_t)(void*, int, unsigned char);
                reinterpret_cast<CAutoCtor_t>(0x59E620)(pVeh, modelId, 2);
            }

            // 7. Set rotation sideways facing driver door using CMatrix::SetRotate at 0x4DF240
            typedef void(__thiscall* SetRotateFn_t)(void*, float, float, float);
            SetRotateFn_t pfnSetRotate = reinterpret_cast<SetRotateFn_t>(0x4DF240);
            pfnSetRotate(reinterpret_cast<void*>(veh + 0x04), 0.0f, 0.0f, veh_heading);

            // Set vehicle coordinates in CPlaceable matrix at veh + 0x34
            *reinterpret_cast<float*>(veh + 0x34) = spawn_x;
            *reinterpret_cast<float*>(veh + 0x38) = spawn_y;
            *reinterpret_cast<float*>(veh + 0x3C) = final_z;

            // 8. Configure status flags and unlocked doors
            uint8_t* pStatus = reinterpret_cast<uint8_t*>(veh + 0x52);
            *pStatus = (*pStatus & ~0x38) | (4 << 3); // STATUS_PLAYER

            if (vehType == 2)
            {
                *reinterpret_cast<uint32_t*>(veh + 0x230) = 1; // Unlocked doors
            }
            else if (vehType == 0)
            {
                *reinterpret_cast<float*>(veh + 0x15C) = 20.0f; // Boat water buoyancy
                *reinterpret_cast<uint8_t*>(veh + 0x160) = 20;
            }

            // 9. Add vehicle entity to CWorld at 0x4DB3F0
            typedef void(__cdecl* WorldAddFn_t)(void*);
            reinterpret_cast<WorldAddFn_t>(0x4DB3F0)(pVeh);

            // 10. Mark model as deletable so CStreaming cache is managed normally
            pfnSetDeletable(modelId);
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
            {
                *reinterpret_cast<float*>(pPed + 0x354) = health;

                // Also maintain car health if player is driving a vehicle
                uintptr_t pVehicle = *reinterpret_cast<uintptr_t*>(pPed + 0x3A8);
                if (!pVehicle)
                    pVehicle = *reinterpret_cast<uintptr_t*>(pPed + 0x3AC);
                if (pVehicle)
                {
                    *reinterpret_cast<float*>(pVehicle + 0x204) = 1000.0f;
                }
            }
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

    static void SafeClearWeapons(uintptr_t pPed)
    {
        __try
        {
            if (!pPed) return;
            for (int i = 0; i < 10; i++)
            {
                uintptr_t pWp = pPed + 0x408 + (i * 0x18);
                *reinterpret_cast<int*>(pWp + 0x00) = 0; // WEAPONTYPE_UNARMED
                *reinterpret_cast<int*>(pWp + 0x04) = 0; // WEAPONSTATE_READY
                *reinterpret_cast<int*>(pWp + 0x08) = 0; // m_nAmmoInClip
                *reinterpret_cast<int*>(pWp + 0x0C) = 0; // m_nAmmoTotal
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeGiveWeapon(uintptr_t pPed, int weaponType, int ammo)
    {
        __try
        {
            if (!pPed || weaponType <= 0) return;
            typedef void(__thiscall* GiveWeaponFn_t)(void*, int, unsigned int, bool);
            GiveWeaponFn_t pfnGiveWeapon = reinterpret_cast<GiveWeaponFn_t>(0x4FFA30);
            pfnGiveWeapon(reinterpret_cast<void*>(pPed), weaponType, static_cast<unsigned int>(ammo), true);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeGiveWeaponSet(uintptr_t pPlayerInfo, int set)
    {
        __try
        {
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (!pPed) return;

            // Clear all slots first to eliminate overlap from other sets
            SafeClearWeapons(pPed);

            if (set == 1)
            {
                // Set 1: Thug / Street Arsenal
                SafeGiveWeapon(pPed, 1, 1);      // Brass Knuckles
                SafeGiveWeapon(pPed, 5, 1);      // Knife
                SafeGiveWeapon(pPed, 17, 250);   // Pistol (.45)
                SafeGiveWeapon(pPed, 19, 100);   // Chrome Shotgun
                SafeGiveWeapon(pPed, 22, 500);   // Tec-9
                SafeGiveWeapon(pPed, 27, 500);   // Ruger
                SafeGiveWeapon(pPed, 28, 50);    // Sniper Rifle
                SafeGiveWeapon(pPed, 15, 25);    // Molotov Cocktails
                SafeGiveWeapon(pPed, 31, 500);   // Flamethrower
            }
            else if (set == 2)
            {
                // Set 2: Tactical / Professional Arsenal
                SafeGiveWeapon(pPed, 10, 1);     // Katana
                SafeGiveWeapon(pPed, 18, 150);   // .357 Python
                SafeGiveWeapon(pPed, 21, 100);   // Stubby Shotgun
                SafeGiveWeapon(pPed, 24, 500);   // Silenced Ingram (Mac-10)
                SafeGiveWeapon(pPed, 26, 500);   // M4
                SafeGiveWeapon(pPed, 29, 50);    // Laser Scope Sniper (PSG-1)
                SafeGiveWeapon(pPed, 12, 25);    // Grenades
                SafeGiveWeapon(pPed, 30, 25);    // Rocket Launcher
            }
            else if (set == 3)
            {
                // Set 3: Heavy Rampage Arsenal (100% distinct weapons)
                SafeGiveWeapon(pPed, 11, 1);     // Chainsaw
                SafeGiveWeapon(pPed, 20, 100);   // Spas-12 Shotgun
                SafeGiveWeapon(pPed, 25, 500);   // MP5 SMG
                SafeGiveWeapon(pPed, 32, 500);   // M60 Heavy Machinegun
                SafeGiveWeapon(pPed, 14, 25);    // Teargas
                SafeGiveWeapon(pPed, 33, 500);   // Minigun
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

        case LCSCheat::InfiniteHealth:
            s_bInfiniteHealth = !s_bInfiniteHealth;
            if (s_bInfiniteHealth)
            {
                SafeSetHealth(pPlayerInfo, 250.0f);
            }
            break;

        case LCSCheat::InfiniteArmor:
            s_bInfiniteArmor = !s_bInfiniteArmor;
            if (s_bInfiniteArmor)
            {
                SafeSetArmour(pPlayerInfo, 100.0f);
            }
            break;

        case LCSCheat::WeaponSet1:
            SimulateCheatCode("TERRORIST");
            SafeGiveWeaponSet(pPlayerInfo, 1);
            break;

        case LCSCheat::WeaponSet2:
            SimulateCheatCode("PALETO");
            SafeGiveWeaponSet(pPlayerInfo, 2);
            break;

        case LCSCheat::WeaponSet3:
            SimulateCheatCode("ICANCU");
            SafeGiveWeaponSet(pPlayerInfo, 3);
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
            // Check if vehicle spawner
            if (info->vehicleModelId > 0)
            {
                SafeSpawnVehicle(info->vehicleModelId);
            }
            else if (!info->codeString.empty())
            {
                SimulateCheatCode(info->codeString);
            }
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

        // Maintain Infinite Health if toggled on
        if (s_bInfiniteHealth)
        {
            SafeSetHealth(0x94AD28, 250.0f);
        }

        // Maintain Infinite Armor if toggled on
        if (s_bInfiniteArmor)
        {
            SafeSetArmour(0x94AD28, 100.0f);
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
