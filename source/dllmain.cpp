#include "pch.h"
#include "CrashLogger.h"
#include "CheatManager.h"
#include "OverlayMenu.h"
#include "D3D8Hook.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        ReLCS::CrashLogger::Init();
        ReLCS::CrashLogger::LogMessage("=== ReLCS-Cleo.asi loaded ===");
        ReLCS::CheatManager::Init();
        ReLCS::OverlayMenu::Init();
        ReLCS::D3D8Hook::Init();
        break;

    case DLL_PROCESS_DETACH:
        ReLCS::D3D8Hook::Uninstall();
        ReLCS::CrashLogger::LogMessage("=== ReLCS-Cleo.asi unloaded ===");
        ReLCS::CrashLogger::Uninstall();
        break;
    }
    return TRUE;
}
