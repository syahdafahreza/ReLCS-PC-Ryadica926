#pragma once

#include "pch.h"

namespace ReLCS
{
    enum class LCSCheat : uint32_t
    {
        // Player Cheats (1-10)
        FullHealth = 1,
        FullArmor,
        HealthArmorMoney250K,
        WeaponSet1,
        WeaponSet2,
        WeaponSet3,
        ClearWantedLevel,
        RaiseWantedLevel,
        NeverWanted,
        UnlimitedAmmo,
        CommitSuicide,

        // Vehicle Cheats (11-19)
        SpawnRhino,
        SpawnTrashmaster,
        SpawnDeaddodo,
        DestroyAllCars,
        PerfectHandling,
        AggressiveDrivers,
        ChromeCars,
        BlackCars,
        CarsDriveOnWater,

        // World Cheats (20-31)
        SunnyWeather,
        ClearWeather,
        OvercastWeather,
        RainyWeather,
        FoggyWeather,
        FasterClock,
        TrafficLightsGreen,
        TrafficLightsRed,
        PedRiot,
        PedAttackYou,
        PedHaveWeapons,
        PedFollowYou,

        // Visual & Gameplay Cheats (32-35)
        SlowMotion,
        FastMotion,
        UpsideDownMode,
        NormalOrientationMode,

        COUNT = 35
    };

    struct CheatInfo
    {
        LCSCheat id;
        std::string name;
        std::string category;
        std::string description;
        std::string codeString;
    };

    class CheatManager
    {
    public:
        static void Init();
        static void Update();
        static bool ActivateCheat(LCSCheat cheat);
        static const std::vector<CheatInfo>& GetCheats();
        static const CheatInfo* GetCheatInfo(LCSCheat cheat);

        // State toggles
        static bool IsNeverWantedEnabled() { return s_bNeverWanted; }
        static bool IsUnlimitedAmmoEnabled() { return s_bUnlimitedAmmo; }
        static bool IsUpsideDownEnabled() { return s_bUpsideDown; }
        static const std::string& GetLastActivatedCheatName() { return s_lastActivatedName; }
        static float GetNotificationAlpha() { return s_notificationAlpha; }

    private:
        static void SimulateCheatCode(const std::string& code);
        static void FindGameOffsets();

        static std::vector<CheatInfo> s_cheatList;
        static std::atomic<bool> s_bNeverWanted;
        static std::atomic<bool> s_bUnlimitedAmmo;
        static std::atomic<bool> s_bUpsideDown;
        static std::string s_lastActivatedName;
        static float s_notificationAlpha;
        static std::chrono::steady_clock::time_point s_notificationTime;

        // Game pointers/offsets discovered at runtime
        static uintptr_t s_pTimeScale;
        static uintptr_t s_pWeather;
        static uintptr_t s_pScriptSpace;
        static HWND s_hGameWnd;
    };
}
