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

    void D3D8Hook::Init()
    {
        CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
    }

    void D3D8Hook::Uninstall()
    {
        if (s_bHooked && s_pVTable)
        {
            DWORD oldProtect;
            if (VirtualProtect(&s_pVTable[16], sizeof(void*) * 30, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                if (s_origReset) s_pVTable[16] = reinterpret_cast<void*>(s_origReset);
                if (s_origPresent) s_pVTable[17] = reinterpret_cast<void*>(s_origPresent);
                if (s_origEndScene) s_pVTable[42] = reinterpret_cast<void*>(s_origEndScene);
                VirtualProtect(&s_pVTable[16], sizeof(void*) * 30, oldProtect, &oldProtect);
            }
            s_bHooked = false;
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
        CrashLogger::LogMessage("[D3DHook] Hook thread started. Waiting for RenderWare Direct3D 9 device...");

        // 1. Wait for rwd3d9.dll
        HMODULE hRw = nullptr;
        for (int i = 0; i < 60 && !hRw; i++)
        {
            hRw = GetModuleHandleA("rwd3d9.dll");
            if (!hRw)
                Sleep(100);
        }

        IDirect3DDevice9* pDevice = nullptr;

        if (hRw)
        {
            CrashLogger::LogMessage("[D3DHook] Found rwd3d9.dll. Querying RwD3D9GetCurrentD3DDevice...");
            typedef void* (*RwD3D9GetCurrentD3DDevice_t)(void);
            RwD3D9GetCurrentD3DDevice_t pfnGetDevice =
                reinterpret_cast<RwD3D9GetCurrentD3DDevice_t>(GetProcAddress(hRw, "RwD3D9GetCurrentD3DDevice"));

            if (pfnGetDevice)
            {
                // Wait for game to initialize the device
                for (int i = 0; i < 150 && !pDevice; i++)
                {
                    pDevice = reinterpret_cast<IDirect3DDevice9*>(pfnGetDevice());
                    if (!pDevice)
                        Sleep(100);
                }
            }
        }

        // 2. Fallback to d3d9.dll if rwd3d9 wasn't present
        if (!pDevice)
        {
            CrashLogger::LogMessage("[D3DHook] Waiting for d3d9.dll in process...");
            HMODULE hD3D9 = nullptr;
            for (int i = 0; i < 50 && !hD3D9; i++)
            {
                hD3D9 = GetModuleHandleA("d3d9.dll");
                if (!hD3D9)
                    Sleep(200);
            }

            if (hD3D9)
            {
                typedef IDirect3D9* (WINAPI* Direct3DCreate9_t)(UINT);
                Direct3DCreate9_t pDirect3DCreate9 = reinterpret_cast<Direct3DCreate9_t>(GetProcAddress(hD3D9, "Direct3DCreate9"));
                if (pDirect3DCreate9)
                {
                    IDirect3D9* pD3D = pDirect3DCreate9(D3D_SDK_VERSION);
                    if (pD3D)
                    {
                        HWND hDesktop = GetDesktopWindow();
                        D3DDISPLAYMODE d3ddm;
                        pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
                        D3DPRESENT_PARAMETERS d3dpp = { 0 };
                        d3dpp.Windowed = TRUE;
                        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
                        d3dpp.BackBufferFormat = d3ddm.Format;
                        d3dpp.hDeviceWindow = hDesktop;

                        IDirect3DDevice9* pDummyDevice = nullptr;
                        if (SUCCEEDED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hDesktop,
                            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice)) && pDummyDevice)
                        {
                            pDevice = pDummyDevice;
                        }
                        pD3D->Release();
                    }
                }
            }
        }

        if (!pDevice)
        {
            CrashLogger::LogMessage("[D3DHook] Could not acquire IDirect3DDevice9.");
            return 0;
        }

        // Hook vtable
        s_pVTable = *reinterpret_cast<void***>(pDevice);
        if (s_pVTable)
        {
            s_origReset = reinterpret_cast<Reset9_t>(s_pVTable[16]);
            s_origPresent = reinterpret_cast<Present9_t>(s_pVTable[17]);
            s_origEndScene = reinterpret_cast<EndScene9_t>(s_pVTable[42]);

            DWORD oldProtect;
            if (VirtualProtect(&s_pVTable[16], sizeof(void*) * 30, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                s_pVTable[16] = reinterpret_cast<void*>(Hooked_Reset);
                s_pVTable[17] = reinterpret_cast<void*>(Hooked_Present);
                s_pVTable[42] = reinterpret_cast<void*>(Hooked_EndScene);
                VirtualProtect(&s_pVTable[16], sizeof(void*) * 30, oldProtect, &oldProtect);

                s_bHooked = true;
                CrashLogger::LogMessage("[D3DHook] Successfully hooked IDirect3DDevice9 EndScene (index 42), Reset (index 16), Present (index 17).");
            }
            else
            {
                CrashLogger::LogMessage("[D3DHook] VirtualProtect failed on vtable entries.");
            }
        }

        return 0;
    }
}
