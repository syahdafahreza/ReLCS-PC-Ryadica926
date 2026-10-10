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

    static void InstallVehicleSpawnHook();

    void CheatManager::Init()
    {
        InstallVehicleSpawnHook();
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

    static void* __cdecl Hook_CreateVehicleForCheat(int modelId, int createdBy)
    {
        typedef void*(__cdecl* NewFn_t)(size_t);
        NewFn_t pfnNew = reinterpret_cast<NewFn_t>(0x5BAB20);

        uintptr_t* pModelInfoPtrs = reinterpret_cast<uintptr_t*>(0x92D4C8);
        uintptr_t pModelInfo = (pModelInfoPtrs && modelId >= 0 && modelId < 300) ? pModelInfoPtrs[modelId] : 0;
        int vehType = pModelInfo ? *reinterpret_cast<int*>(pModelInfo + 0x3C) : -1;

        bool isBike = (vehType == 5) || (modelId >= 202 && modelId <= 210) || (modelId == 225);
        bool isBoat = (vehType == 1) || (modelId >= 192 && modelId <= 196);

        if (isBike)
        {
            void* pVeh = pfnNew(0x4EC);
            if (pVeh)
            {
                typedef void(__thiscall* BikeCtor_t)(void*, int, unsigned char);
                reinterpret_cast<BikeCtor_t>(0x615740)(pVeh, modelId, static_cast<unsigned char>(createdBy));
            }
            return pVeh;
        }
        else if (isBoat)
        {
            void* pVeh = pfnNew(0x4C0);
            if (pVeh)
            {
                typedef void(__thiscall* BoatCtor_t)(void*, int, unsigned char);
                reinterpret_cast<BoatCtor_t>(0x5A6470)(pVeh, modelId, static_cast<unsigned char>(createdBy));
            }
            return pVeh;
        }
        else
        {
            void* pVeh = pfnNew(0x5DC);
            if (pVeh)
            {
                typedef void(__thiscall* AutoCtor_t)(void*, int, unsigned char);
                reinterpret_cast<AutoCtor_t>(0x59E620)(pVeh, modelId, static_cast<unsigned char>(createdBy));
            }
            return pVeh;
        }
    }

    static void __cdecl Hook_PositionAndAddVehicle(void* pVeh, int modelId)
    {
        if (!pVeh) return;

        __try
        {
            const uintptr_t pPlayerInfo = 0x94AD28;
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (!pPed)
            {
                reinterpret_cast<void(__cdecl*)(void*)>(0x4DB3F0)(pVeh);
                return;
            }

            // 1. Read player position from CPlaceable at ped + 0x34..0x3C
            float px = *reinterpret_cast<float*>(pPed + 0x34);
            float py = *reinterpret_cast<float*>(pPed + 0x38);
            float pz = *reinterpret_cast<float*>(pPed + 0x3C);

            // Forward facing vector from ped matrix at ped + 0x14..0x18
            float fx = *reinterpret_cast<float*>(pPed + 0x14);
            float fy = *reinterpret_cast<float*>(pPed + 0x18);

            // Normalize 2D forward vector
            float len = sqrtf(fx * fx + fy * fy);
            float dir_x = (len > 0.001f) ? (fx / len) : 0.0f;
            float dir_y = (len > 0.001f) ? (fy / len) : 1.0f;

            // 2. Determine distance based on player state & vehicle class (matching cleo-ios)
            uintptr_t inVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3A8);
            bool isBoat = (modelId >= 192 && modelId <= 196);
            bool isBike = (modelId >= 202 && modelId <= 210) || (modelId == 225);

            float dist = 5.0f;
            if (inVeh)
            {
                dist = 8.5f;
            }
            else if (modelId == 162 || modelId == 139 || modelId == 163 || modelId == 161 || modelId == 165 || modelId == 213)
            {
                dist = 8.5f; // Large vehicles (Rhino, Trashmaster, Barracks, Bus, Coach, Hunter)
            }
            else if (isBoat)
            {
                dist = 10.0f;
            }
            else if (isBike)
            {
                dist = 3.5f;
            }

            float spawn_x = px + dir_x * dist;
            float spawn_y = py + dir_y * dist;

            // 3. Query ground elevation via CWorld::FindGroundZForCoord (0x4D5540)
            typedef float(__cdecl* FindGroundZFn_t)(float, float);
            float ground_z = reinterpret_cast<FindGroundZFn_t>(0x4D5540)(spawn_x, spawn_y);

            float base_z = pz;
            if (ground_z > -50.0f && fabsf(ground_z - pz) < 10.0f)
            {
                base_z = ground_z;
            }

            // 4. Calculate heading angle in radians: in GTA coordinate system (forward.x = -sin(heading), forward.y = cos(heading))
            float heading = atan2f(-dir_x, dir_y);
            // Rotate vehicle 90 degrees sideways so player stands directly in front of the driver seat (left door)
            // Exactly matches CLEO-iOS Vice City / III implementation: heading + FRAC_PI_2
            float veh_heading = heading + 1.57079632679f;

            // 5. Apply rotation to vehicle placeable matrix via CMatrix::SetRotate at 0x4DF240
            typedef void(__thiscall* SetRotateFn_t)(void*, float, float, float);
            reinterpret_cast<SetRotateFn_t>(0x4DF240)(
                reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pVeh) + 0x04),
                0.0f,
                0.0f,
                veh_heading
            );

            // 6. Set vehicle coordinates into placeable matrix translation vector (veh + 0x34..0x3C)
            float spawn_z = base_z + (isBike ? 0.35f : (isBoat ? 0.0f : 0.6f));
            *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pVeh) + 0x34) = spawn_x;
            *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pVeh) + 0x38) = spawn_y;
            *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pVeh) + 0x3C) = spawn_z;

            // 7. Synchronize RenderWare frame hierarchy (CEntity::UpdateRwFrame at 0x4896C0)
            typedef void(__thiscall* UpdateRwFrameFn_t)(void*);
            reinterpret_cast<UpdateRwFrameFn_t>(0x4896C0)(pVeh);

            // 8. Configure status flags (STATUS_PLAYER = 0x20) and unlocked doors
            uint8_t* pStatus = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pVeh) + 0x50);
            *pStatus = (*pStatus & 7) | 0x20;

            *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pVeh) + 0x230) = 1; // Door lock = CARLOCK_UNLOCKED

            // 9. Add vehicle entity to CWorld at 0x4DB3F0
            typedef void(__cdecl* WorldAddFn_t)(void*);
            reinterpret_cast<WorldAddFn_t>(0x4DB3F0)(pVeh);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            __try
            {
                reinterpret_cast<void(__cdecl*)(void*)>(0x4DB3F0)(pVeh);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
    }

    static void InstallVehicleSpawnHook()
    {
        static bool s_installed = false;
        if (s_installed) return;

        DWORD oldProtect = 0;

        // Hook 1: NOP the jl 0x4aeaa2 check at 0x4AE9AC (6 bytes: 0F 8C F0 00 00 00)
        // This ensures the spawner never fails if parking space nodes are not found nearby.
        void* pCheckAddr = reinterpret_cast<void*>(0x4AE9AC);
        if (VirtualProtect(pCheckAddr, 6, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            memset(pCheckAddr, 0x90, 6);
            VirtualProtect(pCheckAddr, 6, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pCheckAddr, 6);
        }

        // Hook 2: Vehicle constructor hook at 0x4AE9B2 (29 bytes up to 0x4AE9CF)
        // Allocates CBike for motorcycles, CBoat for boats, and CAutomobile for cars.
        void* pCtorAddr = reinterpret_cast<void*>(0x4AE9B2);
        const size_t ctorPatchSize = 29;
        if (VirtualProtect(pCtorAddr, ctorPatchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(pCtorAddr);

            // 6A 01          - push 1
            p[0] = 0x6A;
            p[1] = 0x01;
            // 56             - push esi
            p[2] = 0x56;
            // E8 xx xx xx xx - call Hook_CreateVehicleForCheat
            p[3] = 0xE8;
            uintptr_t callNext = 0x4AE9B2 + 3 + 5;
            int32_t relOffset = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_CreateVehicleForCheat) - callNext);
            *reinterpret_cast<int32_t*>(&p[4]) = relOffset;
            // 83 C4 08       - add esp, 8
            p[8] = 0x83;
            p[9] = 0xC4;
            p[10] = 0x08;
            // 89 C3          - mov ebx, eax
            p[11] = 0x89;
            p[12] = 0xC3;

            // Fill remaining bytes up to 0x4AE9CF with NOPs (0x90)
            for (size_t i = 13; i < ctorPatchSize; ++i)
            {
                p[i] = 0x90;
            }

            VirtualProtect(pCtorAddr, ctorPatchSize, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pCtorAddr, ctorPatchSize);
        }

        // Hook 3: Position & Orientation hook at 0x4AEA4F (83 bytes up to 0x4AEAA2)
        // Repositions the vehicle directly in front of the player, rotated so the driver seat faces Toni!
        void* pPosAddr = reinterpret_cast<void*>(0x4AEA4F);
        const size_t posPatchSize = 83;
        if (VirtualProtect(pPosAddr, posPatchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(pPosAddr);

            // 56             - push esi (modelId)
            p[0] = 0x56;
            // 53             - push ebx (pVeh)
            p[1] = 0x53;
            // E8 xx xx xx xx - call Hook_PositionAndAddVehicle
            p[2] = 0xE8;
            uintptr_t callNext = 0x4AEA4F + 2 + 5;
            int32_t relCall = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_PositionAndAddVehicle) - callNext);
            *reinterpret_cast<int32_t*>(&p[3]) = relCall;
            // 83 C4 08       - add esp, 8
            p[7] = 0x83;
            p[8] = 0xC4;
            p[9] = 0x08;
            // E9 xx xx xx xx - jmp 0x4AEAA2
            p[10] = 0xE9;
            uintptr_t jmpNext = 0x4AEA4F + 10 + 5;
            int32_t relJmp = static_cast<int32_t>(0x4AEAA2 - jmpNext);
            *reinterpret_cast<int32_t*>(&p[11]) = relJmp;

            // Fill remaining bytes up to 0x4AEAA2 with NOPs (0x90)
            for (size_t i = 15; i < posPatchSize; ++i)
            {
                p[i] = 0x90;
            }

            VirtualProtect(pPosAddr, posPatchSize, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pPosAddr, posPatchSize);
        }

        s_installed = true;
        CrashLogger::LogMessage("[CheatManager] Installed vehicle constructor & driver-facing position hooks (CLEO-iOS style).");
    }

    static void SafeSpawnVehicle(int modelId)
    {
        InstallVehicleSpawnHook();
        __try
        {
            typedef void(__cdecl* SpawnVehicleFn_t)(int);
            SpawnVehicleFn_t pfnSpawn = reinterpret_cast<SpawnVehicleFn_t>(0x4AE8F0);
            pfnSpawn(modelId);
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
