#pragma once

#include "pch.h"

namespace ReLCS
{
    class D3D8Hook
    {
    public:
        static void Init();
        static void Uninstall();

    private:
        typedef HRESULT(STDMETHODCALLTYPE* EndScene9_t)(IDirect3DDevice9* pThis);
        typedef HRESULT(STDMETHODCALLTYPE* Reset9_t)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters);
        typedef HRESULT(STDMETHODCALLTYPE* Present9_t)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

        static HRESULT STDMETHODCALLTYPE Hooked_EndScene(IDirect3DDevice9* pThis);
        static HRESULT STDMETHODCALLTYPE Hooked_Reset(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters);
        static HRESULT STDMETHODCALLTYPE Hooked_Present(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

        static DWORD WINAPI HookThread(LPVOID lpParam);

        static EndScene9_t s_origEndScene;
        static Reset9_t s_origReset;
        static Present9_t s_origPresent;
        static void** s_pVTable;
        static std::atomic<bool> s_bHooked;
    };
}
