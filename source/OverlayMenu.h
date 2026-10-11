#pragma once

#include "pch.h"
#include "CheatManager.h"

namespace ReLCS
{
    struct D3DVertex2D
    {
        float x, y, z, rhw;
        DWORD color;
        float u, v;
    };

    #define D3DFVF_VERTEX2D (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

    class OverlayMenu
    {
    public:
        static void Init();
        static void HandleInput();
        static void OnMenuKeyDown(WPARAM vk);
        static void Render(IDirect3DDevice9* pDevice);
        static void OnLostDevice();
        static void OnResetDevice(IDirect3DDevice9* pDevice);

        static bool IsVisible() { return s_bVisible; }
        static void ToggleVisibility();

        static bool IsFpsCounterEnabled() { return s_bShowFps; }
        static void ToggleFpsCounter() { s_bShowFps = !s_bShowFps; }

        static std::vector<CheatInfo> GetCheatsForCategory(int categoryIndex);
        static void SetWeaponFilter(int filterIndex);
        static int GetWeaponFilter() { return s_weaponFilter; }
        static void CycleWeaponFilter(int direction = 1);
        static void SetVehicleFilter(int filterIndex);
        static int GetVehicleFilter() { return s_vehicleFilter; }
        static void CycleVehicleFilter(int direction = 1);

    private:
        static void CreateFontTexture(IDirect3DDevice9* pDevice);
        static void DrawRect(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color);
        static void DrawTextString(IDirect3DDevice9* pDevice, float x, float y, const std::string& text, DWORD color, float scale = 1.0f);
        static void UpdateFps();

        static bool s_bVisible;
        static bool s_bShowFps;
        static int s_activeCategory;
        static int s_selectedIndex;
        static int s_scrollOffset;
        static float s_currentFps;
        static int s_frameCount;
        static std::chrono::steady_clock::time_point s_lastFpsTime;

        static IDirect3DTexture9* s_pFontTexture;
        static std::vector<std::string> s_categories;
        static int s_weaponFilter;
        static std::vector<std::string> s_weaponFilterNames;
        static int s_vehicleFilter;
        static std::vector<std::string> s_vehicleFilterNames;
    };
}
