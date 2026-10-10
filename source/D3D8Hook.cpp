#include "D3D8Hook.h"
#include "OverlayMenu.h"
#include "CrashLogger.h"

namespace ReLCS
{
    D3D8Hook::Present_t D3D8Hook::s_origPresent = nullptr;
    D3D8Hook::Reset_t D3D8Hook::s_origReset = nullptr;
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
            if (VirtualProtect(&s_pVTable[14], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                if (s_origReset) s_pVTable[14] = reinterpret_cast<void*>(s_origReset);
                if (s_origPresent) s_pVTable[15] = reinterpret_cast<void*>(s_origPresent);
                VirtualProtect(&s_pVTable[14], sizeof(void*) * 2, oldProtect, &oldProtect);
            }
            s_bHooked = false;
        }
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_Present(
        IDirect3DDevice8* pThis,
        CONST RECT* pSourceRect,
        CONST RECT* pDestRect,
        HWND hDestWindowOverride,
        CONST RGNDATA* pDirtyRegion)
    {
        // Render menu overlay onto backbuffer
        __try
        {
            OverlayMenu::Render(pThis);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Fail safely to prevent overlay rendering crash from killing the game
        }

        if (s_origPresent)
            return s_origPresent(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

        return D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_Reset(
        IDirect3DDevice8* pThis,
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
        // Wait a short moment for game window and modules to initialize
        Sleep(800);

        HMODULE hD3D8 = nullptr;
        int attempts = 0;
        while (!hD3D8 && attempts < 50)
        {
            hD3D8 = GetModuleHandleA("d3d8.dll");
            if (!hD3D8)
            {
                Sleep(200);
                attempts++;
            }
        }

        if (!hD3D8)
        {
            CrashLogger::LogMessage("[D3D8Hook] d3d8.dll not found in process after waiting.");
            return 0;
        }

        typedef IDirect3D8* (WINAPI* Direct3DCreate8_t)(UINT);
        Direct3DCreate8_t pDirect3DCreate8 = reinterpret_cast<Direct3DCreate8_t>(GetProcAddress(hD3D8, "Direct3DCreate8"));
        if (!pDirect3DCreate8)
        {
            CrashLogger::LogMessage("[D3D8Hook] Failed to get Direct3DCreate8 proc address.");
            return 0;
        }

        IDirect3D8* pD3D = pDirect3DCreate8(D3D_SDK_VERSION);
        if (!pD3D)
        {
            CrashLogger::LogMessage("[D3D8Hook] Direct3DCreate8 returned null.");
            return 0;
        }

        // Register dummy window class
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "ReLCS_DummyD3D";
        RegisterClassA(&wc);

        HWND hDummyWnd = CreateWindowA("ReLCS_DummyD3D", "Dummy", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);
        if (!hDummyWnd)
        {
            pD3D->Release();
            CrashLogger::LogMessage("[D3D8Hook] Failed to create dummy window.");
            return 0;
        }

        D3DDISPLAYMODE d3ddm;
        if (FAILED(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
        {
            d3ddm.Width = 800;
            d3ddm.Height = 600;
            d3ddm.RefreshRate = 0;
            d3ddm.Format = D3DFMT_X8R8G8B8;
        }

        D3DPRESENT_PARAMETERS d3dpp = { 0 };
        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.BackBufferFormat = d3ddm.Format;
        d3dpp.hDeviceWindow = hDummyWnd;

        IDirect3DDevice8* pDummyDevice = nullptr;
        HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hDummyWnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

        if (FAILED(hr))
        {
            hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hDummyWnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
        }

        if (FAILED(hr))
        {
            hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hDummyWnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
        }

        if (FAILED(hr) || !pDummyDevice)
        {
            DestroyWindow(hDummyWnd);
            UnregisterClassA("ReLCS_DummyD3D", wc.hInstance);
            pD3D->Release();
            CrashLogger::LogMessage("[D3D8Hook] Failed to create dummy IDirect3DDevice8.");
            return 0;
        }

        s_pVTable = *reinterpret_cast<void***>(pDummyDevice);
        if (s_pVTable)
        {
            s_origReset = reinterpret_cast<Reset_t>(s_pVTable[14]);
            s_origPresent = reinterpret_cast<Present_t>(s_pVTable[15]);

            DWORD oldProtect;
            if (VirtualProtect(&s_pVTable[14], sizeof(void*) * 2, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                s_pVTable[14] = reinterpret_cast<void*>(Hooked_Reset);
                s_pVTable[15] = reinterpret_cast<void*>(Hooked_Present);
                VirtualProtect(&s_pVTable[14], sizeof(void*) * 2, oldProtect, &oldProtect);

                s_bHooked = true;
                CrashLogger::LogMessage("[D3D8Hook] Successfully hooked IDirect3DDevice8 Present (index 15) and Reset (index 14).");
            }
            else
            {
                CrashLogger::LogMessage("[D3D8Hook] VirtualProtect failed on vtable entries.");
            }
        }

        // Cleanup dummy resources
        pDummyDevice->Release();
        DestroyWindow(hDummyWnd);
        UnregisterClassA("ReLCS_DummyD3D", wc.hInstance);
        pD3D->Release();

        return 0;
    }
}
