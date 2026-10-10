#include "D3D8Hook.h"
#include "OverlayMenu.h"
#include "CrashLogger.h"

namespace ReLCS
{
    D3D8Hook::EndScene9_t D3D8Hook::s_origEndScene = nullptr;
    D3D8Hook::Reset9_t D3D8Hook::s_origReset = nullptr;
    D3D8Hook::Present9_t D3D8Hook::s_origPresent = nullptr;
    void** D3D8Hook::s_pVTable = nullptr;
    std::atomic<bool> D3D8Hook::s_bHooked(false);

    typedef void* (*RwD3D9GetCurrentD3DDevice_t)(void);

    static void* SafeGetD3DDevice(RwD3D9GetCurrentD3DDevice_t pfn)
    {
        __try
        {
            return pfn();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    static void SafeRenderOverlay(IDirect3DDevice9* pDevice)
    {
        __try
        {
            OverlayMenu::Render(pDevice);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Fail safely to prevent overlay rendering crash from killing the game
        }
    }

    static HWND FindGameWindow()
    {
        DWORD currentPid = GetCurrentProcessId();

        // 1. Try common GTA window classes
        const char* classes[] = { "Grand theft auto 3", "Grand Theft Auto", nullptr };
        for (int i = 0; classes[i] != nullptr; i++)
        {
            HWND hWnd = FindWindowA(classes[i], nullptr);
            if (hWnd && IsWindowVisible(hWnd))
            {
                DWORD pid = 0;
                GetWindowThreadProcessId(hWnd, &pid);
                if (pid == currentPid)
                    return hWnd;
            }
        }

        // 2. Try window title
        HWND hWndTitle = FindWindowA(nullptr, "GTA: Liberty City Stories");
        if (hWndTitle && IsWindowVisible(hWndTitle))
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(hWndTitle, &pid);
            if (pid == currentPid)
                return hWndTitle;
        }

        // 3. Enumerate top-level windows of current process
        struct EnumData { DWORD pid; HWND found; };
        EnumData ed = { currentPid, nullptr };
        EnumWindows([](HWND h, LPARAM lp) -> BOOL {
            EnumData* pEd = reinterpret_cast<EnumData*>(lp);
            DWORD p = 0;
            GetWindowThreadProcessId(h, &p);
            if (p == pEd->pid && IsWindowVisible(h))
            {
                char title[128] = { 0 };
                GetWindowTextA(h, title, sizeof(title));
                if (strlen(title) > 0)
                {
                    pEd->found = h;
                    return FALSE;
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ed));

        return ed.found;
    }

    void D3D8Hook::Init()
    {
        CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
    }

    void D3D8Hook::Uninstall()
    {
        if (s_bHooked && s_pVTable)
        {
            DWORD oldProtect;
            if (VirtualProtect(&s_pVTable[0], sizeof(void*) * 60, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                if (s_origReset) s_pVTable[16] = reinterpret_cast<void*>(s_origReset);
                if (s_origPresent) s_pVTable[17] = reinterpret_cast<void*>(s_origPresent);
                if (s_origEndScene) s_pVTable[42] = reinterpret_cast<void*>(s_origEndScene);
                VirtualProtect(&s_pVTable[0], sizeof(void*) * 60, oldProtect, &oldProtect);
            }
            s_bHooked = false;
        }
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_EndScene(IDirect3DDevice9* pThis)
    {
        // Render menu overlay at the end of the scene before buffer is presented
        SafeRenderOverlay(pThis);

        if (s_origEndScene)
            return s_origEndScene(pThis);

        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_Present(
        IDirect3DDevice9* pThis,
        CONST RECT* pSourceRect,
        CONST RECT* pDestRect,
        HWND hDestWindowOverride,
        CONST RGNDATA* pDirtyRegion)
    {
        if (s_origPresent)
            return s_origPresent(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_Reset(
        IDirect3DDevice9* pThis,
        D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        OverlayMenu::OnLostDevice();

        HRESULT hr = D3D_OK;
        if (s_origReset)
            hr = s_origReset(pThis, pPresentationParameters);

        if (SUCCEEDED(hr))
            OverlayMenu::OnResetDevice(pThis);

        return hr;
    }

    DWORD WINAPI D3D8Hook::HookThread(LPVOID lpParam)
    {
        CrashLogger::LogMessage("[D3DHook] Hook thread started. Waiting for game window creation...");

        // 1. Wait for game window to be created and visible (up to 30 seconds)
        HWND hGameWnd = nullptr;
        for (int i = 0; i < 150 && !hGameWnd; i++)
        {
            hGameWnd = FindGameWindow();
            if (!hGameWnd)
                Sleep(200);
        }

        if (!hGameWnd)
        {
            CrashLogger::LogMessage("[D3DHook] Game window not detected yet, will continue waiting in background...");
        }
        else
        {
            CrashLogger::LogMessage("[D3DHook] Game window detected! Waiting 2 seconds for RenderWare 3D device to finish setup...");
            Sleep(2000);
        }

        // 2. Query RenderWare Direct3D 9 device safely
        HMODULE hRw = nullptr;
        for (int i = 0; i < 100 && !hRw; i++)
        {
            hRw = GetModuleHandleA("rwd3d9.dll");
            if (!hRw)
                Sleep(200);
        }

        IDirect3DDevice9* pDevice = nullptr;

        if (hRw)
        {
            RwD3D9GetCurrentD3DDevice_t pfnGetDevice =
                reinterpret_cast<RwD3D9GetCurrentD3DDevice_t>(GetProcAddress(hRw, "RwD3D9GetCurrentD3DDevice"));

            if (pfnGetDevice)
            {
                CrashLogger::LogMessage("[D3DHook] Found RwD3D9GetCurrentD3DDevice export. Polling device safely...");
                for (int i = 0; i < 150 && !pDevice; i++)
                {
                    pDevice = reinterpret_cast<IDirect3DDevice9*>(SafeGetD3DDevice(pfnGetDevice));
                    if (!pDevice)
                        Sleep(200);
                }
            }
        }

        if (!pDevice)
        {
            CrashLogger::LogMessage("[D3DHook] Could not acquire IDirect3DDevice9 from rwd3d9.dll.");
            return 0;
        }

        CrashLogger::LogMessage("[D3DHook] Successfully acquired active IDirect3DDevice9! Patching vtable...");

        // Hook vtable
        s_pVTable = *reinterpret_cast<void***>(pDevice);
        if (s_pVTable)
        {
            DWORD oldProtect;
            if (VirtualProtect(&s_pVTable[0], sizeof(void*) * 60, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                s_origReset = reinterpret_cast<Reset9_t>(s_pVTable[16]);
                s_origPresent = reinterpret_cast<Present9_t>(s_pVTable[17]);
                s_origEndScene = reinterpret_cast<EndScene9_t>(s_pVTable[42]);

                s_pVTable[16] = reinterpret_cast<void*>(Hooked_Reset);
                s_pVTable[17] = reinterpret_cast<void*>(Hooked_Present);
                s_pVTable[42] = reinterpret_cast<void*>(Hooked_EndScene);
                VirtualProtect(&s_pVTable[0], sizeof(void*) * 60, oldProtect, &oldProtect);

                s_bHooked = true;
                CrashLogger::LogMessage("[D3DHook] SUCCESS: Hooked IDirect3DDevice9 EndScene (index 42), Reset (index 16), Present (index 17). Ready for F9!");
            }
            else
            {
                CrashLogger::LogMessage("[D3DHook] VirtualProtect failed on vtable.");
            }
        }

        return 0;
    }
}
