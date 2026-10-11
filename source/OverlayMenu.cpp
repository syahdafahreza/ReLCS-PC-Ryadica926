#include "OverlayMenu.h"
#include "Font8x8.h"
#include "CrashLogger.h"

namespace ReLCS
{
    bool OverlayMenu::s_bVisible = false;
    bool OverlayMenu::s_bShowFps = false;
    int OverlayMenu::s_activeCategory = 0;
    int OverlayMenu::s_selectedIndex = 0;
    int OverlayMenu::s_scrollOffset = 0;
    float OverlayMenu::s_currentFps = 0.0f;
    int OverlayMenu::s_frameCount = 0;
    std::chrono::steady_clock::time_point OverlayMenu::s_lastFpsTime = std::chrono::steady_clock::now();

    IDirect3DTexture9* OverlayMenu::s_pFontTexture = nullptr;
    std::vector<std::string> OverlayMenu::s_categories = { "PLAYER", "WEAPON", "VEHICLE", "WORLD", "VISUAL" };
    int OverlayMenu::s_weaponFilter = 0;
    std::vector<std::string> OverlayMenu::s_weaponFilterNames = {
        "ALL", "MELEE", "HANDGUN", "SHOTGUN", "SMG/RIF", "HEAVY"
    };
    int OverlayMenu::s_vehicleFilter = 0;
    std::vector<std::string> OverlayMenu::s_vehicleFilterNames = {
        "ALL", "CARS", "BIKES", "BOATS", "PLANES", "CHEATS"
    };

    void OverlayMenu::Init()
    {
        s_bVisible = false;
        s_bShowFps = false;
        s_activeCategory = 0;
        s_selectedIndex = 0;
        s_scrollOffset = 0;
        s_weaponFilter = 0;
        s_vehicleFilter = 0;
        s_lastFpsTime = std::chrono::steady_clock::now();
    }

    void OverlayMenu::ToggleVisibility()
    {
        s_bVisible = !s_bVisible;
        CrashLogger::LogMessage(std::string("[OverlayMenu] Menu visibility toggled to: ") + (s_bVisible ? "SHOWN" : "HIDDEN"));
        if (s_bVisible)
        {
            ClipCursor(nullptr);
            while (ShowCursor(TRUE) < 0);
        }
        else
        {
            while (ShowCursor(FALSE) >= 0);
        }
    }

    enum WeaponSubCategory
    {
        WEAP_SUB_ALL = 0,
        WEAP_SUB_MELEE = 1,
        WEAP_SUB_HANDGUN = 2,
        WEAP_SUB_SHOTGUN = 3,
        WEAP_SUB_SMG_RIFLE = 4,
        WEAP_SUB_HEAVY_EXP = 5
    };

    static WeaponSubCategory ClassifyWeapon(const CheatInfo& c)
    {
        int id = c.weaponTypeId;
        if (id >= 1 && id <= 11)
            return WEAP_SUB_MELEE;
        if (id == 17 || id == 18)
            return WEAP_SUB_HANDGUN;
        if (id >= 19 && id <= 21)
            return WEAP_SUB_SHOTGUN;
        if ((id >= 22 && id <= 25) || id == 26 || id == 27)
            return WEAP_SUB_SMG_RIFLE;
        return WEAP_SUB_HEAVY_EXP;
    }

    void OverlayMenu::SetWeaponFilter(int filterIndex)
    {
        if (filterIndex >= 0 && filterIndex < static_cast<int>(s_weaponFilterNames.size()))
        {
            s_weaponFilter = filterIndex;
            s_selectedIndex = 0;
            s_scrollOffset = 0;
        }
    }

    void OverlayMenu::CycleWeaponFilter(int direction)
    {
        int count = static_cast<int>(s_weaponFilterNames.size());
        s_weaponFilter = (s_weaponFilter + direction + count) % count;
        s_selectedIndex = 0;
        s_scrollOffset = 0;
    }

    enum VehicleSubCategory
    {
        VEH_SUB_ALL = 0,
        VEH_SUB_CARS = 1,
        VEH_SUB_BIKES = 2,
        VEH_SUB_BOATS = 3,
        VEH_SUB_AIRCRAFT = 4,
        VEH_SUB_CHEATS = 5
    };

    static VehicleSubCategory ClassifyVehicle(const CheatInfo& c)
    {
        if (c.vehicleModelId <= 0)
            return VEH_SUB_CHEATS;

        int id = c.vehicleModelId;

        // Bikes & Motorcycles (10 models)
        if (id == 202 || id == 203 || id == 204 || id == 205 || id == 206 ||
            id == 207 || id == 208 || id == 209 || id == 210 || id == 225)
        {
            return VEH_SUB_BIKES;
        }

        // Boats, Ships, Ferries (5 models)
        if (id == 192 || id == 193 || id == 194 || id == 195 || id == 196)
        {
            return VEH_SUB_BOATS;
        }

        // Planes & Helicopters (12 models)
        if (id == 164 || id == 198 || id == 199 || id == 200 || id == 201 ||
            id == 211 || id == 212 || id == 213 || id == 214 || id == 215 ||
            id == 216 || id == 224)
        {
            return VEH_SUB_AIRCRAFT;
        }

        // All other models are Cars / Vans / Trucks / SUVs / Emergency / Tanks (79 models)
        return VEH_SUB_CARS;
    }

    void OverlayMenu::SetVehicleFilter(int filterIndex)
    {
        if (filterIndex >= 0 && filterIndex < static_cast<int>(s_vehicleFilterNames.size()))
        {
            s_vehicleFilter = filterIndex;
            s_selectedIndex = 0;
            s_scrollOffset = 0;
        }
    }

    void OverlayMenu::CycleVehicleFilter(int direction)
    {
        int count = static_cast<int>(s_vehicleFilterNames.size());
        s_vehicleFilter = (s_vehicleFilter + direction + count) % count;
        s_selectedIndex = 0;
        s_scrollOffset = 0;
    }

    std::vector<CheatInfo> OverlayMenu::GetCheatsForCategory(int categoryIndex)
    {
        std::vector<CheatInfo> result;
        const char* targetCat = "";
        switch (categoryIndex)
        {
        case 0: targetCat = "Player"; break;
        case 1: targetCat = "Weapon"; break;
        case 2: targetCat = "Vehicle"; break;
        case 3: targetCat = "World"; break;
        case 4: targetCat = "Visual/Gameplay"; break;
        default: break;
        }

        for (const auto& cheat : CheatManager::GetCheats())
        {
            if (cheat.category == targetCat)
            {
                if (categoryIndex == 1 && s_weaponFilter != WEAP_SUB_ALL)
                {
                    WeaponSubCategory sub = ClassifyWeapon(cheat);
                    if (sub != static_cast<WeaponSubCategory>(s_weaponFilter))
                        continue;
                }
                else if (categoryIndex == 2 && s_vehicleFilter != VEH_SUB_ALL)
                {
                    VehicleSubCategory sub = ClassifyVehicle(cheat);
                    if (sub != static_cast<VehicleSubCategory>(s_vehicleFilter))
                        continue;
                }
                result.push_back(cheat);
            }
        }
        return result;
    }

    void OverlayMenu::CreateFontTexture(IDirect3DDevice9* pDevice)
    {
        if (s_pFontTexture || !pDevice)
            return;

        // 128x64 texture fits 16 columns x 6 rows = 96 glyphs (8x8 each)
        if (FAILED(pDevice->CreateTexture(128, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &s_pFontTexture, nullptr)))
            return;

        D3DLOCKED_RECT lr;
        if (FAILED(s_pFontTexture->LockRect(0, &lr, nullptr, 0)))
            return;

        uint32_t* pBits = reinterpret_cast<uint32_t*>(lr.pBits);
        int pitchElements = lr.Pitch / 4;

        // Clear texture
        for (int y = 0; y < 64; y++)
        {
            for (int x = 0; x < 128; x++)
            {
                pBits[y * pitchElements + x] = 0x00000000;
            }
        }

        // Draw 96 glyphs with MSB (left pixel) to LSB (right pixel)
        for (int i = 0; i < 96; i++)
        {
            int col = i % 16;
            int row = i / 16;
            int startX = col * 8;
            int startY = row * 8;

            for (int py = 0; py < 8; py++)
            {
                uint8_t byteRow = g_font8x8_basic[i][py];
                for (int px = 0; px < 8; px++)
                {
                    // Bit 7 is leftmost pixel (px = 0), Bit 0 is rightmost pixel (px = 7)
                    if ((byteRow >> (7 - px)) & 1)
                    {
                        pBits[(startY + py) * pitchElements + (startX + px)] = 0xFFFFFFFF;
                    }
                }
            }
        }

        s_pFontTexture->UnlockRect(0);
    }

    void OverlayMenu::OnLostDevice()
    {
        if (s_pFontTexture)
        {
            s_pFontTexture->Release();
            s_pFontTexture = nullptr;
        }
    }

    void OverlayMenu::OnResetDevice(IDirect3DDevice9* pDevice)
    {
        CreateFontTexture(pDevice);
    }

    void OverlayMenu::OnMenuKeyDown(WPARAM vk)
    {
        if (!s_bVisible)
            return;

        if (s_activeCategory == 1) // WEAPON category
        {
            if (vk == VK_TAB)
            {
                CycleWeaponFilter((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                return;
            }
            else if (vk == 'Q' || vk == 'q' || vk == VK_OEM_4)
            {
                CycleWeaponFilter(-1);
                return;
            }
            else if (vk == 'E' || vk == 'e' || vk == VK_OEM_6)
            {
                CycleWeaponFilter(1);
                return;
            }
            else if (vk == '1' || vk == VK_NUMPAD1) { SetWeaponFilter(0); return; }
            else if (vk == '2' || vk == VK_NUMPAD2) { SetWeaponFilter(1); return; }
            else if (vk == '3' || vk == VK_NUMPAD3) { SetWeaponFilter(2); return; }
            else if (vk == '4' || vk == VK_NUMPAD4) { SetWeaponFilter(3); return; }
            else if (vk == '5' || vk == VK_NUMPAD5) { SetWeaponFilter(4); return; }
            else if (vk == '6' || vk == VK_NUMPAD6) { SetWeaponFilter(5); return; }
        }
        else if (s_activeCategory == 2) // VEHICLE category
        {
            if (vk == VK_TAB)
            {
                CycleVehicleFilter((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                return;
            }
            else if (vk == 'Q' || vk == 'q' || vk == VK_OEM_4)
            {
                CycleVehicleFilter(-1);
                return;
            }
            else if (vk == 'E' || vk == 'e' || vk == VK_OEM_6)
            {
                CycleVehicleFilter(1);
                return;
            }
            else if (vk == '1' || vk == VK_NUMPAD1) { SetVehicleFilter(0); return; }
            else if (vk == '2' || vk == VK_NUMPAD2) { SetVehicleFilter(1); return; }
            else if (vk == '3' || vk == VK_NUMPAD3) { SetVehicleFilter(2); return; }
            else if (vk == '4' || vk == VK_NUMPAD4) { SetVehicleFilter(3); return; }
            else if (vk == '5' || vk == VK_NUMPAD5) { SetVehicleFilter(4); return; }
            else if (vk == '6' || vk == VK_NUMPAD6) { SetVehicleFilter(5); return; }
        }

        std::vector<CheatInfo> currentCategoryCheats = GetCheatsForCategory(s_activeCategory);
        int totalItems = static_cast<int>(currentCategoryCheats.size());
        if (s_activeCategory == 4)
            totalItems += 1; // FPS counter toggle

        const int MAX_VISIBLE_ROWS = (s_activeCategory == 1 || s_activeCategory == 2) ? 11 : 12;

        if (vk == VK_LEFT)
        {
            s_activeCategory = (s_activeCategory - 1 + 5) % 5;
            s_selectedIndex = 0;
            s_scrollOffset = 0;
        }
        else if (vk == VK_RIGHT)
        {
            s_activeCategory = (s_activeCategory + 1) % 5;
            s_selectedIndex = 0;
            s_scrollOffset = 0;
        }
        else if (vk == VK_UP)
        {
            s_selectedIndex--;
            if (s_selectedIndex < 0)
            {
                s_selectedIndex = (totalItems > 0 ? totalItems - 1 : 0);
                s_scrollOffset = (std::max)(0, totalItems - MAX_VISIBLE_ROWS);
            }
            else if (s_selectedIndex < s_scrollOffset)
            {
                s_scrollOffset = s_selectedIndex;
            }
        }
        else if (vk == VK_DOWN)
        {
            s_selectedIndex++;
            if (s_selectedIndex >= totalItems)
            {
                s_selectedIndex = 0;
                s_scrollOffset = 0;
            }
            else if (s_selectedIndex >= s_scrollOffset + MAX_VISIBLE_ROWS)
            {
                s_scrollOffset = s_selectedIndex - MAX_VISIBLE_ROWS + 1;
            }
        }
        else if (vk == VK_RETURN)
        {
            if (s_activeCategory == 4 && s_selectedIndex == totalItems - 1)
            {
                s_bShowFps = !s_bShowFps;
            }
            else if (s_selectedIndex < static_cast<int>(currentCategoryCheats.size()))
            {
                CheatManager::ActivateCheat(currentCategoryCheats[s_selectedIndex].id);
            }
        }
    }

    void OverlayMenu::DrawRect(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color)
    {
        D3DVertex2D vertices[4] = {
            { x,     y,     0.0f, 1.0f, color, 0.0f, 0.0f },
            { x + w, y,     0.0f, 1.0f, color, 0.0f, 0.0f },
            { x,     y + h, 0.0f, 1.0f, color, 0.0f, 0.0f },
            { x + w, y + h, 0.0f, 1.0f, color, 0.0f, 0.0f },
        };

        pDevice->SetTexture(0, nullptr);
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

        pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(D3DVertex2D));
    }

    void OverlayMenu::DrawTextString(IDirect3DDevice9* pDevice, float x, float y, const std::string& text, DWORD color, float scale)
    {
        if (!s_pFontTexture)
            CreateFontTexture(pDevice);
        if (!s_pFontTexture)
            return;

        pDevice->SetTexture(0, s_pFontTexture);
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

        float curX = x;
        float charW = 8.0f * scale;
        float charH = 8.0f * scale;

        for (char c : text)
        {
            if (c < 32 || c > 127)
                c = '?';

            int idx = c - 32;
            int col = idx % 16;
            int row = idx / 16;

            float u0 = (col * 8.0f) / 128.0f;
            float v0 = (row * 8.0f) / 64.0f;
            float u1 = ((col + 1) * 8.0f) / 128.0f;
            float v1 = ((row + 1) * 8.0f) / 64.0f;

            D3DVertex2D verts[4] = {
                { curX,         y,         0.0f, 1.0f, color, u0, v0 },
                { curX + charW, y,         0.0f, 1.0f, color, u1, v0 },
                { curX,         y + charH, 0.0f, 1.0f, color, u0, v1 },
                { curX + charW, y + charH, 0.0f, 1.0f, color, u1, v1 },
            };

            pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(D3DVertex2D));
            curX += charW + (1.0f * scale);
        }
    }

    void OverlayMenu::UpdateFps()
    {
        s_frameCount++;
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastFpsTime).count();
        if (ms >= 500)
        {
            s_currentFps = (s_frameCount * 1000.0f) / ms;
            s_frameCount = 0;
            s_lastFpsTime = now;
        }
    }

    void OverlayMenu::HandleInput()
    {
        static bool s_lastF9 = false;
        static bool s_lastF10 = false;
        static bool s_lastUp = false;
        static bool s_lastDown = false;
        static bool s_lastLeft = false;
        static bool s_lastRight = false;
        static bool s_lastEnter = false;
        static bool s_lastEsc = false;
        static bool s_lastTab = false;
        static bool s_lastQ = false;
        static bool s_lastE = false;
        static bool s_lastNum[6] = { false, false, false, false, false, false };
        static bool s_lastLButton = false;
        static auto s_lastToggleTime = std::chrono::steady_clock::now();

        auto now = std::chrono::steady_clock::now();
        int64_t msSinceToggle = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastToggleTime).count();

        bool currF9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        bool currF10 = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;

        // Toggle visibility only on fresh key down with minimum 250ms cooldown to avoid accidental double toggles
        if (currF9 && !s_lastF9 && msSinceToggle > 250)
        {
            ToggleVisibility();
            s_lastToggleTime = now;
        }
        s_lastF9 = currF9;

        if (currF10 && !s_lastF10)
        {
            ToggleFpsCounter();
        }
        s_lastF10 = currF10;

        if (!s_bVisible)
            return;

        bool currUp = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
        bool currDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
        bool currLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
        bool currRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
        bool currEnter = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
        bool currEsc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        bool currTab = (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;
        bool currQ = (GetAsyncKeyState('Q') & 0x8000) != 0 || (GetAsyncKeyState(VK_OEM_4) & 0x8000) != 0;
        bool currE = (GetAsyncKeyState('E') & 0x8000) != 0 || (GetAsyncKeyState(VK_OEM_6) & 0x8000) != 0;

        if (currUp && !s_lastUp) OnMenuKeyDown(VK_UP);
        if (currDown && !s_lastDown) OnMenuKeyDown(VK_DOWN);
        if (currLeft && !s_lastLeft) OnMenuKeyDown(VK_LEFT);
        if (currRight && !s_lastRight) OnMenuKeyDown(VK_RIGHT);
        if (currEnter && !s_lastEnter) OnMenuKeyDown(VK_RETURN);
        if (currEsc && !s_lastEsc && msSinceToggle > 250)
        {
            ToggleVisibility();
            s_lastToggleTime = now;
        }

        if (s_activeCategory == 1 || s_activeCategory == 2)
        {
            if (currTab && !s_lastTab) OnMenuKeyDown(VK_TAB);
            if (currQ && !s_lastQ) OnMenuKeyDown('Q');
            if (currE && !s_lastE) OnMenuKeyDown('E');

            int numKeys[6] = { '1', '2', '3', '4', '5', '6' };
            int numpadKeys[6] = { VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6 };
            for (int k = 0; k < 6; k++)
            {
                bool currNum = ((GetAsyncKeyState(numKeys[k]) & 0x8000) != 0) || ((GetAsyncKeyState(numpadKeys[k]) & 0x8000) != 0);
                if (currNum && !s_lastNum[k])
                {
                    OnMenuKeyDown(numKeys[k]);
                }
                s_lastNum[k] = currNum;
            }
        }

        // Mouse click navigation support
        bool currLButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (currLButton && !s_lastLButton)
        {
            POINT pt;
            if (GetCursorPos(&pt))
            {
                HWND hWnd = GetForegroundWindow();
                if (hWnd)
                {
                    ScreenToClient(hWnd, &pt);
                    float mx = static_cast<float>(pt.x);
                    float my = static_cast<float>(pt.y);

                    const float menuX = 35.0f;
                    const float menuY = 45.0f;
                    const float menuW = 460.0f;

                    // Main Category Tabs (5 tabs)
                    float tabY = menuY + 40.0f;
                    float tabW = menuW / 5.0f;
                    if (my >= tabY && my <= tabY + 24.0f && mx >= menuX && mx <= menuX + menuW)
                    {
                        int clickedCat = static_cast<int>((mx - menuX) / tabW);
                        if (clickedCat >= 0 && clickedCat < 5)
                        {
                            s_activeCategory = clickedCat;
                            s_selectedIndex = 0;
                            s_scrollOffset = 0;
                        }
                    }

                    // Subcategory Filter Pills (when in Weapon or Vehicle tab)
                    if (s_activeCategory == 1 || s_activeCategory == 2)
                    {
                        float filterY = tabY + 28.0f;
                        float fW = (menuW - 16.0f) / 6.0f;
                        if (my >= filterY && my <= filterY + 22.0f && mx >= menuX + 8.0f && mx <= menuX + menuW - 8.0f)
                        {
                            int clickedFilter = static_cast<int>((mx - (menuX + 8.0f)) / fW);
                            if (clickedFilter >= 0 && clickedFilter < 6)
                            {
                                if (s_activeCategory == 1)
                                    SetWeaponFilter(clickedFilter);
                                else
                                    SetVehicleFilter(clickedFilter);
                            }
                        }
                    }
                }
            }
        }
        s_lastLButton = currLButton;

        s_lastUp = currUp;
        s_lastDown = currDown;
        s_lastLeft = currLeft;
        s_lastRight = currRight;
        s_lastEnter = currEnter;
        s_lastEsc = currEsc;
        s_lastTab = currTab;
        s_lastQ = currQ;
        s_lastE = currE;
    }

    void OverlayMenu::Render(IDirect3DDevice9* pDevice)
    {
        if (!pDevice)
            return;

        UpdateFps();
        HandleInput();
        CheatManager::Update();

        // Capture full state block if supported to preserve all RenderStates and Shaders
        IDirect3DStateBlock9* pStateBlock = nullptr;
        if (SUCCEEDED(pDevice->CreateStateBlock(D3DSBT_ALL, &pStateBlock)) && pStateBlock)
        {
            pStateBlock->Capture();
        }

        // Configure rendering state for 2D UI
        pDevice->SetVertexShader(nullptr);
        pDevice->SetPixelShader(nullptr);
        pDevice->SetFVF(D3DFVF_VERTEX2D);

        pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        pDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);

        // 1. Draw FPS Counter (if enabled)
        if (s_bShowFps)
        {
            char fpsBuf[32];
            snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", s_currentFps);
            DrawRect(pDevice, 10.0f, 10.0f, 90.0f, 22.0f, 0xAA000000);
            DrawTextString(pDevice, 16.0f, 15.0f, fpsBuf, 0xFF10B981, 1.2f);
        }

        // 2. Draw Menu Window (if visible)
        if (s_bVisible)
        {
            const float menuX = 35.0f;
            const float menuY = 45.0f;
            const float menuW = 460.0f;
            const float menuH = 430.0f;

            // Outer border and main panel
            DrawRect(pDevice, menuX - 2.0f, menuY - 2.0f, menuW + 4.0f, menuH + 4.0f, 0xFF3B82F6);
            DrawRect(pDevice, menuX, menuY, menuW, menuH, 0xEE0F172A);

            // Header bar
            DrawRect(pDevice, menuX, menuY, menuW, 36.0f, 0xFF1E293B);
            DrawTextString(pDevice, menuX + 16.0f, menuY + 11.0f, "ReLCS-Cleo v1.0", 0xFF60A5FA, 1.5f);
            DrawTextString(pDevice, menuX + menuW - 130.0f, menuY + 13.0f, "[F9] HIDE", 0xFF94A3B8, 1.1f);

            // Category Tab Bar (5 tabs)
            float tabY = menuY + 40.0f;
            float tabW = menuW / 5.0f;
            for (int i = 0; i < 5; i++)
            {
                float tX = menuX + (i * tabW);
                bool isActive = (i == s_activeCategory);
                DWORD tabBg = isActive ? 0xFF2563EB : 0xFF1E293B;
                DWORD tabTextColor = isActive ? 0xFFFFFFFF : 0xFF94A3B8;

                DrawRect(pDevice, tX + 2.0f, tabY, tabW - 4.0f, 24.0f, tabBg);
                DrawTextString(pDevice, tX + 6.0f, tabY + 7.0f, s_categories[i], tabTextColor, 1.0f);
            }

            // Subcategory Filter Pills (Live filter bar when WEAPON or VEHICLE tab is active)
            float filterY = tabY + 28.0f;
            if (s_activeCategory == 1 || s_activeCategory == 2)
            {
                float fW = (menuW - 16.0f) / 6.0f;
                const auto& filterNames = (s_activeCategory == 1) ? s_weaponFilterNames : s_vehicleFilterNames;
                int currentFilter = (s_activeCategory == 1) ? s_weaponFilter : s_vehicleFilter;

                for (int j = 0; j < 6; j++)
                {
                    float fX = menuX + 8.0f + (j * fW);
                    bool isFilterActive = (j == currentFilter);
                    DWORD pBg = isFilterActive ? 0xFF0284C7 : 0xFF1E293B;
                    DWORD pTextColor = isFilterActive ? 0xFFFFFFFF : 0xFF94A3B8;

                    DrawRect(pDevice, fX + 1.0f, filterY, fW - 2.0f, 20.0f, pBg);

                    float textW = filterNames[j].length() * 8.0f * 0.95f;
                    float textX = fX + (fW - textW) / 2.0f;
                    DrawTextString(pDevice, textX, filterY + 5.0f, filterNames[j], pTextColor, 0.95f);
                }
            }

            // Items list for currently active category
            std::vector<CheatInfo> curCheats = GetCheatsForCategory(s_activeCategory);

            int totalRows = static_cast<int>(curCheats.size());
            if (s_activeCategory == 4)
                totalRows += 1; // FPS counter item

            const int MAX_VISIBLE_ROWS = (s_activeCategory == 1 || s_activeCategory == 2) ? 11 : 12;
            if (s_scrollOffset < 0) s_scrollOffset = 0;
            if (s_scrollOffset >= totalRows && totalRows > 0) s_scrollOffset = totalRows - 1;

            int firstVisible = s_scrollOffset;
            int countToDraw = (std::min)(MAX_VISIBLE_ROWS, totalRows - firstVisible);
            if (countToDraw < 0) countToDraw = 0;

            float itemStartY = (s_activeCategory == 1 || s_activeCategory == 2) ? (filterY + 25.0f) : (tabY + 34.0f);
            float rowH = 26.0f;

            for (int k = 0; k < countToDraw; k++)
            {
                int i = firstVisible + k;
                float rowY = itemStartY + (k * rowH);
                bool isSelected = (i == s_selectedIndex);

                if (isSelected)
                {
                    DrawRect(pDevice, menuX + 8.0f, rowY - 2.0f, menuW - 16.0f, rowH - 2.0f, 0xFF1D4ED8);
                }
                else if (i % 2 == 1)
                {
                    DrawRect(pDevice, menuX + 8.0f, rowY - 2.0f, menuW - 16.0f, rowH - 2.0f, 0x22FFFFFF);
                }

                if (s_activeCategory == 4 && i == totalRows - 1)
                {
                    // FPS counter item
                    DrawTextString(pDevice, menuX + 20.0f, rowY + 3.0f, "Toggle FPS Counter", isSelected ? 0xFFFFFFFF : 0xFFE2E8F0, 1.2f);
                    std::string stateTag = s_bShowFps ? "[ON]" : "[OFF]";
                    DrawTextString(pDevice, menuX + menuW - 75.0f, rowY + 3.0f, stateTag, s_bShowFps ? 0xFF10B981 : 0xFFEF4444, 1.2f);
                }
                else
                {
                    const auto& item = curCheats[i];
                    DrawTextString(pDevice, menuX + 20.0f, rowY + 3.0f, item.name, isSelected ? 0xFFFFFFFF : 0xFFE2E8F0, 1.2f);

                    // Status / Code tag
                    std::string tag = "";
                    DWORD tagColor = 0xFF60A5FA;

                    if (item.weaponTypeId > 0)
                    {
                        int slot = -1;
                        int wid = item.weaponTypeId;
                        if (wid == 1) slot = 0;
                        else if (wid >= 2 && wid <= 11) slot = 1;
                        else if (wid >= 12 && wid <= 15) slot = 2;
                        else if (wid == 17 || wid == 18) slot = 3;
                        else if (wid >= 19 && wid <= 21) slot = 4;
                        else if (wid >= 22 && wid <= 25) slot = 5;
                        else if (wid == 26 || wid == 27) slot = 6;
                        else if (wid >= 30 && wid <= 33) slot = 7;
                        else if (wid == 28 || wid == 29) slot = 8;
                        else if (wid == 34 || wid == 36) slot = 9;

                        if (slot >= 0)
                        {
                            char slotBuf[24];
                            snprintf(slotBuf, sizeof(slotBuf), "[SLOT %d]", slot);
                            tag = slotBuf;
                            tagColor = 0xFF38BDF8;
                        }
                    }
                    else if (item.id == LCSCheat::NeverWanted)
                    {
                        tag = CheatManager::IsNeverWantedEnabled() ? "[ACTIVE]" : "[OFF]";
                        tagColor = CheatManager::IsNeverWantedEnabled() ? 0xFF10B981 : 0xFF94A3B8;
                    }
                    else if (item.id == LCSCheat::UnlimitedAmmo)
                    {
                        tag = CheatManager::IsUnlimitedAmmoEnabled() ? "[ACTIVE]" : "[OFF]";
                        tagColor = CheatManager::IsUnlimitedAmmoEnabled() ? 0xFF10B981 : 0xFF94A3B8;
                    }
                    else if (item.id == LCSCheat::InfiniteHealth)
                    {
                        tag = CheatManager::IsInfiniteHealthEnabled() ? "[ACTIVE]" : "[OFF]";
                        tagColor = CheatManager::IsInfiniteHealthEnabled() ? 0xFF10B981 : 0xFF94A3B8;
                    }
                    else if (item.id == LCSCheat::InfiniteArmor)
                    {
                        tag = CheatManager::IsInfiniteArmorEnabled() ? "[ACTIVE]" : "[OFF]";
                        tagColor = CheatManager::IsInfiniteArmorEnabled() ? 0xFF10B981 : 0xFF94A3B8;
                    }
                    else if (item.id == LCSCheat::UpsideDownMode)
                    {
                        tag = CheatManager::IsUpsideDownEnabled() ? "[ACTIVE]" : "";
                        tagColor = 0xFFF59E0B;
                    }

                    if (!tag.empty())
                    {
                        DrawTextString(pDevice, menuX + menuW - 85.0f, rowY + 3.0f, tag, tagColor, 1.1f);
                    }
                }
            }

            // Draw scrollbar if content exceeds MAX_VISIBLE_ROWS
            if (totalRows > MAX_VISIBLE_ROWS)
            {
                float scrollTrackX = menuX + menuW - 7.0f;
                float scrollTrackY = itemStartY - 2.0f;
                float scrollTrackH = (MAX_VISIBLE_ROWS * rowH);
                DrawRect(pDevice, scrollTrackX, scrollTrackY, 3.0f, scrollTrackH, 0x44FFFFFF);

                float thumbH = ((float)MAX_VISIBLE_ROWS / (float)totalRows) * scrollTrackH;
                if (thumbH < 14.0f) thumbH = 14.0f;
                float scrollProgress = (float)s_scrollOffset / (float)(totalRows - MAX_VISIBLE_ROWS);
                float thumbY = scrollTrackY + scrollProgress * (scrollTrackH - thumbH);
                DrawRect(pDevice, scrollTrackX, thumbY, 3.0f, thumbH, 0xFF3B82F6);
            }

            // Footer instructions bar
            float footerY = menuY + menuH - 30.0f;
            DrawRect(pDevice, menuX, footerY, menuW, 30.0f, 0xFF0F172A);

            if (s_activeCategory == 1)
            {
                DrawTextString(pDevice, menuX + 14.0f, footerY + 8.0f,
                    "[TAB/Q/E/1-6] Filter  [UP/DN] Move  [ENTER] Equip", 0xFF94A3B8, 0.95f);
            }
            else if (s_activeCategory == 2)
            {
                DrawTextString(pDevice, menuX + 14.0f, footerY + 8.0f,
                    "[TAB/Q/E/1-6] Filter  [UP/DN] Move  [ENTER] Spawn", 0xFF94A3B8, 0.95f);
            }
            else
            {
                DrawTextString(pDevice, menuX + 16.0f, footerY + 8.0f,
                    "[UP/DN] Move  [ENTER] Apply  [F9] Close", 0xFF94A3B8, 1.05f);
            }

            char counterBuf[32];
            snprintf(counterBuf, sizeof(counterBuf), "[%d/%d]", (totalRows > 0 ? s_selectedIndex + 1 : 0), totalRows);
            DrawTextString(pDevice, menuX + menuW - 70.0f, footerY + 8.0f, counterBuf, 0xFF60A5FA, 1.05f);
        }

        // 3. Draw Cheat Activation Toast Banner (on top of menu so it never clips)
        float notifAlpha = CheatManager::GetNotificationAlpha();
        if (notifAlpha > 0.01f)
        {
            DWORD a = static_cast<DWORD>(notifAlpha * 255.0f);
            DWORD bannerBg = (a << 24) | 0x001E293B; // Slate background
            DWORD bannerText = (a << 24) | 0x00F8FAFC;
            DWORD bannerAccent = (a << 24) | 0x003B82F6; // Blue border

            std::string notifStr;
            if (CheatManager::GetLastActivatedCategory() == "Weapon")
                notifStr = "[ WEAPON EQUIPPED: " + CheatManager::GetLastActivatedCheatName() + " ]";
            else if (CheatManager::GetLastActivatedCategory() == "Vehicle")
                notifStr = "[ VEHICLE SPAWNED: " + CheatManager::GetLastActivatedCheatName() + " ]";
            else
                notifStr = "[ CHEAT ACTIVATED: " + CheatManager::GetLastActivatedCheatName() + " ]";
            float bannerW = (notifStr.length() * 10.0f) + 40.0f;

            D3DVIEWPORT9 vp;
            float screenCenterX = 400.0f;
            if (SUCCEEDED(pDevice->GetViewport(&vp)) && vp.Width > 0)
                screenCenterX = vp.Width / 2.0f;

            float bannerX = screenCenterX - (bannerW / 2.0f);
            if (bannerX < 20.0f) bannerX = 20.0f;

            DrawRect(pDevice, bannerX - 2.0f, 12.0f, bannerW + 4.0f, 32.0f, bannerAccent);
            DrawRect(pDevice, bannerX, 14.0f, bannerW, 28.0f, bannerBg);
            DrawTextString(pDevice, bannerX + 20.0f, 22.0f, notifStr, bannerText, 1.25f);
        }

        // Apply and release state block to restore original game graphics pipeline
        if (pStateBlock)
        {
            pStateBlock->Apply();
            pStateBlock->Release();
        }
    }
}
