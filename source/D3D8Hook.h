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
        typedef HRESULT(STDMETHODCALLTYPE* Present_t)(
            IDirect3DDevice8* pThis,
            CONST RECT* pSourceRect,
            CONST RECT* pDestRect,
            HWND hDestWindowOverride,
            CONST RGNDATA* pDirtyRegion);

        typedef HRESULT(STDMETHODCALLTYPE* Reset_t)(
            IDirect3DDevice8* pThis,
            D3DPRESENT_PARAMETERS* pPresentationParameters);

        static HRESULT STDMETHODCALLTYPE Hooked_Present(
            IDirect3DDevice8* pThis,
            CONST RECT* pSourceRect,
            CONST RECT* pDestRect,
            HWND hDestWindowOverride,
            CONST RGNDATA* pDirtyRegion);

        static HRESULT STDMETHODCALLTYPE Hooked_Reset(
            IDirect3DDevice8* pThis,
            D3DPRESENT_PARAMETERS* pPresentationParameters);

        static DWORD WINAPI HookThread(LPVOID lpParam);

        static Present_t s_origPresent;
        static Reset_t s_origReset;
        static void** s_pVTable;
        static std::atomic<bool> s_bHooked;
    };
}
