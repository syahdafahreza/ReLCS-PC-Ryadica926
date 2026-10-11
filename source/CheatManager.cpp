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
    std::atomic<bool> CheatManager::s_bVehicleGodMode(false);
    std::atomic<bool> CheatManager::s_bUpsideDown(false);
    std::string CheatManager::s_lastActivatedName = "";
    std::string CheatManager::s_lastActivatedCategory = "";
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

            // Weapon Arsenal & Selector
            { LCSCheat::ClearWeapons, "Clear All Weapons", "Weapon", "Removes all weapons from inventory", "" },
            { LCSCheat::WeaponSet1, "Weapon Set 1 (Street)", "Weapon", "Knuckles, knife, 9mm, shotgun, Tec-9, Ruger, sniper, Molotov, flame", "TERRORIST" },
            { LCSCheat::WeaponSet2, "Weapon Set 2 (Tactical)", "Weapon", "Katana, .357 Python, stubby, Uzi, M4, laser sniper, grenade, RPG", "PALETO" },
            { LCSCheat::WeaponSet3, "Weapon Set 3 (Heavy Rampage)", "Weapon", "Chainsaw, Spas-12, MP5, M60 heavy MG, teargas, minigun", "ICANCU" },

            // Melee (Slot 0 & 1)
            { static_cast<LCSCheat>(2000 + 1), "Brass Knuckles", "Weapon", "Melee weapon (Slot 0)", "", 0, 1 },
            { static_cast<LCSCheat>(2000 + 2), "Screwdriver", "Weapon", "Melee weapon (Slot 1)", "", 0, 2 },
            { static_cast<LCSCheat>(2000 + 3), "Golf Club", "Weapon", "Melee weapon (Slot 1)", "", 0, 3 },
            { static_cast<LCSCheat>(2000 + 4), "Nightstick", "Weapon", "Melee weapon (Slot 1)", "", 0, 4 },
            { static_cast<LCSCheat>(2000 + 5), "Knife", "Weapon", "Melee weapon (Slot 1)", "", 0, 5 },
            { static_cast<LCSCheat>(2000 + 6), "Baseball Bat", "Weapon", "Melee weapon (Slot 1)", "", 0, 6 },
            { static_cast<LCSCheat>(2000 + 7), "Hammer", "Weapon", "Melee weapon (Slot 1)", "", 0, 7 },
            { static_cast<LCSCheat>(2000 + 8), "Meat Cleaver", "Weapon", "Melee weapon (Slot 1)", "", 0, 8 },
            { static_cast<LCSCheat>(2000 + 9), "Machete", "Weapon", "Melee weapon (Slot 1)", "", 0, 9 },
            { static_cast<LCSCheat>(2000 + 10), "Katana", "Weapon", "Melee weapon (Slot 1)", "", 0, 10 },
            { static_cast<LCSCheat>(2000 + 11), "Chainsaw", "Weapon", "Melee weapon (Slot 1)", "", 0, 11 },

            // Handguns (Slot 3)
            { static_cast<LCSCheat>(2000 + 17), "Pistol (Colt .45)", "Weapon", "Handgun with 99999 ammo (Slot 3)", "", 0, 17 },
            { static_cast<LCSCheat>(2000 + 18), ".357 Python", "Weapon", "High-caliber revolver with 99999 ammo (Slot 3)", "", 0, 18 },

            // Shotguns (Slot 4)
            { static_cast<LCSCheat>(2000 + 19), "Chrome Shotgun", "Weapon", "Pump shotgun with 99999 ammo (Slot 4)", "", 0, 19 },
            { static_cast<LCSCheat>(2000 + 20), "Spas-12 Shotgun", "Weapon", "Combat shotgun with 99999 ammo (Slot 4)", "", 0, 20 },
            { static_cast<LCSCheat>(2000 + 21), "Stubby Shotgun", "Weapon", "Sawed-off shotgun with 99999 ammo (Slot 4)", "", 0, 21 },

            // Submachine Guns & Rifles (Slot 5 & 6)
            { static_cast<LCSCheat>(2000 + 22), "Tec-9", "Weapon", "Submachine gun with 99999 ammo (Slot 5)", "", 0, 22 },
            { static_cast<LCSCheat>(2000 + 23), "Micro-SMG (Uzi)", "Weapon", "Submachine gun with 99999 ammo (Slot 5)", "", 0, 23 },
            { static_cast<LCSCheat>(2000 + 24), "Silenced Mac-10", "Weapon", "Silenced SMG with 99999 ammo (Slot 5)", "", 0, 24 },
            { static_cast<LCSCheat>(2000 + 25), "MP5", "Weapon", "Submachine gun with 99999 ammo (Slot 5)", "", 0, 25 },
            { static_cast<LCSCheat>(2000 + 26), "M4 Assault Rifle", "Weapon", "Assault rifle with 99999 ammo (Slot 6)", "", 0, 26 },
            { static_cast<LCSCheat>(2000 + 27), "Ruger (AK-47)", "Weapon", "Assault rifle with 99999 ammo (Slot 6)", "", 0, 27 },

            // Heavy Weapons (Slot 7)
            { static_cast<LCSCheat>(2000 + 30), "Rocket Launcher", "Weapon", "RPG with 99999 rockets (Slot 7)", "", 0, 30 },
            { static_cast<LCSCheat>(2000 + 31), "Flamethrower", "Weapon", "Incendiary weapon with 99999 fuel (Slot 7)", "", 0, 31 },
            { static_cast<LCSCheat>(2000 + 32), "M60 Machinegun", "Weapon", "Heavy machinegun with 99999 ammo (Slot 7)", "", 0, 32 },
            { static_cast<LCSCheat>(2000 + 33), "Minigun", "Weapon", "Rotary minigun with 99999 ammo (Slot 7)", "", 0, 33 },

            // Snipers (Slot 8)
            { static_cast<LCSCheat>(2000 + 28), "Sniper Rifle", "Weapon", "Bolt-action sniper with 99999 ammo (Slot 8)", "", 0, 28 },
            { static_cast<LCSCheat>(2000 + 29), "Laser Scope Sniper", "Weapon", "Semi-auto laser sniper with 99999 ammo (Slot 8)", "", 0, 29 },

            // Projectiles & Special (Slot 2 & 9)
            { static_cast<LCSCheat>(2000 + 12), "Grenade", "Weapon", "Thrown explosive with 99999 count (Slot 2)", "", 0, 12 },
            { static_cast<LCSCheat>(2000 + 13), "Remote Grenade", "Weapon", "Detonated explosive with 99999 count (Slot 2)", "", 0, 13 },
            { static_cast<LCSCheat>(2000 + 14), "Tear Gas", "Weapon", "Gas grenade with 99999 count (Slot 2)", "", 0, 14 },
            { static_cast<LCSCheat>(2000 + 15), "Molotov Cocktail", "Weapon", "Incendiary grenade with 99999 count (Slot 2)", "", 0, 15 },
            { static_cast<LCSCheat>(2000 + 34), "Detonator", "Weapon", "Bomb detonator with 99999 count (Slot 9)", "", 0, 34 },
            { static_cast<LCSCheat>(2000 + 36), "Camera", "Weapon", "Camera device (Slot 9)", "", 0, 36 },

            // Vehicle Cheats & Spawners
            { static_cast<LCSCheat>(1000 + 130), "Deimos SP", "Vehicle", "Spawns Deimos SP (car)", "", 130 },
            { static_cast<LCSCheat>(1000 + 131), "Landstalker", "Vehicle", "Spawns Landstalker (car)", "", 131 },
            { static_cast<LCSCheat>(1000 + 132), "Idaho", "Vehicle", "Spawns Idaho (car)", "", 132 },
            { static_cast<LCSCheat>(1000 + 133), "Stinger", "Vehicle", "Spawns Stinger (car)", "", 133 },
            { static_cast<LCSCheat>(1000 + 134), "Linerunner", "Vehicle", "Spawns Linerunner (car)", "", 134 },
            { static_cast<LCSCheat>(1000 + 135), "Perennial", "Vehicle", "Spawns Perennial (car)", "", 135 },
            { static_cast<LCSCheat>(1000 + 136), "Sentinel", "Vehicle", "Spawns Sentinel (car)", "", 136 },
            { static_cast<LCSCheat>(1000 + 137), "Patriot", "Vehicle", "Spawns Patriot (car)", "", 137 },
            { static_cast<LCSCheat>(1000 + 138), "Firetruck", "Vehicle", "Spawns Firetruck (car)", "", 138 },
            { static_cast<LCSCheat>(1000 + 139), "Trashmaster", "Vehicle", "Spawns Trashmaster (car)", "UNTOUCHABLE", 139 },
            { static_cast<LCSCheat>(1000 + 140), "Stretch", "Vehicle", "Spawns Stretch (car)", "", 140 },
            { static_cast<LCSCheat>(1000 + 141), "Manana", "Vehicle", "Spawns Manana (car)", "", 141 },
            { static_cast<LCSCheat>(1000 + 142), "Infernus", "Vehicle", "Spawns Infernus (car)", "", 142 },
            { static_cast<LCSCheat>(1000 + 143), "Blista", "Vehicle", "Spawns Blista (car)", "", 143 },
            { static_cast<LCSCheat>(1000 + 144), "Pony", "Vehicle", "Spawns Pony (car)", "", 144 },
            { static_cast<LCSCheat>(1000 + 145), "Mule", "Vehicle", "Spawns Mule (car)", "", 145 },
            { static_cast<LCSCheat>(1000 + 146), "Cheetah", "Vehicle", "Spawns Cheetah (car)", "", 146 },
            { static_cast<LCSCheat>(1000 + 147), "Ambulance", "Vehicle", "Spawns Ambulance (car)", "", 147 },
            { static_cast<LCSCheat>(1000 + 148), "FBI Cruiser", "Vehicle", "Spawns FBI Cruiser (car)", "", 148 },
            { static_cast<LCSCheat>(1000 + 149), "Moonbeam", "Vehicle", "Spawns Moonbeam (car)", "", 149 },
            { static_cast<LCSCheat>(1000 + 150), "Esperanto", "Vehicle", "Spawns Esperanto (car)", "", 150 },
            { static_cast<LCSCheat>(1000 + 151), "Taxi", "Vehicle", "Spawns Taxi (car)", "", 151 },
            { static_cast<LCSCheat>(1000 + 152), "Kuruma", "Vehicle", "Spawns Kuruma (car)", "", 152 },
            { static_cast<LCSCheat>(1000 + 153), "Bobcat", "Vehicle", "Spawns Bobcat (car)", "", 153 },
            { static_cast<LCSCheat>(1000 + 154), "Mr Whoopee", "Vehicle", "Spawns Mr Whoopee (car)", "", 154 },
            { static_cast<LCSCheat>(1000 + 155), "BF Injection", "Vehicle", "Spawns BF Injection (car)", "", 155 },
            { static_cast<LCSCheat>(1000 + 156), "Hearse", "Vehicle", "Spawns Hearse (car)", "", 156 },
            { static_cast<LCSCheat>(1000 + 157), "Police Cruiser", "Vehicle", "Spawns Police Cruiser (car)", "", 157 },
            { static_cast<LCSCheat>(1000 + 158), "Enforcer", "Vehicle", "Spawns Enforcer (car)", "", 158 },
            { static_cast<LCSCheat>(1000 + 159), "Securicar", "Vehicle", "Spawns Securicar (car)", "", 159 },
            { static_cast<LCSCheat>(1000 + 160), "Banshee", "Vehicle", "Spawns Banshee (car)", "", 160 },
            { static_cast<LCSCheat>(1000 + 161), "Bus", "Vehicle", "Spawns Bus (car)", "", 161 },
            { static_cast<LCSCheat>(1000 + 162), "Rhino", "Vehicle", "Spawns Rhino (car)", "TOOLOFPEACE", 162 },
            { static_cast<LCSCheat>(1000 + 163), "Barracks OL", "Vehicle", "Spawns Barracks OL (car)", "", 163 },
            { static_cast<LCSCheat>(1000 + 164), "Dodo", "Vehicle", "Spawns Dodo (car)", "", 164 },
            { static_cast<LCSCheat>(1000 + 165), "Coach", "Vehicle", "Spawns Coach (car)", "", 165 },
            { static_cast<LCSCheat>(1000 + 166), "Cabbie", "Vehicle", "Spawns Cabbie (car)", "", 166 },
            { static_cast<LCSCheat>(1000 + 167), "Stallion", "Vehicle", "Spawns Stallion (car)", "", 167 },
            { static_cast<LCSCheat>(1000 + 168), "Rumpo", "Vehicle", "Spawns Rumpo (car)", "", 168 },
            { static_cast<LCSCheat>(1000 + 169), "RC Bandit", "Vehicle", "Spawns RC Bandit (car)", "", 169 },
            { static_cast<LCSCheat>(1000 + 170), "Triad Fish Van", "Vehicle", "Spawns Triad Fish Van (car)", "", 170 },
            { static_cast<LCSCheat>(1000 + 171), "Mr Wongs", "Vehicle", "Spawns Mr Wongs (car)", "", 171 },
            { static_cast<LCSCheat>(1000 + 172), "Leone Sentinel", "Vehicle", "Spawns Leone Sentinel (car)", "", 172 },
            { static_cast<LCSCheat>(1000 + 173), "Yardie Lobo", "Vehicle", "Spawns Yardie Lobo (car)", "", 173 },
            { static_cast<LCSCheat>(1000 + 174), "Yakuza Stinger", "Vehicle", "Spawns Yakuza Stinger (car)", "", 174 },
            { static_cast<LCSCheat>(1000 + 175), "Diablo Stallion", "Vehicle", "Spawns Diablo Stallion (car)", "", 175 },
            { static_cast<LCSCheat>(1000 + 176), "Cartel Cruiser", "Vehicle", "Spawns Cartel Cruiser (car)", "", 176 },
            { static_cast<LCSCheat>(1000 + 177), "Hoods Rumpo XL", "Vehicle", "Spawns Hoods Rumpo XL (car)", "", 177 },
            { static_cast<LCSCheat>(1000 + 178), "Panlantic", "Vehicle", "Spawns Panlantic (car)", "", 178 },
            { static_cast<LCSCheat>(1000 + 179), "Flatbed", "Vehicle", "Spawns Flatbed (car)", "", 179 },
            { static_cast<LCSCheat>(1000 + 180), "Yankee", "Vehicle", "Spawns Yankee (car)", "", 180 },
            { static_cast<LCSCheat>(1000 + 181), "Bickle '76 (Taxi)", "Vehicle", "Spawns Bickle '76 (Taxi) (car)", "", 181 },
            { static_cast<LCSCheat>(1000 + 182), "TOYZ", "Vehicle", "Spawns TOYZ (car)", "", 182 },
            { static_cast<LCSCheat>(1000 + 183), "Campaign Rumpo", "Vehicle", "Spawns Campaign Rumpo (car)", "", 183 },
            { static_cast<LCSCheat>(1000 + 184), "Ballot Van", "Vehicle", "Spawns Ballot Van (car)", "", 184 },
            { static_cast<LCSCheat>(1000 + 185), "Hellenbach GT", "Vehicle", "Spawns Hellenbach GT (car)", "", 185 },
            { static_cast<LCSCheat>(1000 + 186), "Phobos VT", "Vehicle", "Spawns Phobos VT (car)", "", 186 },
            { static_cast<LCSCheat>(1000 + 187), "V8 Ghost", "Vehicle", "Spawns V8 Ghost (car)", "", 187 },
            { static_cast<LCSCheat>(1000 + 188), "Ammo Truck", "Vehicle", "Spawns Ammo Truck (car)", "", 188 },
            { static_cast<LCSCheat>(1000 + 189), "Thunder-Rodd", "Vehicle", "Spawns Thunder-Rodd (car)", "", 189 },
            { static_cast<LCSCheat>(1000 + 190), "Sindacco Argento", "Vehicle", "Spawns Sindacco Argento (car)", "", 190 },
            { static_cast<LCSCheat>(1000 + 191), "Forelli ExSess", "Vehicle", "Spawns Forelli ExSess (car)", "", 191 },
            { static_cast<LCSCheat>(1000 + 192), "Ferry", "Vehicle", "Spawns Ferry (ferry)", "", 192 },
            { static_cast<LCSCheat>(1000 + 193), "Ghost", "Vehicle", "Spawns Ghost (boat)", "", 193 },
            { static_cast<LCSCheat>(1000 + 194), "Speeder", "Vehicle", "Spawns Speeder (boat)", "", 194 },
            { static_cast<LCSCheat>(1000 + 195), "Reefer", "Vehicle", "Spawns Reefer (boat)", "", 195 },
            { static_cast<LCSCheat>(1000 + 196), "Predator", "Vehicle", "Spawns Predator (boat)", "", 196 },
            { static_cast<LCSCheat>(1000 + 197), "Train", "Vehicle", "Spawns Train (train)", "", 197 },
            { static_cast<LCSCheat>(1000 + 198), "Escape Helicopter", "Vehicle", "Spawns Escape Helicopter (heli)", "", 198 },
            { static_cast<LCSCheat>(1000 + 199), "Police Chopper", "Vehicle", "Spawns Police Chopper (heli)", "", 199 },
            { static_cast<LCSCheat>(1000 + 200), "Airtrain", "Vehicle", "Spawns Airtrain (plane)", "", 200 },
            { static_cast<LCSCheat>(1000 + 201), "Deaddodo Plane", "Vehicle", "Spawns Deaddodo Plane (plane)", "", 201 },
            { static_cast<LCSCheat>(1000 + 202), "Angel", "Vehicle", "Spawns Angel (bike)", "", 202 },
            { static_cast<LCSCheat>(1000 + 203), "Pizzaboy", "Vehicle", "Spawns Pizzaboy (bike)", "", 203 },
            { static_cast<LCSCheat>(1000 + 204), "Noodleboy", "Vehicle", "Spawns Noodleboy (bike)", "", 204 },
            { static_cast<LCSCheat>(1000 + 205), "PCJ-600", "Vehicle", "Spawns PCJ-600 (bike)", "", 205 },
            { static_cast<LCSCheat>(1000 + 206), "Faggio", "Vehicle", "Spawns Faggio (bike)", "", 206 },
            { static_cast<LCSCheat>(1000 + 207), "Freeway", "Vehicle", "Spawns Freeway (bike)", "", 207 },
            { static_cast<LCSCheat>(1000 + 208), "Avenger (Angel)", "Vehicle", "Spawns Avenger (Angel) (bike)", "", 208 },
            { static_cast<LCSCheat>(1000 + 209), "Manchez (Dirt Bike)", "Vehicle", "Spawns Manchez (Dirt Bike) (bike)", "", 209 },
            { static_cast<LCSCheat>(1000 + 210), "Sanchez", "Vehicle", "Spawns Sanchez (bike)", "", 210 },
            { static_cast<LCSCheat>(1000 + 211), "RC Goblin", "Vehicle", "Spawns RC Goblin (car)", "", 211 },
            { static_cast<LCSCheat>(1000 + 212), "RC Raider", "Vehicle", "Spawns RC Raider (car)", "", 212 },
            { static_cast<LCSCheat>(1000 + 213), "Hunter", "Vehicle", "Spawns Hunter (car)", "", 213 },
            { static_cast<LCSCheat>(1000 + 214), "Maverick", "Vehicle", "Spawns Maverick (car)", "", 214 },
            { static_cast<LCSCheat>(1000 + 215), "Police Maverick", "Vehicle", "Spawns Police Maverick (car)", "", 215 },
            { static_cast<LCSCheat>(1000 + 216), "VCN Maverick", "Vehicle", "Spawns VCN Maverick (car)", "", 216 },
            { static_cast<LCSCheat>(1000 + 217), "Infernus Classic", "Vehicle", "Spawns Infernus Classic (car)", "", 217 },
            { static_cast<LCSCheat>(1000 + 218), "Impreza WRX", "Vehicle", "Spawns Impreza WRX (car)", "", 218 },
            { static_cast<LCSCheat>(1000 + 219), "Supra", "Vehicle", "Spawns Supra (car)", "", 219 },
            { static_cast<LCSCheat>(1000 + 220), "Skoiloine (Skyline)", "Vehicle", "Spawns Skoiloine (Skyline) (car)", "", 220 },
            { static_cast<LCSCheat>(1000 + 221), "Cyclone GT", "Vehicle", "Spawns Cyclone GT (car)", "", 221 },
            { static_cast<LCSCheat>(1000 + 222), "Tommy (Mini)", "Vehicle", "Spawns Tommy (Mini) (car)", "", 222 },
            { static_cast<LCSCheat>(1000 + 223), "Leone Sentinel (Bloody)", "Vehicle", "Spawns Leone Sentinel (Bloody) (car)", "", 223 },
            { static_cast<LCSCheat>(1000 + 224), "Campaign Maverick", "Vehicle", "Spawns Campaign Maverick (car)", "", 224 },
            { static_cast<LCSCheat>(1000 + 225), "PCJ-600 Classic", "Vehicle", "Spawns PCJ-600 Classic (bike)", "", 225 },
            { static_cast<LCSCheat>(1000 + 226), "FBI Kuruma", "Vehicle", "Spawns FBI Kuruma (car)", "", 226 },
            { static_cast<LCSCheat>(1000 + 227), "Manana (Corpse)", "Vehicle", "Spawns Manana (Corpse) (car)", "", 227 },
            { static_cast<LCSCheat>(1000 + 228), "Borgnine Sedan", "Vehicle", "Spawns Borgnine Sedan (car)", "", 228 },
            { static_cast<LCSCheat>(1000 + 229), "LCPD Cruiser (Premier)", "Vehicle", "Spawns LCPD Cruiser (Premier) (car)", "", 229 },
            { static_cast<LCSCheat>(1000 + 230), "Top Fun", "Vehicle", "Spawns Top Fun (car)", "", 230 },
            { static_cast<LCSCheat>(1000 + 231), "Hotring Racer A", "Vehicle", "Spawns Hotring Racer A (car)", "", 231 },
            { static_cast<LCSCheat>(1000 + 232), "Hotring Racer B", "Vehicle", "Spawns Hotring Racer B (car)", "", 232 },
            { static_cast<LCSCheat>(1000 + 233), "Hotring Racer C", "Vehicle", "Spawns Hotring Racer C (car)", "", 233 },
            { static_cast<LCSCheat>(1000 + 234), "Comet", "Vehicle", "Spawns Comet (car)", "", 234 },
            { static_cast<LCSCheat>(1000 + 235), "Deluxo", "Vehicle", "Spawns Deluxo (car)", "", 235 },
            { static_cast<LCSCheat>(1000 + 236), "Cheetah Classic", "Vehicle", "Spawns Cheetah Classic (car)", "", 236 },

            { LCSCheat::CarsDriveOnWater, "Cars Drive On Water", "Vehicle", "Vehicles float and drive across water", "JESUSCAR" },
            { LCSCheat::VehicleGodMode, "Vehicle God Mode", "Vehicle", "Indestructible vehicle (no scratch, dent, or explosion)", "" },
            { LCSCheat::RepairVehicle, "Repair Current Vehicle", "Vehicle", "Instantly restores vehicle health, fixes all body damage/panels, and flips upright", "" },
            { LCSCheat::DestroyAllCars, "Blow Up All Cars", "Vehicle", "Detonates all vehicles in area", "" },
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

            // 7. Configure status flags (STATUS_ABANDONED = 0x20) and unlocked doors
            uint8_t* pStatus = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pVeh) + 0x50);
            *pStatus = (*pStatus & 7) | 0x20;

            *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(pVeh) + 0x230) = 1; // Door lock = CARLOCK_UNLOCKED

            // Configure engine cleanup flags so CCarCtrl::Prune deletes the vehicle when player moves far away:
            // 0x1FA: Bit 1 (0x02) = cleanable / bCanBeDeleted
            // Bit 4 (0x10) = bIsMissionCar (MUST be 0 so engine does not protect it permanently)
            uint8_t* pFlags1FA = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pVeh) + 0x1FA);
            *pFlags1FA = (*pFlags1FA | 0x02) & ~0x10;

            // 0x1FB: Ensure standard damageability (NOT proof, can be damaged/dented/blown up normally)
            uint8_t* pFlags1FB = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(pVeh) + 0x1FB);
            *pFlags1FB = (*pFlags1FB & ~0x1F) | 0x40;

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

    __declspec(naked) static void Hook_SampleLookup()
    {
        __asm
        {
            // Verify sound bank pointer in ecx
            cmp ecx, 0x10000
            jb ret_zero_bank

            // If sound bank data pointer [ecx+4] is NULL or invalid, return NULL safely
            mov eax, [ecx + 4]
            cmp eax, 0x10000
            jb ret_zero_bank

            push ebx
            mov ebx, [esp + 8]
            sub ebx, [ecx + 0x0C]
            imul ebx, ebx, 0x3C
            add ebx, eax
            cmp ebx, 0x10000
            jb ret_zero_sample

            // Jump to 0x401022 to resume native engine execution
            mov eax, 0x401022
            jmp eax

        ret_zero_sample:
            pop ebx
        ret_zero_bank:
            xor eax, eax
            ret 4
        }
    }

    __declspec(naked) static void Hook_PlaySampleCheck()
    {
        __asm
        {
            // Verify sample pointer returned from 0x401010
            cmp eax, 0x10000
            jb ret_null

            mov ebp, eax

            // Check if pRwObject [esp + 0x0C] is a valid pointer
            mov eax, [esp + 0x0C]
            cmp eax, 0x10000
            jb ret_null

            // Dereference the plugin offset at address 0x978798
            mov edx, 0x978798
            mov edx, dword ptr [edx]
            test edx, edx
            js ret_null

            add eax, edx
            cmp eax, 0x10000
            jb ret_null

            xor edx, edx
            mov ebx, [eax]
            cmp ebx, 0x10000
            jb ret_null

            // Jump to 0x4058E0 to continue native engine execution
            mov edx, 0x4058E0
            jmp edx

        ret_null:
            xor eax, eax
            pop ebp
            pop ebx
            ret
        }
    }

    __declspec(naked) static void Hook_AudioSampleConfig1()
    {
        __asm
        {
            cmp ecx, 0x10000
            jb ret_safe
            mov dword ptr [ecx + 0x30], 1
            mov eax, [esp + 4]
            mov [ecx + 0x34], eax
            mov eax, [esp + 8]
            mov [ecx + 0x38], eax
        ret_safe:
            ret 8
        }
    }

    __declspec(naked) static void Hook_AudioSampleConfig2()
    {
        __asm
        {
            cmp ecx, 0x10000
            jb ret_safe
            mov dword ptr [ecx + 0x30], 2
            mov eax, [esp + 4]
            mov [ecx + 0x34], eax
            mov eax, [esp + 8]
            mov [ecx + 0x38], eax
        ret_safe:
            ret 8
        }
    }

    __declspec(naked) static void Hook_AudioSampleConfig3()
    {
        __asm
        {
            cmp ecx, 0x10000
            jb ret_safe
            fld dword ptr [esp + 4]
            fstp dword ptr [ecx + 0x18]
            fld dword ptr [esp + 8]
            fstp dword ptr [ecx + 0x1C]
        ret_safe:
            ret 8
        }
    }

    __declspec(naked) static void Hook_AudioSampleConfig4()
    {
        __asm
        {
            cmp ecx, 0x10000
            jb ret_safe
            or word ptr [ecx + 0x2E], 1
            push dword ptr [esp + 4]
            mov eax, 0x401770
            call eax
        ret_safe:
            ret 4
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

        // Hook 4: Audio sample bank safety check at 0x401010 to prevent crash when sound bank unloaded
        void* pAudioAddr = reinterpret_cast<void*>(0x401010);
        if (VirtualProtect(pAudioAddr, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(pAudioAddr);
            p[0] = 0xE9;
            uintptr_t callNext = 0x401010 + 5;
            int32_t relOffset = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_SampleLookup) - callNext);
            *reinterpret_cast<int32_t*>(&p[1]) = relOffset;
            VirtualProtect(pAudioAddr, 5, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pAudioAddr, 5);
        }

        // Hook 5: Audio sample playback safety check at 0x4058D0 to prevent null dereference at 0x4058E0
        void* pPlayAddr = reinterpret_cast<void*>(0x4058D0);
        const size_t playPatchSize = 16;
        if (VirtualProtect(pPlayAddr, playPatchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(pPlayAddr);
            p[0] = 0xE9;
            uintptr_t callNext = 0x4058D0 + 5;
            int32_t relOffset = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_PlaySampleCheck) - callNext);
            *reinterpret_cast<int32_t*>(&p[1]) = relOffset;
            for (size_t i = 5; i < playPatchSize; ++i)
            {
                p[i] = 0x90;
            }
            VirtualProtect(pPlayAddr, playPatchSize, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pPlayAddr, playPatchSize);
        }

        // Hook 6: Audio sample config 1 safety at 0x4016A0 (prevent crash on vehicle entry)
        void* pCfg1 = reinterpret_cast<void*>(0x4016A0);
        const size_t cfg1Size = 24;
        if (VirtualProtect(pCfg1, cfg1Size, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(pCfg1);
            p[0] = 0xE9;
            uintptr_t callNext = 0x4016A0 + 5;
            int32_t relOffset = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_AudioSampleConfig1) - callNext);
            *reinterpret_cast<int32_t*>(&p[1]) = relOffset;
            for (size_t i = 5; i < cfg1Size; ++i) p[i] = 0x90;
            VirtualProtect(pCfg1, cfg1Size, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pCfg1, cfg1Size);
        }

        // Hook 7: Audio sample config 2 safety at 0x4016C0
        void* pCfg2 = reinterpret_cast<void*>(0x4016C0);
        const size_t cfg2Size = 24;
        if (VirtualProtect(pCfg2, cfg2Size, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(pCfg2);
            p[0] = 0xE9;
            uintptr_t callNext = 0x4016C0 + 5;
            int32_t relOffset = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_AudioSampleConfig2) - callNext);
            *reinterpret_cast<int32_t*>(&p[1]) = relOffset;
            for (size_t i = 5; i < cfg2Size; ++i) p[i] = 0x90;
            VirtualProtect(pCfg2, cfg2Size, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pCfg2, cfg2Size);
        }

        // Hook 8: Audio sample config 3 safety at 0x4016E0
        void* pCfg3 = reinterpret_cast<void*>(0x4016E0);
        const size_t cfg3Size = 17;
        if (VirtualProtect(pCfg3, cfg3Size, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(pCfg3);
            p[0] = 0xE9;
            uintptr_t callNext = 0x4016E0 + 5;
            int32_t relOffset = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_AudioSampleConfig3) - callNext);
            *reinterpret_cast<int32_t*>(&p[1]) = relOffset;
            for (size_t i = 5; i < cfg3Size; ++i) p[i] = 0x90;
            VirtualProtect(pCfg3, cfg3Size, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pCfg3, cfg3Size);
        }

        // Hook 9: Audio sample config 4 safety at 0x401700
        void* pCfg4 = reinterpret_cast<void*>(0x401700);
        const size_t cfg4Size = 17;
        if (VirtualProtect(pCfg4, cfg4Size, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            uint8_t* p = reinterpret_cast<uint8_t*>(pCfg4);
            p[0] = 0xE9;
            uintptr_t callNext = 0x401700 + 5;
            int32_t relOffset = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&Hook_AudioSampleConfig4) - callNext);
            *reinterpret_cast<int32_t*>(&p[1]) = relOffset;
            for (size_t i = 5; i < cfg4Size; ++i) p[i] = 0x90;
            VirtualProtect(pCfg4, cfg4Size, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), pCfg4, cfg4Size);
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
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeMaintainVehicleGodMode(uintptr_t pPlayerInfo)
    {
        __try
        {
            if (!pPlayerInfo) return;
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (!pPed) return;

            uintptr_t pVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3A8);
            if (!pVeh)
                pVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3AC);

            if (pVeh)
            {
                // 1. Set proof flags (bullet, fire, explosion, collision, melee) and disable bCanBeDamaged
                uint8_t* pProof = reinterpret_cast<uint8_t*>(pVeh + 0x1FB);
                *pProof = (*pProof | 0x1F) & ~0x40;

                // 2. Lock vehicle health at 1000.0f
                *reinterpret_cast<float*>(pVeh + 0x204) = 1000.0f;

                // 3. Clear damage / fire timers so engine never starts smoking or burning
                *reinterpret_cast<uint32_t*>(pVeh + 0x560) = 0;
                *reinterpret_cast<uint32_t*>(pVeh + 0x494) = 0;

                // 4. Extinguish fire
                *reinterpret_cast<uintptr_t*>(pVeh + 0x1E8) = 0;

                // 5. Check if vehicle is an automobile or bike:
                uintptr_t* pModelInfoPtrs = reinterpret_cast<uintptr_t*>(0x92D4C8);
                int modelId = *reinterpret_cast<int16_t*>(pVeh + 0x5C);
                uintptr_t pModelInfo = (pModelInfoPtrs && modelId >= 0 && modelId < 300) ? pModelInfoPtrs[modelId] : 0;
                int vehType = pModelInfo ? *reinterpret_cast<int*>(pModelInfo + 0x3C) : -1;
                bool isBike = (vehType == 5) || (modelId >= 202 && modelId <= 210) || (modelId == 225);
                bool isBoat = (vehType == 1) || (modelId >= 192 && modelId <= 196);

                if (!isBike && !isBoat)
                {
                    // Monitor CDamageManager at pVeh + 0x2A0:
                    // If any panel, door, light, or wheel took any scratch/dent, immediately restore pristine mesh
                    uint32_t* pDmg = reinterpret_cast<uint32_t*>(pVeh + 0x2A0);
                    bool hasDamage = (pDmg[1] != 0 || pDmg[2] != 0 || pDmg[3] != 0 || pDmg[4] != 0 || pDmg[5] != 0);
                    if (hasDamage)
                    {
                        typedef void(__thiscall* AutoFixFn_t)(void*);
                        reinterpret_cast<AutoFixFn_t>(0x588530)(reinterpret_cast<void*>(pVeh));
                        memset(reinterpret_cast<void*>(pVeh + 0x2A0), 0, 0x18);
                    }
                }
                else if (isBike)
                {
                    typedef void(__thiscall* BikeFixFn_t)(void*);
                    reinterpret_cast<BikeFixFn_t>(0x609F00)(reinterpret_cast<void*>(pVeh));
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeDisableVehicleGodMode(uintptr_t pPlayerInfo)
    {
        __try
        {
            if (!pPlayerInfo) return;
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (!pPed) return;

            uintptr_t pVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3A8);
            if (!pVeh)
                pVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3AC);

            if (pVeh)
            {
                // Clear proof flags and re-enable bCanBeDamaged
                uint8_t* pProof = reinterpret_cast<uint8_t*>(pVeh + 0x1FB);
                *pProof = (*pProof & ~0x1F) | 0x40;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeRepairCurrentVehicle(uintptr_t pPlayerInfo)
    {
        __try
        {
            if (!pPlayerInfo) return;
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (!pPed) return;

            uintptr_t pVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3A8);
            if (!pVeh)
                pVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3AC);

            if (!pVeh) return;

            // 1. Restore vehicle health to 1000.0f
            *reinterpret_cast<float*>(pVeh + 0x204) = 1000.0f;

            // 2. Clear damage / fire timers (Pay N Spray offsets 0x560 & 0x494)
            *reinterpret_cast<uint32_t*>(pVeh + 0x560) = 0;
            *reinterpret_cast<uint32_t*>(pVeh + 0x494) = 0;

            // 3. Extinguish fire
            *reinterpret_cast<uintptr_t*>(pVeh + 0x1E8) = 0;

            // 4. Ensure doors unlocked (CARLOCK_UNLOCKED)
            *reinterpret_cast<uint32_t*>(pVeh + 0x230) = 1;

            // 5. Restore vehicle geometries, panels, doors, lights, and engine status
            uintptr_t* pModelInfoPtrs = reinterpret_cast<uintptr_t*>(0x92D4C8);
            int modelId = *reinterpret_cast<int16_t*>(pVeh + 0x5C);
            uintptr_t pModelInfo = (pModelInfoPtrs && modelId >= 0 && modelId < 300) ? pModelInfoPtrs[modelId] : 0;
            int vehType = pModelInfo ? *reinterpret_cast<int*>(pModelInfo + 0x3C) : -1;
            bool isBike = (vehType == 5) || (modelId >= 202 && modelId <= 210) || (modelId == 225);

            if (isBike)
            {
                typedef void(__thiscall* BikeFixFn_t)(void*);
                reinterpret_cast<BikeFixFn_t>(0x609F00)(reinterpret_cast<void*>(pVeh));
            }
            else
            {
                typedef void(__thiscall* AutoFixFn_t)(void*);
                reinterpret_cast<AutoFixFn_t>(0x588530)(reinterpret_cast<void*>(pVeh));
                memset(reinterpret_cast<void*>(pVeh + 0x2A0), 0, 0x18);
            }

            // 6. Flip upright if overturned / upside-down (up.z < 0.0f) matching Pay N Spray / CLEO-iOS
            float up_z = *reinterpret_cast<float*>(pVeh + 0x2C);
            if (up_z < 0.0f)
            {
                float fwd_x = *reinterpret_cast<float*>(pVeh + 0x14);
                float fwd_y = *reinterpret_cast<float*>(pVeh + 0x18);
                float heading = atan2f(-fwd_x, fwd_y);

                typedef void(__thiscall* SetRotateFn_t)(void*, float, float, float);
                reinterpret_cast<SetRotateFn_t>(0x4DF240)(
                    reinterpret_cast<void*>(pVeh + 0x04),
                    0.0f,
                    0.0f,
                    heading
                );
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeBlowUpAllCars()
    {
        __try
        {
            uintptr_t pPool = *reinterpret_cast<uintptr_t*>(0xA0FDE4);
            if (!pPool) return;

            uint8_t* byteMap = *reinterpret_cast<uint8_t**>(pPool + 0x00);
            uintptr_t pObjects = *reinterpret_cast<uintptr_t*>(pPool + 0x04);
            int nSize = *reinterpret_cast<int*>(pPool + 0x08);

            if (!byteMap || !pObjects || nSize <= 0) return;

            uintptr_t pPed = 0;
            uintptr_t pPlayerVeh = 0;
            uintptr_t* pPlayerInfo = reinterpret_cast<uintptr_t*>(0x94AD28);
            if (pPlayerInfo && *pPlayerInfo)
            {
                pPed = *pPlayerInfo;
                if (pPed)
                {
                    pPlayerVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3A8);
                    if (!pPlayerVeh)
                        pPlayerVeh = *reinterpret_cast<uintptr_t*>(pPed + 0x3AC);
                }
            }

            typedef void(__thiscall* BlowUpCarFn_t)(void*, void*, bool);
            BlowUpCarFn_t pfnBlowUp = reinterpret_cast<BlowUpCarFn_t>(0x587D50);

            for (int i = 0; i < nSize; ++i)
            {
                // In CPool, slot is valid / allocated if (byteMap[i] & 0x80) == 0
                if ((byteMap[i] & 0x80) == 0)
                {
                    uintptr_t pVeh = pObjects + (i * 0x5DC);
                    // If vehicle god mode is active on player's car, protect it!
                    if (CheatManager::IsVehicleGodModeEnabled() && pVeh == pPlayerVeh)
                    {
                        continue;
                    }

                    // Remove proof flags and enable bCanBeDamaged
                    *reinterpret_cast<uint8_t*>(pVeh + 0x1FB) &= ~0x1F;
                    *reinterpret_cast<uint8_t*>(pVeh + 0x1FB) |= 0x40;

                    // Detonate car instantly
                    pfnBlowUp(reinterpret_cast<void*>(pVeh), reinterpret_cast<void*>(pPed), false);
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

    static int GetWeaponSlot(int weaponType)
    {
        switch (weaponType)
        {
        case 1:  return 0; // Brass Knuckles
        case 2: case 3: case 4: case 5: case 6:
        case 7: case 8: case 9: case 10: case 11:
            return 1; // Melee
        case 12: case 13: case 14: case 15:
            return 2; // Projectiles
        case 17: case 18:
            return 3; // Handguns
        case 19: case 20: case 21:
            return 4; // Shotguns
        case 22: case 23: case 24: case 25:
            return 5; // SMGs
        case 26: case 27:
            return 6; // Rifles
        case 30: case 31: case 32: case 33:
            return 7; // Heavy
        case 28: case 29:
            return 8; // Sniper
        case 34: case 36:
            return 9; // Special (Detonator, Camera)
        default:
            return -1;
        }
    }

    static void SafeGiveSingleWeapon(uintptr_t pPlayerInfo, int weaponType)
    {
        __try
        {
            if (!pPlayerInfo) return;
            uintptr_t pPed = *reinterpret_cast<uintptr_t*>(pPlayerInfo);
            if (!pPed || weaponType <= 0) return;

            int slot = GetWeaponSlot(weaponType);
            if (slot < 0 || slot >= 10) return;

            // 1. Get weapon model IDs from CWeaponInfo
            typedef void*(__cdecl* GetWeaponInfoFn_t)(int);
            GetWeaponInfoFn_t pfnGetWeaponInfo = reinterpret_cast<GetWeaponInfoFn_t>(0x5D5710);
            void* pInfo = pfnGetWeaponInfo(weaponType);
            int modelId = -1;
            int model2Id = -1;
            if (pInfo)
            {
                modelId = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pInfo) + 0x54);
                model2Id = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pInfo) + 0x58);
            }

            // 2. Request and load models into CStreaming
            typedef void(__cdecl* RequestModelFn_t)(int, int);
            RequestModelFn_t pfnRequestModel = reinterpret_cast<RequestModelFn_t>(0x40E310);
            typedef void(__cdecl* LoadAllRequestedModelsFn_t)(bool);
            LoadAllRequestedModelsFn_t pfnLoadModels = reinterpret_cast<LoadAllRequestedModelsFn_t>(0x40B5F0);
            typedef void(__cdecl* SetModelIsDeletableFn_t)(int);
            SetModelIsDeletableFn_t pfnSetModelIsDeletable = reinterpret_cast<SetModelIsDeletableFn_t>(0x40AEA0);

            if (modelId > 0)
                pfnRequestModel(modelId, 1);
            if (model2Id > 0)
                pfnRequestModel(model2Id, 1);
            pfnLoadModels(false);

            // 3. Remove old weapon model attached to Tommy's hand
            typedef void(__thiscall* RemoveWeaponModelFn_t)(void*, int);
            RemoveWeaponModelFn_t pfnRemoveWeaponModel = reinterpret_cast<RemoveWeaponModelFn_t>(0x4FFD80);
            pfnRemoveWeaponModel(reinterpret_cast<void*>(pPed), -1);

            // 4. Give the weapon to player ped
            int ammoCount = (slot == 0 || slot == 1) ? 1 : 99999;
            typedef void(__thiscall* GiveWeaponFn_t)(void*, int, unsigned int, bool);
            GiveWeaponFn_t pfnGiveWeapon = reinterpret_cast<GiveWeaponFn_t>(0x4FFA30);
            pfnGiveWeapon(reinterpret_cast<void*>(pPed), weaponType, static_cast<unsigned int>(ammoCount), true);

            // 5. Explicitly switch active weapon to this slot and attach the model + update HUD
            typedef void(__thiscall* SetCurrentWeaponFn_t)(void*, int);
            SetCurrentWeaponFn_t pfnSetCurrentWeapon = reinterpret_cast<SetCurrentWeaponFn_t>(0x4FF900);
            pfnSetCurrentWeapon(reinterpret_cast<void*>(pPed), slot);

            // 6. Mark models as deletable so streaming engine manages them normally
            if (modelId > 0)
                pfnSetModelIsDeletable(modelId);
            if (model2Id > 0)
                pfnSetModelIsDeletable(model2Id);

            // 7. Directly guarantee massive ammo in slot
            if (slot != 0 && slot != 1)
            {
                uintptr_t pWp = pPed + 0x408 + (slot * 0x18);
                *reinterpret_cast<int*>(pWp + 0x08) = 9999;  // m_nAmmoInClip
                *reinterpret_cast<int*>(pWp + 0x0C) = 99999; // m_nAmmoTotal
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void SafeGiveWeapon(uintptr_t pPed, int weaponType, int ammo)
    {
        __try
        {
            if (!pPed || weaponType <= 0) return;

            typedef void*(__cdecl* GetWeaponInfoFn_t)(int);
            GetWeaponInfoFn_t pfnGetWeaponInfo = reinterpret_cast<GetWeaponInfoFn_t>(0x5D5710);
            void* pInfo = pfnGetWeaponInfo(weaponType);
            int modelId = -1;
            int model2Id = -1;
            if (pInfo)
            {
                modelId = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pInfo) + 0x54);
                model2Id = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(pInfo) + 0x58);
            }

            typedef void(__cdecl* RequestModelFn_t)(int, int);
            RequestModelFn_t pfnRequestModel = reinterpret_cast<RequestModelFn_t>(0x40E310);
            typedef void(__cdecl* LoadAllRequestedModelsFn_t)(bool);
            LoadAllRequestedModelsFn_t pfnLoadModels = reinterpret_cast<LoadAllRequestedModelsFn_t>(0x40B5F0);
            typedef void(__cdecl* SetModelIsDeletableFn_t)(int);
            SetModelIsDeletableFn_t pfnSetModelIsDeletable = reinterpret_cast<SetModelIsDeletableFn_t>(0x40AEA0);

            if (modelId > 0)
                pfnRequestModel(modelId, 1);
            if (model2Id > 0)
                pfnRequestModel(model2Id, 1);
            pfnLoadModels(false);

            typedef void(__thiscall* GiveWeaponFn_t)(void*, int, unsigned int, bool);
            GiveWeaponFn_t pfnGiveWeapon = reinterpret_cast<GiveWeaponFn_t>(0x4FFA30);
            pfnGiveWeapon(reinterpret_cast<void*>(pPed), weaponType, static_cast<unsigned int>(ammo), true);

            if (modelId > 0)
                pfnSetModelIsDeletable(modelId);
            if (model2Id > 0)
                pfnSetModelIsDeletable(model2Id);
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

            typedef void(__thiscall* SetCurrentWeaponFn_t)(void*, int);
            SetCurrentWeaponFn_t pfnSetCurrentWeapon = reinterpret_cast<SetCurrentWeaponFn_t>(0x4FF900);

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

                pfnSetCurrentWeapon(reinterpret_cast<void*>(pPed), 6); // Equip Ruger
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

                pfnSetCurrentWeapon(reinterpret_cast<void*>(pPed), 6); // Equip M4
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

                pfnSetCurrentWeapon(reinterpret_cast<void*>(pPed), 7); // Equip Minigun
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

        case LCSCheat::ClearWeapons:
            SafeClearWeapons(*reinterpret_cast<uintptr_t*>(pPlayerInfo));
            break;

        case LCSCheat::VehicleGodMode:
            s_bVehicleGodMode = !s_bVehicleGodMode;
            if (!s_bVehicleGodMode)
            {
                SafeDisableVehicleGodMode(pPlayerInfo);
            }
            break;

        case LCSCheat::RepairVehicle:
            SafeRepairCurrentVehicle(pPlayerInfo);
            break;

        case LCSCheat::DestroyAllCars:
            SafeBlowUpAllCars();
            break;

        default:
            // Check if weapon selector
            if (info->weaponTypeId > 0)
            {
                SafeGiveSingleWeapon(pPlayerInfo, info->weaponTypeId);
            }
            // Check if vehicle spawner
            else if (info->vehicleModelId > 0)
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
        s_lastActivatedCategory = info->category;
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

        // Maintain Vehicle God Mode if toggled on
        if (s_bVehicleGodMode)
        {
            SafeMaintainVehicleGodMode(0x94AD28);
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
